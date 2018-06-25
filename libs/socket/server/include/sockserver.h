#ifndef __SOCKSERVER_H__
#define __SOCKSERVER_H__

#include "sock_defs.h"

#define SOCKSERVER_MAXCONNECTED_CLIENTS         20

typedef struct{
    int                     socket_des;
    int                     accepted_sd[SOCKSERVER_MAXCONNECTED_CLIENTS];
} socket_server_t;

SOCK_STATUS sockserver_init(socket_server_t *server, bool   inet, unsigned int queueSize, unsigned int port );
SOCK_STATUS sockserver_close(socket_server_t *server);
SOCK_STATUS sockserver_send( socket_server_t *server, int ep, unsigned char* data_p, unsigned int num_bytes );
SOCK_STATUS sockserver_receive( socket_server_t *server, unsigned char* buffer_p, unsigned int buf_len, unsigned int* num_byte_p, int* reply_ep );

#endif
