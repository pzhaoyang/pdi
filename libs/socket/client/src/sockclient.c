#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include "sockclient.h"

SOCK_STATUS sockclient_init(int *socket_num, bool   inet, const char* host, unsigned int port){
    int sd;
    struct sockaddr_in pin;
    struct sockaddr_un pun;
    int socket_family = (inet)? AF_INET : AF_UNIX;

    *socket_num = 0;

    if((sd = socket(socket_family, SOCK_STREAM, 0)) == -1){
        perror("socket");
        return SOCK_STATUS_FAILURE;
    }

    if(inet){
        bzero(&pin, sizeof(pin));
        pin.sin_family = AF_INET;
        pin.sin_addr.s_addr = inet_addr( host );
        pin.sin_port = htons(port);

        if(connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1){
            perror("connect");
            return SOCK_STATUS_FAILURE;
        }
    }else{
        bzero(&pun, sizeof(pun));        
        pun.sun_family = AF_UNIX;        
        snprintf(pun.sun_path, sizeof(pun.sun_path), SOCK_NAMED_PATH, port);

        if(connect(sd,(struct sockaddr *)  &pun, sizeof(pun)) == -1){
            perror("connect");
            return SOCK_STATUS_FAILURE;
        }
    }

    *socket_num = sd;

    return SOCK_STATUS_SUCCESS;
}

SOCK_STATUS sockclient_close(int *socket_num){
    if(close(*socket_num) == -1){
        perror("close");
        return SOCK_STATUS_FAILURE;
    }

    return SOCK_STATUS_SUCCESS;
}

SOCK_STATUS sockclient_send( int *socket_num, uint8* data_p, uint32 num_bytes ){
    if(send(*socket_num, data_p, num_bytes, 0) == -1){
        perror("send");
        return SOCK_STATUS_FAILURE;
    }

    return SOCK_STATUS_SUCCESS;
}

SOCK_STATUS sockclient_receive( int socket_num, unsigned char* data_p, unsigned int buf_len, unsigned int* num_byte_p ){
    int      msg_size;

    *num_byte_p = 0;

    if((msg_size = recv(socket_num, data_p, buf_len, 0)) == -1){
        perror("recv");
        return SOCK_STATUS_FAILURE;
    }

    *num_byte_p = msg_size;

    return SOCK_STATUS_SUCCESS;
}
