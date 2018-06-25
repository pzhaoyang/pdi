#ifndef OS_PROC_H_
#define OS_PROC_H_

#include <sys/types.h>
#include <unistd.h>

#define OS_PROC_PATH_MAX            (256)


char* os_get_proc_cmdline( char *cmdline, size_t cmdline_sz, pid_t pid);
char* os_get_proc_name( char *buf, size_t size, pid_t pid );
char* os_get_proc_path( char *buf, size_t size, pid_t pid );
const char* os_get_proc_name_self( void );

#endif
