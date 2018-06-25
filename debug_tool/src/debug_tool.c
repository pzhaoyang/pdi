#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "pdi_client.h"
#include "debug_tool.h"
#include "debug_tool_priv.h"


#define EXIT_SUCCESS        (0)
#define EXIT_FAILURE        (1)
#define PDI_MAX_PROMPT_LEN  (80)

// This PDI debug port number is used for connection between this debug tool
// and the PDI server running in the PDI server. Application user may change
// this vaiable at application startup for different applciations debugging.
unsigned int pdi_debug_port_num = PDI_PAD_PORT_NUM;

static pdi_msg_t msg;
static char prompt_string[PDI_MAX_PROMPT_LEN] = "-> ";

void pdi_client_prompt_set(char *new_prompt){
    strncpy (prompt_string, new_prompt, PDI_MAX_PROMPT_LEN);
}

int debug_read_string(int fd, register char string[], int maxbytes){
    char c;
    FAST int nchars = 0;
    FAST int i = 0;

    while(i < maxbytes){
        if((nchars = read(fd, &c, 1)) == PDI_ERROR){
            break;
        }

        if (nchars != 1 || c == '\n' || c == EOS){
            string[i++] = EOS;
            break;
        }

        string[i++] = c;
    }

    if (nchars == PDI_ERROR  || (nchars == 0 && i == 1)) {
        i = EOF;
    }

    return(i);
}

static void string_trim_right(char * str_to_trim){
    register char * str_cursor = NULL;   

    str_cursor = str_to_trim + strlen(str_to_trim) - 1;

    while (str_cursor > str_to_trim) {
        if(isspace((int)(*str_cursor))){
            str_cursor--;
        }else{
            break;
        }
    }

    if(str_cursor == str_to_trim){
        if(isspace((int)(*str_cursor))){
            *str_cursor = EOS;
            return;
        }
    }

    *(str_cursor+1) = EOS;
    return;
}

int debug_send_command(){
    FAST int i=0;

    msg.buff[PDI_MAX_LINE_LEN] = EOS;  

    for(i = 0; msg.buff[i] == ' '; i++);

    // a '#' char is treated as a comment and the remainder is ignored
    if(msg.buff [i] != '#' && msg.buff [i] != EOS){
        string_trim_right (&msg.buff[i]);
        if (msg.buff[i] == EOS){
            return PDI_OK;
        }

        if(strcmp(&msg.buff [i], "exit") == 0){
            i = 0;
            return PDI_ERROR;
        }

        msg.command_id = DEBUG_TOOL_COMMAND;
        sprintf(msg.tx_proc, "%s", "debug_tool");

        if(pdi_client_send_command(&msg)){
            return PDI_ERROR;
        }

        if( msg.command_id == DEBUG_TOOL_REPLY ){
            printf("process %s handled %s\n", msg.tx_proc, msg.buff);
        }else if( msg.command_id == DEBUG_TOOL_ERROR ){
            printf("process %s error %s\n", msg.tx_proc, msg.buff);
        }else{
            printf("error: response %d from process %s handled %s\n", msg.command_id, msg.tx_proc, msg.buff);
        }
    }
    return PDI_OK;
}

void debug_tool_loop(){
    while(TRUE){
        printf ("%s", prompt_string); fflush(stdout);

        if( debug_read_string (STD_IN, msg.buff, PDI_MAX_LINE_LEN) == EOF ) break;
        
        if( debug_send_command() == PDI_ERROR ) break;
    }
}

int main(int argc, char *argv[]){
    static const char * help_string = "debug_tool [-p pdi_port] [-c command]";
    int c;

    msg.buff[0] = EOS;

    while((c = getopt(argc, argv, "hc:p:")) != -1){
        switch (c) {
        case 'p':
            pdi_debug_port_num = (uint32)atoi(optarg);
            if(pdi_debug_port_num >= 0xFFFF || pdi_debug_port_num == 0){
                printf("debug_tool: invalid PDI port assignment: %s\n", optarg);
                exit(EXIT_FAILURE);
            }else{
                printf("debug_tool: the PDI port is changed from %d to %d.\n", PDI_PAD_PORT_NUM, pdi_debug_port_num);
            }
            break;
        case 'c':
            if(strlen(optarg) >= sizeof(msg.buff)){
                printf("debug_tool: input command string too long\n");
                exit(EXIT_FAILURE);
            }else{
                strncpy(msg.buff, optarg, sizeof(msg.buff));
            }
            break;
        case 'h':
        default:
            printf("%s\n", help_string);
            exit(EXIT_SUCCESS);
        }
    }

    if(pdi_client_init(PDI_HOST_IP, pdi_debug_port_num) != PDI_OK ){
        exit(EXIT_FAILURE);
    }
    pdi_client_prompt_set("dbg> ");

    if(msg.buff[0] == EOS){
        debug_tool_loop();
    }else{
        debug_send_command();
    }

    if(pdi_client_close() != PDI_OK){
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
