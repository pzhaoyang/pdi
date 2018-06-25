#ifndef __SOCKCLIENT_H__
#define __SOCKCLIENT_H__


#include "sock_defs.h"


SOCK_STATUS sockclient_init(int *socket_num,  bool   inet, const char* host, uint32 port);
SOCK_STATUS sockclient_close(int *socket_num);
SOCK_STATUS sockclient_send( int *socket_num, unsigned char* data_p, unsigned int num_bytes );
SOCK_STATUS sockclient_receive( int socket_num, unsigned char* data_p, unsigned int buf_len, unsigned int* num_byte_p );

#endif
