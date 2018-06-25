#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "os_proc.h"

char *os_get_proc_cmdline(char* cmdline, size_t cmdline_sz, pid_t pid){
    int n;
    char cmdfile[OS_PROC_PATH_MAX];
    char *p;
    FILE *f;

    *cmdline = 0;

    n = snprintf(cmdfile, sizeof(cmdfile), "/proc/%d/cmdline", pid);
    if((0 >= n) || ((int)sizeof(cmdfile) <= n)){
        return (NULL);
    }

    f = fopen(cmdfile, "r");
    if(!f){
        return (NULL);
    }

    p = fgets(cmdline, cmdline_sz, f);

    fclose(f);

    return p;
}


char* os_get_proc_name( char *buf, size_t size, pid_t pid ){
    char cmdline[OS_PROC_PATH_MAX];
    char *p;

    *buf = 0;

    p = os_get_proc_cmdline(cmdline, sizeof(cmdline), pid);

    if(NULL == p){
        return (buf);
    }

    p = strrchr(cmdline, '/');

    if(NULL == p){
        snprintf( buf, size, "%s", cmdline);
    }else {
        snprintf( buf, size, "%s", p+1);
    }

    p = strchr(buf, ' ');
    if (NULL != p) {
        *p = 0;
    }
    
    return (buf);
}

char* os_get_proc_path( char *buf, size_t size, pid_t pid ){
    char cmdline[OS_PROC_PATH_MAX];
    char *p;

    *buf = 0;

    p = os_get_proc_cmdline(cmdline, sizeof(cmdline), pid);

    if (NULL == p) {
        return (buf);
    }

    p = strrchr(cmdline, '/');

    if (NULL == p) {
        return (buf);
    }

    *p = 0;

    snprintf( buf, size, "%s", cmdline);

    return (buf);
}

const char* os_get_proc_name_self( void ){
    static char proc_name[OS_PROC_PATH_MAX] = "";

    if (0 == strnlen(proc_name, sizeof(proc_name))) {
        return (os_get_proc_name(proc_name, sizeof(proc_name), getpid()));
    }

    return (proc_name);
}
