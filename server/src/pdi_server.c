/******************************************************************************
* Copyright (c) 2011-2014 Ericsson AB.
* Copyright (c) 2008 Redback Networks, Inc. All rights reserved.
* This software is the confidential and proprietary information of
* Redback Networks Inc.
*
* Description:
*
* The process debug infrastructure thread.
*
******************************************************************************/

#include "os_proc.h"
#include "pdi.h"
#include "pdi_internal.h"
#include "pdi_server.h"
#include "pdi_sym_table.h"
#include "sockserver.h"
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <inttypes.h>
#include <assert.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/prctl.h>

#define DEBUG_INFRA_SOCK_SERVER_CONNECT_Q_SIZE 5

/* socket server data */
LOCAL socket_server_t server;
LOCAL uint32          pdi_server_port = 0;
LOCAL pthread_t       pdi_thread_id;      /* PDI thread ID*/

LOCAL const char*     pdi_server_name = "unknown";

IMPORT void yystart (char *line);
IMPORT int yyparse ();

void print_banner(const char *func)
{
    printf("\n" SEPARATOR "\n");
    printf("    %s/%d/%s()\n", os_get_proc_name_self(), getpid(), func);
    printf(SEPARATOR "\n");
}

void print_sub_banner(const char *format, ...)
{
    char sub_banner_format[80];
    va_list arglist;

    /* format the sub banner */
    snprintf(sub_banner_format, sizeof(sub_banner_format),
             "\n    -> %s\n", format);

    /* print the sub banner and serparator */
    va_start( arglist, format );
    vprintf( sub_banner_format, arglist );
    va_end( arglist );

    printf(SEPARATOR "\n");
}

/******************************************************************************
* FUNCTION: string_trim_right - remove trailing white space from a string
*
* RETURNS: void.
* TODO: same function exists in debug_tool - consolidate to a library
******************************************************************************/
LOCAL void string_trim_right( char * str_to_trim )
{
    /* string cursor */
    FAST char * str_cursor = NULL;

    /* set str_cursor to end of str_to_trim */
    str_cursor = str_to_trim + strlen(str_to_trim) - 1;

    /* move backwords through str_to_trim until a non-space is found */
    while (str_cursor > str_to_trim) {
        if (isspace ((int)(*str_cursor))) {
            str_cursor--;
        } else {
            break;
        }
    }

    if (str_cursor == str_to_trim) {
        /* whole string is white space */
        if (isspace ((int)(*str_cursor))) {
            *str_cursor = EOS;
            return;
        }
    }

    /* Normal return, non-empty string */
    *(str_cursor+1) = EOS;
    return;
}

/******************************************************************************
* FUNCTION: pdi_execute - interpret and execute a source line
*
* RETURNS: PDI_OK or PDI_ERROR.
******************************************************************************/
STATUS pdi_execute( FAST char *line)
{
    STATUS status;

    /* parse and execute line */
    yystart (line);
    status = (yyparse () == 0) ? PDI_OK : PDI_ERROR;

    return(status);
}

