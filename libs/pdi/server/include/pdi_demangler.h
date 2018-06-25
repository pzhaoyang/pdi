#ifndef __DEMANGLER_H__
#define __DEMANGLER_H__

#include <string.h>
#include <stdlib.h>

#include "pdi_internal.h"
#include "pdi_sym_table.h"
#include "libiberty.demangler/demangle.h"

#ifdef __cplusplus
extern "C" {
#endif

/* prepends_underscore should be 1 if the compiler prepends an underscore 
 * to symbol names; otherwise 0.
 */

#define CPU_FAMILY_PREPENDS_UNDERSCORE 0

typedef enum demangler_style {
    DMGL_STYLE_GNU,
    DMGL_STYLE_DIAB,
    DMGL_STYLE_ARM
} pdi_demangler_style_p;

typedef enum {
    DEMANGLER_OFF       = 0,
    TERSE   = 1,
    COMPLETE    = 2
} CPLUS_DEMANGLER_MODES;

BOOL pdi_cplus_match_mangled( SYMTAB_ID symTab, char *string, SYM_TYPE *pType, int *pValue );

const char * pdi_cplus_demangle( const char * source, char * dest, int n, CPLUS_DEMANGLER_MODES mode );

#ifdef __cplusplus
}
#endif

#endif /* __DEMANGLER_H__ */
