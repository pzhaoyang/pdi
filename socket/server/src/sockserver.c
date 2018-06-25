/*-----------------------------------------------------------------------------
 *
 *  sockserver.c
 *
 *  Socket Server
 *
 *  Copyright (c) 2013 Ericsson AB.
 *  Copyright (c) 2008 Redback Networks Inc.
 *  All rights reserved.
 *
 *-----------------------------------------------------------------------------
 */

#include "sockserver.h"
#include "globalDefs.h"

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
#include <errno.h>

#define sock_max(x,y) ((x) > (y) ? (x) : (y))

/*********************************************************************** 
  Function: sockserver_init
 
  Description: This function initializes a socket server.
  
  Input: inet - true to use AF_INET sockets, false to use AF_UNIX
         queueSize - maximum length the queue of pending connections may grow to.
         port - the IP port for INET, or a unique filename for UNIX
  Output: The generated socket server handle.

************************************************************************/
SOCK_STATUS
sockserver_init(socket_server_t *server,
                bool   inet,
                uint32 queueSize,
                uint32 port )
{
    int      sd;
    struct   sockaddr_in sin;      /* address of AF_INET server */
    struct   sockaddr_un sun;      /* address of AF_UNIX server */
    int32    sock_flag;
    int socket_family = (inet)? AF_INET : AF_UNIX;

    memset(server, 0, sizeof(socket_server_t));

    /* get an internet domain socket */
    if ((sd = socket(socket_family, SOCK_STREAM, 0 )) == -1) {
        perror("socket");
        return SOCK_STATUS_FAILURE;
    }

    /* Set FD options */
    if ((sock_flag = fcntl( sd, F_GETFL, 0 )) == -1 ) {
        close(sd);
        perror("get socket flag");
        return SOCK_STATUS_FAILURE;
    }
 
    if (fcntl( sd, F_SETFL, sock_flag | O_NONBLOCK ) == -1) {
        close(sd);
        perror("set socket flag");
        return SOCK_STATUS_FAILURE;
    } 

    if (inet) {
        /* complete the socket structure */
        bzero(&sin, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(port);

        /* bind the socket to the port number */
        if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
            close(sd);
            perror("bind");
            return SOCK_STATUS_FAILURE;
        }
    } else {

        bzero(&sun, sizeof(sun));        
        sun.sun_family = AF_UNIX;        
        /* path to the named socket */
        snprintf(sun.sun_path, sizeof(sun.sun_path), SOCK_NAMED_PATH, port);

        /* unlink to be sure no leftovers from a previous run */
        unlink(sun.sun_path);
        
        if (bind(sd, (struct sockaddr *)&sun, sizeof(sun)) < 0) {                
            close(sd);
            perror("bind");
            return SOCK_STATUS_FAILURE;
        }        
    }

    /* show that we are willing to listen */
    if (listen(sd, queueSize) == -1) {
        close(sd);
        perror("listen");
        return SOCK_STATUS_FAILURE;
    }

    /* set the return value */
    server->socket_des = sd;

    return SOCK_STATUS_SUCCESS;
}

SOCK_STATUS sockserver_close(socket_server_t *server)
{
    if (close(server->socket_des) == -1) {
        perror("close");
        return SOCK_STATUS_FAILURE;
    }

    return SOCK_STATUS_SUCCESS;
}


SOCK_STATUS sockserver_send( UNUSED socket_server_t *server,
                             int ep,
                             uint8* data_p,
                             uint32 num_bytes )
{
    if (send(ep, data_p, num_bytes, 0) == -1) {
        perror("send");
        return SOCK_STATUS_FAILURE;
    }
    return SOCK_STATUS_SUCCESS;
}

int sockserver_freeclient( socket_server_t *server )
{
    int i=0;

    /* look for an empty space */
    while ((i<SOCKSERVER_MAXCONNECTED_CLIENTS) && (server->accepted_sd[i] != 0)) {
        i++;
    }

    /* if no empty spaces, return error */
    if (i==SOCKSERVER_MAXCONNECTED_CLIENTS) {
        return -1;
    }

    return i;
}

/* this function attempts to listen to and read messages from multiple clients */
SOCK_STATUS sockserver_receive( socket_server_t *server,
                                uint8* buffer_p,
                                uint32 buf_len,
                                uint32* num_byte_p,
                                int* reply_ep )
{
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

    while (1) {

        msg_size = 0;

        /* zero the rdfds */
        FD_ZERO (&rdfds);

        /* set the main socket */
        nfds = sd;

        /* set the rdfds to listen from main socket */
        FD_SET(sd,&rdfds);

        /* see if any client sockets exist */
        for (client_id=0;client_id<SOCKSERVER_MAXCONNECTED_CLIENTS;client_id++) {
            if (server->accepted_sd[client_id] > 0) {
                /* update the max fds */
                nfds = sock_max(nfds,server->accepted_sd[client_id]);
                /* set the rdfds to listen from client socket */
                FD_SET(server->accepted_sd[client_id],&rdfds);
            }
        }

        /* wait for activity */
        select_count = select (nfds + 1, &rdfds, NULL, NULL, NULL);

        if ((select_count == -1) && (errno == EINTR))
            continue;
        if (select_count < 0) {
            perror ("select");
            return SOCK_STATUS_FAILURE;
        }

        if (FD_ISSET (sd, &rdfds)) {
            /* see if free space for new clients */
            client_id = sockserver_freeclient(server);

            if ((client_id == -1) || (client_id > SOCKSERVER_MAXCONNECTED_CLIENTS)) {
                /* if no space, exit */
                break;
            }

            /* see if a client wants to talk to us */
            if ((sd_current = accept(sd, (struct sockaddr *)  &pin, &addrlen)) == -1) {
                if (errno == EAGAIN) {
                    /* if no client trying to connect, break */
                    break;
                } else {
                    perror("accept");
                    return SOCK_STATUS_FAILURE;
                }
            }

            /* assign the accepted socket */
            server->accepted_sd[client_id] = sd_current;
#ifdef DEBUG_SOCKSERVER
            printf("Connecting from %s on port 0x%x\n",inet_ntoa(pin.sin_addr),ntohs(pin.sin_port));
#endif
        }

        for (client_id=0;client_id<SOCKSERVER_MAXCONNECTED_CLIENTS;client_id++) {
            sd_current = server->accepted_sd[client_id];

            /* skip over unconnected or no activity client sockets */
            if ((sd_current <= 0) ||
                !FD_ISSET(sd_current, &rdfds)) {
                continue;
            }

            /* get a message from the client */
            if ((msg_size=recv(sd_current, buffer_p, buf_len, 0)) == -1) {
                if (errno == EAGAIN) {
                    /* no message so continue */
                    continue;
                } else {
                    /* an error, return */
                    perror("recv");
                    return SOCK_STATUS_FAILURE;
                }
            }

            if (msg_size > 0) {
                break;
            }

            if (msg_size==0) {
                if (shutdown(sd_current,SHUT_RDWR) == -1) {
                    /* error, just log it */
                    perror("shutdown");
                }
                server->accepted_sd[client_id] = 0;
                continue;
            }

        }

        /* if a non-zero message has been received */
        if (msg_size>0) {
            /* set the size and return it */
            *num_byte_p = msg_size;
            *reply_ep = sd_current;
            break;
        }
    }

    return SOCK_STATUS_SUCCESS;
}


