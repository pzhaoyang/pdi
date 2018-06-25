#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>
#include <inttypes.h>
#include <assert.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/prctl.h>

#include "pdi_internal.h"
#include "pdi_server.h"
#include "pdi_sym_table.h"
#include "sockserver.h"
#include "os_proc.h"


#define DEBUG_INFRA_SOCK_SERVER_CONNECT_Q_SIZE      (5)

static socket_server_t                              server;
static unsigned int                                 pdi_server_port = 0;
static pthread_t                                    pdi_thread_id;
static const char*                                  pdi_server_name = "unknown";

extern void yystart (char *line);
extern int yyparse ();


void print_banner(const char *func){
    printf("\n" SEPARATOR "\n");
    printf("    %s/%d/%s()\n", os_get_proc_name_self(), getpid(), func);
    printf(SEPARATOR "\n");
}

void print_sub_banner(const char *format, ...){
    char sub_banner_format[80];
    va_list arglist;

    snprintf(sub_banner_format, sizeof(sub_banner_format), "\n    -> %s\n", format);

    va_start( arglist, format );
    vprintf( sub_banner_format, arglist );
    va_end( arglist );

    printf(SEPARATOR "\n");
}

static void string_trim_right( char * str_to_trim ){
    register char * str_cursor = NULL;

    str_cursor = str_to_trim + strlen(str_to_trim) - 1;

    while(str_cursor > str_to_trim){
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

STATUS pdi_execute( FAST char *line){
    STATUS status;

    yystart (line);
    status = (yyparse () == 0) ? PDI_OK : PDI_ERROR;

    return(status);
}

STATUS pdi_command_loop( void ){
    register int i;
    STATUS status = PDI_OK;
    int stdout_fd;
    int newstdout_fd;

    pdi_msg_t    rx_msg;
    pdi_msg_t    tx_msg;

    int reply_ep;

    unsigned int msg_size;

    stdout_fd = dup(STDOUT_FILENO);
    if(0 > stdout_fd){
        syslog(LOG_ERR, "PDI cannot dup stdout %s", strerror(errno));
        return PDI_ERROR;
    }

    if(sockserver_init(&server, false, DEBUG_INFRA_SOCK_SERVER_CONNECT_Q_SIZE, pdi_server_port) != SOCK_STATUS_SUCCESS){
        syslog(LOG_ERR, "PDI cannot init server");
        close(stdout_fd);
        return PDI_ERROR;
    }

    while (TRUE){
        if(sockserver_receive(&server,(unsigned char*)&rx_msg, sizeof(pdi_msg_t), &msg_size,&reply_ep) != SOCK_STATUS_SUCCESS){
            syslog(LOG_ERR, "PDI fatal rx error");
            status = PDI_ERROR;
            break;
        }

        if(msg_size <= 0){
            syslog(LOG_ERR, "PDI rx error mgs size %d", msg_size);
            continue;
        }

        tx_msg.command_id = DEBUG_TOOL_REPLY;

        sprintf (tx_msg.tx_proc,"%s", pdi_server_name);
        sprintf (tx_msg.buff,"%s",rx_msg.buff);

        rx_msg.buff [PDI_MAX_LINE_LEN] = EOS;

        for (i = 0; rx_msg.buff [i] == ' '; i++);

        if(rx_msg.buff [i] != '#' && rx_msg.buff [i] != EOS){
            string_trim_right (&rx_msg.buff[i]);
            if(rx_msg.buff[i] == EOS){
                sockserver_send(&server,reply_ep,(unsigned char*)&tx_msg,sizeof(pdi_msg_t));
                continue;
            }
        }

        newstdout_fd = open(rx_msg.stdout_path, O_WRONLY | O_NONBLOCK );
        if(0 > newstdout_fd){
            tx_msg.command_id = DEBUG_TOOL_ERROR;
            sprintf (tx_msg.buff, "cannot open %s errno %d (%s)", rx_msg.stdout_path, errno, strerror(errno));
        }else if(0 > dup2(newstdout_fd, STDOUT_FILENO)){
            tx_msg.command_id = DEBUG_TOOL_ERROR;
            sprintf (tx_msg.buff, "cannot dup2 %s errno %d (%s)", rx_msg.stdout_path, errno, strerror(errno));
            close(newstdout_fd);
        }else{
            status = pdi_execute (&rx_msg.buff[i]);

            fflush(stdout);

            close(newstdout_fd);
        }

        if(sockserver_send(&server,reply_ep,(unsigned char*)&tx_msg,sizeof(pdi_msg_t)) != SOCK_STATUS_SUCCESS){
            syslog(LOG_ERR, "PDI cannot send reply");
            status = PDI_ERROR;
            break;
        }

        if(0 > dup2(stdout_fd, STDOUT_FILENO)){
            syslog(LOG_ERR, "PDI cannot dup2 stdout %s", strerror(errno));
            status = PDI_ERROR;
            break;
        }
    }

    if(sockserver_close(&server) != SOCK_STATUS_SUCCESS){
        syslog(LOG_ERR, "PDI cannot close server");
        status = PDI_ERROR;
    }

    if(-1 == close(stdout_fd)){
        syslog(LOG_ERR, "PDI cannot close fd %s", strerror(errno));
        status =  PDI_ERROR;
    }

    return(status);
}


void* pdi_thread( void * arg){
    (void)arg;
    prctl(PR_SET_NAME,"pdid", 0, 0, 0);
    pdi_command_loop();
    return NULL;
}

void pdi_server_init( unsigned int server_port, PDI_THREAD_CREATE_FUNCPTR thread_create_func, const char* name){
    unsigned int  ix;

    pdi_server_port = server_port;
    
    if(name != NULL){
        pdi_server_name = name;
    }

    pdi_sym_tbl_id = pdi_sym_tbl_create(SYM_TBL_HASH_SIZE_LOG2, TRUE);

    printf ("\nAdding %" PRIu32 " symbols for %s.\n", standTblSize, pdi_server_name);

    for (ix = 0; ix < standTblSize; ix++)
      pdi_sym_tbl_add(pdi_sym_tbl_id, &(standTbl[ix]));

    if(thread_create_func == NULL){
        assert(pthread_create(&pdi_thread_id, NULL, pdi_thread, NULL) == 0);
    }
    else {
        assert(thread_create_func(pdi_thread) == 0);
    }
}
