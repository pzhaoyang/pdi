/******************************************************************************
 * Copyright (c) 2011 Ericsson AB.
 * Copyright (c) 2008 Redback Networks, Inc.
 * This software is the confidential and proprietary information of
 * Redback Networks Inc.
 *
 * Description:
 *
 * Provides implementation for debug commands.
******************************************************************************/

#include "pdi_sym_table.h"
#include "pdi_demangler.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>

#define DEBUG_USR_DEMANGLE_PRINT_LEN 256  /* Num chars of demangled names to print */
#define DEBUG_NUM_SYMBLS  3

typedef struct      /* SYMBL - symbol table entry used by lkAddr */ {
    void *addr;     /* address associated with symbol */
    char *name;     /* points to name in system symbol table */
    SYM_TYPE type;  /* type of this symbol table entry */
} SYMBL;

typedef struct      /* LKADDR_ARG */ {
    int count;          /* number of entries printed by lkAddr */
    void *addr;      /* target address */
    SYMBL symbl[DEBUG_NUM_SYMBLS];
} LKADDR_ARG;

/******************************************************************************
* FUNCTION: help - print a synopsis of selected routines
*
* RETURNS: N/A
******************************************************************************/
void help (void)
{
    static char *help_msg [] = {
        "help                           Print this list",
        "lkup      [\"substr\"]         List symbols in system symbol table",
        "lkAddr    address              List symbol table entries near address",
        NULL
    };
    int ix;

    printf ("\n");
    for (ix = 0; help_msg [ix] != NULL; ix++) {
        printf ("%s\n", help_msg [ix]);
    }
    printf ("\n");
}

/******************************************************************************
* FUNCTION: lkup - list symbols
*
* RETURNS: none.
******************************************************************************/
void lkup( char *substr )
{
    pdi_sym_show ( pdi_sym_tbl_id, substr);   /* pdi_sym_show() does the work */
}

/******************************************************************************
* FUNCTION: printSTE - print symbol table entry
*
* RETURNS: none.
******************************************************************************/
void printSTE( void * addr, char * name, SYM_TYPE type )
{
    char demangled[DEBUG_USR_DEMANGLE_PRINT_LEN + 1];
    const char *nameToPrint = pdi_cplus_demangle (name, demangled, sizeof (demangled), COMPLETE);

    printf ("%p %-40s %-8s", addr, nameToPrint, pdi_typeName[(type >> 1) & 7]);

    if ((type & N_EXT) == 0)
        printf (" (local)");

    printf ("\n");
}

/******************************************************************************
* FUNCTION: lkAddrPrintSame - yet another support routine for lkAddr()
*
* This command is called by pdi_sym_each() to deal with each symbol in the table.
* If the value associated with the symbol is equal to the target, print it.
*
* RETURNS: TRUE
******************************************************************************/
BOOL lkAddrPrintSame( char *   name,
                      void *   value,
                      SYM_TYPE     type,
                      LKADDR_ARG * arg )
{
    if (value == arg->addr) {
        printSTE (value, name, type);
        arg->count++;
    }

    return(TRUE);
}

/******************************************************************************
* FUNCTION: lkAddrNext - another support routine for lkAddr()
*
* This command is called by pdi_sym_each() to deal with each symbol in the table.
* If the value associated with the symbol is greater than target, but less
* than symbl[2].addr, it replaces symbl[2].addr.
*
* RETURNS: TRUE
******************************************************************************/
BOOL lkAddrNext( char * name,
                 void * value,
                 SYM_TYPE     type,
                 LKADDR_ARG * arg)
{
    void *addr = value;

    if (addr > arg->addr) {
        if (addr < arg->symbl[2].addr ||
            arg->symbl[2].addr == NULL) {
            /* found closer symbol that is greater than target */

            arg->symbl[2].addr = addr;
            arg->symbl[2].name = name;
            arg->symbl[2].type = type;
        }
    }

    return(TRUE);
}

/******************************************************************************
* FUNCTION: lkAddrFind - support routine for lkAddr()
*
* This command is called by pdi_sym_each() to deal with each symbol in the table.
* If the value associated with the symbol is equal to lkAddrTarg(), or closer
* to it than previous close values, the appropriate slot in the array `symbl'
* is filled with the data for this symbol.
*
* RETURNS: TRUE
******************************************************************************/
BOOL lkAddrFind( char * name,
                 void * value,
                 SYM_TYPE     type,
                 LKADDR_ARG * arg )
{
    void * addr = value;

    if (addr < arg->addr) {
        if (addr > arg->symbl[0].addr) {
            /* found closer symbol that is less than target */

            arg->symbl[0].addr = addr;
            arg->symbl[0].name = name;
            arg->symbl[0].type = type;
        }
    } else if (addr == arg->addr) {
        /* found target, fill in target entry */

        arg->symbl[1].addr = addr;
        arg->symbl[1].name = name;
        arg->symbl[1].type = type;
    } else if (addr > arg->addr) {
        if ((addr < arg->symbl[2].addr) ||
            (arg->symbl[2].addr == NULL)) {
            /* found closer symbol that is greater than target */

            arg->symbl[2].addr = addr;
            arg->symbl[2].name = name;
            arg->symbl[2].type = type;
        }
    }

    return(TRUE);
}

