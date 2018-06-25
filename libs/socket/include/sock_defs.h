#ifndef __SOCK_DEFS_H__
#define __SOCK_DEFS_H__

#include "stdio.h"
#include "string.h"

#define SOCK_NAMED_PATH         "/tmp/sock_%d"

typedef enum{
    SOCK_STATUS_FAILURE = -1,
    SOCK_STATUS_SUCCESS,
    SOCK_STATUS_WARNING,
    SOCK_STATUS_TIMEOUT,
    SOCK_STATUS_SERVER_GONE,
    SOCK_STATUS_PEER_GONE
} SOCK_STATUS;

#endif
