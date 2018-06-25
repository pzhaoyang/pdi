#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

#include "pdi_client.h"
#include "sockclient.h"


static int          socket_num;
static char         stdout_pipe[PDI_MAX_LINE_LEN+1];
static int          stdout_pipe_fd;

int pdi_create_stdout_pipe(){
    snprintf(stdout_pipe, PDI_MAX_LINE_LEN, "/tmp/dbgtl_%d", (int)getpid());
    if (0 != mkfifo(stdout_pipe, 0666)) {
        return PDI_ERROR;
    }
    return PDI_OK;
}


int pdi_client_init(const char* host, unsigned int port){
    if(PDI_OK != pdi_create_stdout_pipe()){
        return PDI_ERROR;
    }

    if(sockclient_init(&socket_num, false, host, port) != SOCK_STATUS_SUCCESS ){
        return PDI_ERROR;
    }

    return PDI_OK;
}

int pdi_client_close() {
    unlink(stdout_pipe);
    if(sockclient_close(&socket_num) != SOCK_STATUS_SUCCESS){
        return PDI_ERROR;
    }

    return PDI_OK;
}

int pdi_client_send_command(pdi_msg_t *msg){
    uint32 msg_size;
    char inbuf[1024];
    int nfds;
    fd_set readfds;
    struct timeval timeout;
    int rc = PDI_OK;
    int reply = 0, got_eof = 0;
    int nread;

    strncpy(msg->stdout_path, stdout_pipe, PDI_MAX_LINE_LEN);

    stdout_pipe_fd = open(stdout_pipe, O_RDONLY | O_NONBLOCK);
    if(stdout_pipe_fd < 0){
        printf("failed to open file %s %d\n", stdout_pipe, errno);
        return PDI_ERROR;
    }

    if(sockclient_send(&socket_num,(uint8*)msg, sizeof(pdi_msg_t)) != SOCK_STATUS_FAILURE){
        nfds = stdout_pipe_fd > socket_num ? stdout_pipe_fd : socket_num;
        
        while (!reply || !got_eof) {
            timeout.tv_sec = 10;
            timeout.tv_usec = 0;
            FD_ZERO (&readfds);
            FD_SET(socket_num, &readfds);
            FD_SET(stdout_pipe_fd, &readfds);
            if(-1 != select(nfds+1, &readfds, NULL, NULL, &timeout)){
                if (FD_ISSET(socket_num, &readfds)) {    
                    if(sockclient_receive(socket_num,(uint8*)msg, sizeof(pdi_msg_t), &msg_size) == SOCK_STATUS_FAILURE){
                        rc = PDI_ERROR;
                        got_eof = 1;
                    }else if(DEBUG_TOOL_ERROR == msg->command_id) {
                        rc = PDI_OK;
                        got_eof = 1;
                    }
                    reply = 1;
                }
                if(FD_ISSET(stdout_pipe_fd, &readfds)){
                    do{
                        memset(inbuf, 0, sizeof(inbuf));
                        nread = read(stdout_pipe_fd, inbuf, sizeof(inbuf)-1);
                        if(nread > 0){
                            printf("%s", inbuf);
                        }
                    }while(nread != 0);
                    got_eof = 1;
                }
            }
        }
    }else{
        rc = PDI_ERROR;
    }

    close(stdout_pipe_fd);
    return rc;
}

