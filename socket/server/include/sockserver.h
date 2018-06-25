/*-----------------------------------------------------------------------------
 *
 *  sockserver.h
 *
 *  Socket Server
 *
 *  Copyright (c) 2008 Redback Networks Inc.
 *  All rights reserved.
 *
 *-----------------------------------------------------------------------------
 */

#ifndef __SOCKSERVER_H__
#define __SOCKSERVER_H__

#include "sock_defs.h"

#define SOCKSERVER_MAXCONNECTED_CLIENTS 20

typedef struct {
    int socket_des;
    int accepted_sd[SOCKSERVER_MAXCONNECTED_CLIENTS];
} socket_server_t;

SOCK_STATUS sockserver_init(socket_server_t *server,
                            bool   inet,
                            uint32 queueSize,
                            uint32 port );

SOCK_STATUS sockserver_close(socket_server_t *server);

SOCK_STATUS sockserver_send( socket_server_t *server,
                             int ep,
                             uint8* data_p,
                             uint32 num_bytes );

SOCK_STATUS sockserver_receive( socket_server_t *server,
                                uint8* buffer_p,
                                uint32 buf_len,
                                uint32* num_byte_p,
                                int* reply_ep );

#endif /* __SOCKSERVER_H__ */

