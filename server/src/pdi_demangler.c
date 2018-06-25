/******************************************************************************
 * Copyright (c) 2011 Ericsson AB.
 * Copyright (c) 2008 Redback Networks, Inc.
 * This software is the confidential and proprietary information of
 * Redback Networks Inc.
 *
 * Description:
 *
 * Provides a demangling implementation.
******************************************************************************/

/* includes */
#include "pdi_demangler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_OVERLOAD_SYMS 50

/* typedefs */
struct manglingStyles {
    char * name;
    pdi_demangler_style_p style;
};

struct manglingStyles manglingStyles[] =
{
    {"gnu", DMGL_STYLE_GNU},
    {"diab", DMGL_STYLE_DIAB},
    {"arm", DMGL_STYLE_ARM},
};

/* globals */
pdi_demangler_style_p cplusDemanglerStyle = DMGL_STYLE_GNU;

/* locals */
static char * overloadMatches [MAX_OVERLOAD_SYMS];
static int overloadMatchCount;

/******************************************************************************
* FUNCTION: symbolStartOf - skip leading underscore
*
* RETURNS: pointer to the start of the symbol name after any compiler 
*          prepended leading underscore.
******************************************************************************/
static const char *symbolStartOf( const char *str )
{
    if (CPU_FAMILY_PREPENDS_UNDERSCORE && (str [0] == '_'))
        return str + 1;
    else
        return str;
}

/******************************************************************************
* FUNCTION: askUser - ask user to choose among overloaded name alternatives
*
* This routine is used by pdi_cplus_match_mangled when a name is overloaded.
*
* RETURNS: index into overloadMatches, or negative if no symbol is selected
******************************************************************************/
static int askUser( char *name )
{
    char              demangled [MAX_SYS_SYM_LEN + 1];
    char              userInput [10];
    const char          * nameToPrint;
    int               choice;
    int               i;

    do {
        printf ("\"%s\" is overloaded - Please select:\n", name);
        for (i = 0; i < overloadMatchCount; i++) {
            nameToPrint = pdi_cplus_demangle (overloadMatches [i], demangled,
                                         MAX_SYS_SYM_LEN + 1, COMPLETE);
            printf ("  %3d: %s\n", i+1, nameToPrint);
        }
        printf ("Enter <number> to select, anything else to stop: ");
        fgets (userInput, 10, stdin);
        choice = atoi (userInput) - 1;
    }
    while (choice >= overloadMatchCount);

    return choice;
}

/******************************************************************************
* FUNCTION: findMatches - find (possibly overloaded) symbols that match goal string
*
* RETURNS: TRUE
******************************************************************************/
static BOOL findMatches( char *name, int dummy1 UNUSED, SYM_TYPE dummy2 UNUSED, char * string )
{
    const char * result;
    const char * goalString = string;
    char         demangled [MAX_SYS_SYM_LEN + 1];

    /* demangle the symbol table entry */
    result = pdi_cplus_demangle(name,demangled,MAX_SYS_SYM_LEN + 1,TERSE);

    /* compare the demangled name to the goal string */
    if (!strncmp(result,goalString,MAX_SYS_SYM_LEN)) {
        if (overloadMatchCount < MAX_OVERLOAD_SYMS) {
            /* if it matches, add the sybol table entry to the list of options */
            overloadMatches [overloadMatchCount++] = name;
        }
    }

    return TRUE;
}

/******************************************************************************
* FUNCTION: pdi_cplus_match_mangled - match string against mangled symbol table entries
*
* RETURNS: TRUE if a unique match is resolved, otherwise FALSE.
******************************************************************************/
BOOL pdi_cplus_match_mangled( SYMTAB_ID symTab, char *string, SYM_TYPE *pType, int *pValue )
{
    int userChoice;

    overloadMatchCount = 0;
    pdi_sym_each (symTab, (FUNCPTR) findMatches, (intptr_t) string);
    switch (overloadMatchCount) {
    case 0:
        return FALSE;

    case 1:
        return(pdi_sym_find_by_name (symTab, overloadMatches[0],
                              (char **) pValue, pType)
               == PDI_OK);
    default:
        userChoice = askUser (string);
        if (userChoice >= 0) {
            return(pdi_sym_find_by_name (symTab, overloadMatches [userChoice],
                                  (char **)pValue, pType)
                   == PDI_OK);
        } else {
            return FALSE;
        }
    }
}

/******************************************************************************
* FUNCTION: cplusDemanglerStyleSet - change C++ demangling style
*
* RETURNS: none.
******************************************************************************/
void cplusDemanglerStyleSet( pdi_demangler_style_p style )          
{
    cplusDemanglerStyle = style;
}

/******************************************************************************
* FUNCTION: demanglerStyleFromName - given a name return the style
*
* RETURNS: the style enum
******************************************************************************/
pdi_demangler_style_p demanglerStyleFromName(const char * styleName,
                                       pdi_demangler_style_p defaultStyle)
{
    int i;
    for (i = 0; i != ARRAY_SIZE(manglingStyles); ++i) {
        if (strcmp(styleName, manglingStyles[i].name) == 0) {
            return manglingStyles[i].style;
        }
    }
    return defaultStyle;
}

/******************************************************************************
* FUNCTION: demanglerNameFromStyle - given a style return the name
*
* RETURNS: the style name
******************************************************************************/
const char * demanglerNameFromStyle(pdi_demangler_style_p style)
{
    int i;
    for (i = 0; i != ARRAY_SIZE(manglingStyles); ++i) {
        if (style == manglingStyles[i].style) {
            return manglingStyles[i].name;
        }
    }
    return "unknown";
}

/******************************************************************************
* FUNCTION: demangle - decode a C++ mangled name
*
* RETURNS: an allocated string or NULL
******************************************************************************/
char * demangle( const char * mangledSymbol, 
                 CPLUS_DEMANGLER_MODES mode)
{
    int options = 0;

    switch (mode) {
    case DEMANGLER_OFF:
        {
            char * result = (char *)malloc(strlen(mangledSymbol) + 1);
            if (result) {
                strcpy(result, mangledSymbol);
            }
            return result;
        }
    case TERSE:
        options = 0;
        break;
    case COMPLETE:
        options = DMGL_PARAMS | DMGL_ANSI;
        break;
    }
    switch (cplusDemanglerStyle) {
    case DMGL_STYLE_GNU:
        options |= DMGL_AUTO;
        break;
    case DMGL_STYLE_DIAB:
        options |= DMGL_EDG;
        break;
    case DMGL_STYLE_ARM:
        options |= DMGL_ARM;
        break;
    default:
        options |= DMGL_EDG;
        break;   
    }
    return  cplus_demangle (mangledSymbol, options);
}

/******************************************************************************
* FUNCTION: pdi_cplus_demangle - demangle symbol
*
* RETURNS: Destination string if demangling is successful, otherwise 
*           the source string.
******************************************************************************/
const char * pdi_cplus_demangle( const char * source, char * dest, int n, CPLUS_DEMANGLER_MODES mode )
{
    char *buf;

    const char * temp_source = symbolStartOf (source);

    buf = demangle(temp_source, mode);
    if (buf !=0) {
        strncpy (dest, buf, n);
        free (buf);
        return dest;
    } else {
        return temp_source;
    }     
}