/******************************************************************************
* FUNCTION: lkAddr - list symbols whose values are near a specified value
*
* RETURNS: N/A
******************************************************************************/
void lkAddr( void *addr )
{
    int ix;
    LKADDR_ARG arg;

    arg.count = 0;          /* haven't printed anything yet */
    arg.addr  = addr;

    for (ix = 0; ix < DEBUG_NUM_SYMBLS; ++ix)
        arg.symbl[ix].addr = NULL; /* clear little symbol table */

    /* call lkAddrFind for each symbol */

    pdi_sym_each (pdi_sym_tbl_id, (FUNCPTR)lkAddrFind, (intptr_t)&arg);

    /* print out the symbols found */

    for (ix = 0; ix < DEBUG_NUM_SYMBLS; ix++) {
        if (arg.symbl[ix].addr != NULL) {
            arg.addr = arg.symbl[ix].addr;
            pdi_sym_each (pdi_sym_tbl_id, (FUNCPTR)lkAddrPrintSame, (intptr_t)&arg);
        }
    }

    if (arg.symbl[2].addr == NULL)
        return;         /* couldn't find anything greater */

    /* print until there are enough entries */

    while (arg.count < 12) {
        arg.addr = arg.symbl[2].addr;

        arg.symbl[2].addr = NULL;

        /* find next entry */

        pdi_sym_each (pdi_sym_tbl_id, (FUNCPTR)lkAddrNext, (intptr_t)&arg);

        if (arg.symbl[2].addr == NULL)
            break;          /* couldn't find anything greater */

        /* print them */

        arg.addr = arg.symbl[2].addr;
        pdi_sym_each (pdi_sym_tbl_id, (FUNCPTR)lkAddrPrintSame, (intptr_t)&arg);
    }
}

/******************************************************************************
* FUNCTION: substrcmp - determine if string <s1> is a slash-delineated word in string <s>
*
* This routine returns TRUE if string <s1> is a substring of <s>, where the
* substring is a word delineated by slashes.  If there are no slashes, both
* strings must be identical.
*
* EXAMPLES
*  substrcmp("/this/is/a/string/", "string") returns TRUE.
*  substrcmp("/this/is/a/string/", "str") returns FALSE.
*  substrcmp("string", "string") returns TRUE.
*
* RETURNS: TRUE or FALSE.
******************************************************************************/
BOOL substrcmp( char *s, char *s1 )
{
    char *pSubStr=s;    /* pointer to string divided by slashes in the string */
    char *pSlash;   /* pointer to slash in the first string */
    size_t strLen;
    BOOL found = FALSE;


    while (pSubStr != NULL) {
        pSlash = index (pSubStr, '/');
        if (pSlash == NULL) {
            found = (strcmp (pSubStr, s1) == 0);
            break;
        } else {
            strLen = pSlash - pSubStr;
            if ((found = (strncmp (pSubStr, s1, strLen) == 0 &&
                          (strlen(s1) == strLen))))
                break;
            else
                pSubStr = pSlash + 1;   /* point to char next to slash */
        }
    }
    return(found);
}

/******************************************************************************
* FUNCTION: pdi_util_line_no_feed - draw a line without line feed at the end
*
* RETURNS: none.
******************************************************************************/
void pdi_util_line_no_feed( char type, size_t len )
{
    size_t i;

    for (i = 0; i < len; i ++) {
        putchar(type);
    }
}

/******************************************************************************
* FUNCTION: pdi_util_line - draw a line with line feed at the end
*
* RETURNS: none.
******************************************************************************/
void pdi_util_line( char type, size_t len )
{
    pdi_util_line_no_feed(type, len);
    putchar('\n');
}

/******************************************************************************
* FUNCTION: _is_mem_read_accessible - check if a piece of memory is readble
*
* RETURNS: none.
******************************************************************************/
static BOOL _is_mem_read_accessible( void * addr, size_t bytes )
{
    int fd;
    int rc;
    int err;

    fd = open("/dev/random", O_WRONLY);
    if (fd == -1) {
        /* something unexpected happening here */
        printf("Open file 'dev/random' failed: %s\n", strerror(errno));
        return (FALSE);
    } else {
        rc = write(fd, addr, bytes);
        err = errno;
        close(fd);
        if (rc == -1) {
            /* write failed, most likely due to bad address */
            printf("Write 'dev/random' from %p (size=%u) failed: %s\n",
                   addr, (unsigned int) bytes, strerror(err));
            return (FALSE);
        }
    }

    return (TRUE);
}

