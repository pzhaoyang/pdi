#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>

#include "sockserver.h"



#define sock_max(x,y)                               ((x) > (y) ? (x) : (y))


SOCK_STATUS sockserver_init(socket_server_t *server, bool inet, unsigned int queueSize, unsigned int port ){
    int      sd;
    struct   sockaddr_in sin;
    struct   sockaddr_un sun;
    int    sock_flag;
    int socket_family = (inet)? AF_INET : AF_UNIX;

    memset(server, 0, sizeof(socket_server_t));

    if((sd = socket(socket_family, SOCK_STREAM, 0 )) == -1){
        perror("socket");
        return SOCK_STATUS_FAILURE;
    }

    if((sock_flag = fcntl( sd, F_GETFL, 0 )) == -1 ){
        close(sd);
        perror("get socket flag");
        return SOCK_STATUS_FAILURE;
    }
 
    if(fcntl( sd, F_SETFL, sock_flag | O_NONBLOCK ) == -1){
        close(sd);
        perror("set socket flag");
        return SOCK_STATUS_FAILURE;
    } 

    if(inet){
        bzero(&sin, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(port);

        if(bind(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1){
            close(sd);
            perror("bind");
            return SOCK_STATUS_FAILURE;
        }
    }else{

        bzero(&sun, sizeof(sun));        
        sun.sun_family = AF_UNIX;        
        snprintf(sun.sun_path, sizeof(sun.sun_path), SOCK_NAMED_PATH, port);

        unlink(sun.sun_path);
        
        if(bind(sd, (struct sockaddr *)&sun, sizeof(sun)) < 0){                
            close(sd);
            perror("bind");
            return SOCK_STATUS_FAILURE;
        }        
    }

    if(listen(sd, queueSize) == -1){
        close(sd);
        perror("listen");
        return SOCK_STATUS_FAILURE;
    }

    server->socket_des = sd;

    return SOCK_STATUS_SUCCESS;
}

SOCK_STATUS sockserver_close(socket_server_t *server){
    if(close(server->socket_des) == -1){
        perror("close");
        return SOCK_STATUS_FAILURE;
    }

    return SOCK_STATUS_SUCCESS;
}

SOCK_STATUS sockserver_send( UNUSED socket_server_t *server, int ep, unsigned char* data_p, unsigned int num_bytes ){
    if(send(ep, data_p, num_bytes, 0) == -1){
        perror("send");
        return SOCK_STATUS_FAILURE;
    }
    return SOCK_STATUS_SUCCESS;
}

int sockserver_freeclient( socket_server_t *server ){
    int i=0;

    while ((i<SOCKSERVER_MAXCONNECTED_CLIENTS) && (server->accepted_sd[i] != 0)){
        i++;
    }

    if(i==SOCKSERVER_MAXCONNECTED_CLIENTS){
        return -1;
    }

    return i;
}

SOCK_STATUS sockserver_receive( socket_server_t *server, unsigned char* buffer_p, unsigned int buf_len, unsigned int* num_byte_p, int* reply_ep ){
    struct   sockaddr_in pin;
    int sd = server->socket_des;
    int      sd_current;
    socklen_t      addrlen;
    int      client_id;
    int      msg_size;
    int      nfds;
    int      select_count;
    fd_set   rdfds;
    addrlen = sizeof(pin);

    *num_byte_p = 0;
    *reply_ep = 0;

    while(1){

        msg_size = 0;
        FD_ZERO (&rdfds);
        nfds = sd;
        FD_SET(sd,&rdfds);

        for (client_id=0;client_id<SOCKSERVER_MAXCONNECTED_CLIENTS;client_id++){
            if(server->accepted_sd[client_id] > 0){
                nfds = sock_max(nfds,server->accepted_sd[client_id]);
                FD_SET(server->accepted_sd[client_id],&rdfds);
            }
        }

        select_count = select (nfds + 1, &rdfds, NULL, NULL, NULL);

        if((select_count == -1) && (errno == EINTR))
            continue;
        if(select_count < 0){
            perror ("select");
            return SOCK_STATUS_FAILURE;
        }

        if(FD_ISSET (sd, &rdfds)){
            client_id = sockserver_freeclient(server);

            if((client_id == -1) || (client_id > SOCKSERVER_MAXCONNECTED_CLIENTS)){
                break;
            }

            if((sd_current = accept(sd, (struct sockaddr *)  &pin, &addrlen)) == -1){
                if(errno == EAGAIN){
                    break;
                } else {
                    perror("accept");
                    return SOCK_STATUS_FAILURE;
                }
            }

            server->accepted_sd[client_id] = sd_current;
            printf("Connecting from %s on port 0x%x\n",inet_ntoa(pin.sin_addr),ntohs(pin.sin_port));
        }

        for (client_id=0;client_id<SOCKSERVER_MAXCONNECTED_CLIENTS;client_id++){
            sd_current = server->accepted_sd[client_id];

            if((sd_current <= 0) ||
                !FD_ISSET(sd_current, &rdfds)){
                continue;
            }

            if((msg_size=recv(sd_current, buffer_p, buf_len, 0)) == -1){
                if(errno == EAGAIN){
                    continue;
                } else {
                    perror("recv");
                    return SOCK_STATUS_FAILURE;
                }
            }

            if(msg_size > 0){
                break;
            }

            if(msg_size==0){
                if(shutdown(sd_current,SHUT_RDWR) == -1){
                    perror("shutdown");
                }
                server->accepted_sd[client_id] = 0;
                continue;
            }

        }

        if(msg_size>0){
            *num_byte_p = msg_size;
            *reply_ep = sd_current;
            break;
        }
    }

    return SOCK_STATUS_SUCCESS;
}