/******************************************************************************
* FUNCTION: pdi_command_loop - execute stream of debug commands
*
* RETURNS: PDI_OK or PDI_ERROR.
******************************************************************************/
STATUS pdi_command_loop( void )
{
    FAST int i;
    STATUS status = PDI_OK;
    int stdout_fd;
    int newstdout_fd;

    pdi_msg_t    rx_msg;
    pdi_msg_t    tx_msg;

    int reply_ep;

    uint32 msg_size;

    /* remember the original stdout fd so we can set it back after the command executes */
    stdout_fd = dup(STDOUT_FILENO);
    if (0 > stdout_fd) {
        syslog(LOG_ERR, "PDI cannot dup stdout %s", strerror(errno));
        return PDI_ERROR;
    }

    if (sockserver_init(&server, false,
                        DEBUG_INFRA_SOCK_SERVER_CONNECT_Q_SIZE,
                        pdi_server_port) != SOCK_STATUS_SUCCESS) {
        syslog(LOG_ERR, "PDI cannot init server");
        close(stdout_fd);
        return PDI_ERROR;
    }

    while (TRUE) {
        if (sockserver_receive(&server,(uint8*)&rx_msg,
                               sizeof(pdi_msg_t),
                               &msg_size,&reply_ep) != SOCK_STATUS_SUCCESS) {
            syslog(LOG_ERR, "PDI fatal rx error");
            status = PDI_ERROR;
            break;
        }

        if (msg_size <= 0) {
            syslog(LOG_ERR, "PDI rx error mgs size %d", msg_size);
            continue;
        }

        tx_msg.command_id = DEBUG_TOOL_REPLY;

        sprintf (tx_msg.tx_proc,"%s", pdi_server_name);
        sprintf (tx_msg.buff,"%s",rx_msg.buff);

        /* make sure inLine has EOS */
        rx_msg.buff [PDI_MAX_LINE_LEN] = EOS;

        /* skip leading blanks */
        for (i = 0; rx_msg.buff [i] == ' '; i++)
            ;

        /* ignore comments, blank lines, and null lines */
        if (rx_msg.buff [i] != '#' && rx_msg.buff [i] != EOS) {

            /* Eliminate trailing space */
            string_trim_right (&rx_msg.buff[i]);
            if (rx_msg.buff[i] == EOS) {
                sockserver_send(&server,reply_ep,(uint8*)&tx_msg,sizeof(pdi_msg_t));
                continue;
            }
        }

        /* redirect stdout to the stdout of the calling shell   */
        /* IMPORTANT NOTE - only works if the input shell is on */
        /* the same machine as the target process               */
        newstdout_fd = open(rx_msg.stdout_path, O_WRONLY | O_NONBLOCK );
        if (0 > newstdout_fd) {
            tx_msg.command_id = DEBUG_TOOL_ERROR;
            sprintf (tx_msg.buff,
                    "cannot open %s errno %d (%s)",
                    rx_msg.stdout_path,
                    errno,
                    strerror(errno));
        } else if (0 > dup2(newstdout_fd, STDOUT_FILENO)) {
            tx_msg.command_id = DEBUG_TOOL_ERROR;
            sprintf (tx_msg.buff,
                    "cannot dup2 %s errno %d (%s)",
                    rx_msg.stdout_path,
                    errno,
                    strerror(errno));
            close(newstdout_fd);
        } else {
            /* parse & execute command */
            status = pdi_execute (&rx_msg.buff[i]);

            fflush(stdout);

            close(newstdout_fd);
        }

        /* reply to the client AFTER executing*/
        if (sockserver_send(&server,reply_ep,(uint8*)&tx_msg,sizeof(pdi_msg_t)) !=
            SOCK_STATUS_SUCCESS) {
            syslog(LOG_ERR, "PDI cannot send reply");
            status = PDI_ERROR;
            break;
        }

        /* set STDOUT back to the original */
        if (0 > dup2(stdout_fd, STDOUT_FILENO)) {
            syslog(LOG_ERR, "PDI cannot dup2 stdout %s", strerror(errno));
            status = PDI_ERROR;
            break;
        }
    }

    /* close the server */
    if (sockserver_close(&server) != SOCK_STATUS_SUCCESS) {
        syslog(LOG_ERR, "PDI cannot close server");
        status = PDI_ERROR;
    }

    /* close the fd */
    if (-1 == close(stdout_fd)) {
        syslog(LOG_ERR, "PDI cannot close fd %s", strerror(errno));
        status =  PDI_ERROR;
    }

    return(status);
}

/******************************************************************************
* FUNCTION: pdi_thread - main for debug thread
*
* RETURNS: none.
******************************************************************************/
void * pdi_thread( void * arg)
{
    (void)arg;
    prctl(PR_SET_NAME,"ald_pdi", 0, 0, 0);
    pdi_command_loop();
    return NULL;
}

/******************************************************************************
* FUNCTION: pdi_server_init - initialize the pdi server
*
* RETURNS: PDI_OK or PDI_ERROR.
******************************************************************************/
void pdi_server_init(
    uint32 server_port,
    PDI_THREAD_CREATE_FUNCPTR thread_create_func,
    const char* name)
{
    uint32  ix;

    pdi_server_port = server_port;
    if (name != NULL) {
        pdi_server_name = name;
    }

    /* create system and status symbol tables */

    pdi_sym_tbl_id = pdi_sym_tbl_create (SYM_TBL_HASH_SIZE_LOG2, TRUE);

    printf ("\nAdding %" PRIu32 " symbols for %s.\n", standTblSize, pdi_server_name);

    /* fill in from built in table*/

    for (ix = 0; ix < standTblSize; ix++)
      pdi_sym_tbl_add (pdi_sym_tbl_id, &(standTbl[ix]));

    /* create the thread */
    if (thread_create_func == NULL) {
        assert( pthread_create(&pdi_thread_id, NULL, pdi_thread, NULL) == 0);
    }
    else {
        assert( thread_create_func(pdi_thread) == 0);
    }
}