/******************************************************************************
* FUNCTION: _mem_show - show a piece of memory
*
* RETURNS: none.
******************************************************************************/
static void _mem_show( unsigned char * addr, size_t bytes )
{
    size_t i;
    size_t j=0;
    unsigned char buff[18];

    printf("\nShow memory at %p:\n", addr);
    pdi_util_line('-', 78);
    buff[17] = '\0';
    for (i = 0; i < bytes; i ++) {
        if (! (i & 0xF)) {
            /* first byte in a line, print address */
            printf("%08X: ", (unsigned int)(uintptr_t)(addr + i));
            j = 0;
        } else if (! (i & 7)) {
            /* 8th byte in a line, print a separator */
            printf(" ");
            buff[j ++] = ' ';
        }
        printf(" %02X", addr[i]);
        if (isprint(addr[i])) {
            buff[j ++] = addr[i];
        } else {
            buff[j ++] = '.';
        }
        if ((i & 0xF) == 0xF) {
            /* last char in a line */
            printf("  %s\n", buff);
        }
    }

    /* print left over */
    if ((i & 0xF) != 0) {
        /* last char in a line */
        buff[j] = '\0';
        j = (16 - (i & 0xF)) * 3;
        if ((i & 0xF) <= 8) {
            j ++;
        }
        pdi_util_line_no_feed(' ', j);
        printf("  %s\n", buff);
    }

    pdi_util_line('-', 78);
}

/******************************************************************************
* FUNCTION: pdi_mem_show - show a piece of memory
*
* RETURNS: none.
*
* NOTES: 
* 1. This function can be invoked through PDI client to dump a piece of memory.
* It is usualloy hooked up with a special symbol "d", which simulates vxWorks
* command. So the full command syntax is like:
*   shell> d [address] [size]
* or
*   shell> d [symbol] [size]
* All the arguments can be omitted after the first use. There is a set internal
* static variables to track the previous command arguments, and automatically
* adding the address with size for the next use if no argument is provided
* in the next call. But there is a restriction to such a convenient feature,
* see next item.
* 2. This function is not thread-safe, which means if multiple clients use
* this service at the same time, the auto-filled start pointer and size info
* will be screwed up with unexpected value, but there should be no other side
* effect even in such a case. Since this is debugging function, such behavior
* should be understandable/reasonable/tolerable and if multiple client support
* is required, the users should use full command instead of the simplified
* version.
******************************************************************************/
#define PDI_MAX_READ_CHARS  (4096)  /* 4K MAX allowed to prevent abusing */
void pdi_mem_show( intptr_t sym, size_t size )
{
    static char   * prev_addr = NULL;
    static size_t   prev_bytes = 0x80;
    char          * addr;
    size_t          bytes;

    bytes = (size) ? size : prev_bytes;
    if (bytes > PDI_MAX_READ_CHARS) {
        printf("Max displaying bytes are: %u\n", PDI_MAX_READ_CHARS);
        return;
    }

    if (sym == 0) {
        if (! prev_addr) {
            printf("Invalid address input!\n");
            return;
        } else {
            addr = prev_addr + bytes;
        }
    } else {
#if 0
    /* there is a bug in pdi_sym_find_by_name() that if an invalid number 
     * passed in, it will kill the session, so the app. Leave this piece of
     * code here in case the bug is resovled so we can re-enable the symbol
     * memory show control.
     */
        /* try global symbol first */
        if (pdi_sym_find_by_name(pdi_sym_tbl_id, (char *)sym, & addr, NULL) == PDI_OK) {
            /* found a matched symbol with address filled in addr */
            printf("Symbol %s has address: %p\n", (char *)sym, addr);
        } else {
            /* let's try hardcoded address */
            if (! addr) {
                printf("NULL pointer assigned!\n");
                return;
            }
        }
#else
    addr = (char *) sym;
#endif
    }

    /* now identify if the address is read accessible */
    if (! _is_mem_read_accessible(addr, bytes)) {
        printf("Invalid address: %p (size = 0x%X)\n", addr, (unsigned int)bytes);
        return;
    }

    /* now we can display the memory piece */
    _mem_show((unsigned char *)addr, bytes);

    /* keep the static variables for next time repeat command */
    prev_addr = addr;
    prev_bytes = bytes;
}

