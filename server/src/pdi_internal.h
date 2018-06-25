/******************************************************************************
* Copyright (c) 2008 Redback Networks, Inc. All rights reserved.
* This software is the confidential and proprietary information of
* Redback Networks Inc.
*
* Description:
*
* The process debug infrastructure (PDI) internal header.
******************************************************************************/

#ifndef __PDI_INTERNAL_H__
#define __PDI_INTERNAL_H__

#include "pdi.h"

#define X_symLib                                (28 << 16)
#define M_symLib                                X_symLib

/* status codes */

#define S_symLib_SYMBOL_NOT_FOUND	(M_symLib | 1)
#define S_symLib_NAME_CLASH		(M_symLib | 2)
#define S_symLib_TABLE_NOT_EMPTY	(M_symLib | 3)
#define S_symLib_SYMBOL_STILL_IN_TABLE	(M_symLib | 4)
#define S_symLib_INVALID_SYMTAB_ID	(M_symLib | 12)
#define S_symLib_INVALID_SYM_ID_PTR     (M_symLib | 13)

#define  MAX_SYS_SYM_LEN  (256)

#if !defined (FALSE) /* || (FALSE!=0) */
#define FALSE (0 == 1)
#endif

#if !defined (TRUE) /* || (TRUE!=1) */
#define TRUE  (1 == 1)
#endif

#ifndef BOOL
typedef int    BOOL;
#endif

typedef int             STATUS;

#ifdef __cplusplus
typedef intptr_t (*FUNCPTR)(...);   /* ptr to func returning pointer sized int   */
typedef void   (*VOIDFUNCPTR)(...); /* ptr to func returning void  */
typedef double (*DBLFUNCPTR)(...); /* ptr to func returning double*/
typedef float  (*FLTFUNCPTR)(...); /* ptr to func returning float */
#else
typedef intptr_t (*FUNCPTR)();      /* ptr to func returning pointer sized int   */
typedef void   (*VOIDFUNCPTR)();   /* ptr to func returning void  */
typedef double (*DBLFUNCPTR)();    /* ptr to func returning double*/
typedef float  (*FLTFUNCPTR)();    /* ptr to func returning float */
#endif

#define NELEMENTS(array)               (sizeof (array) / sizeof ((array) [0]))

#define errnoGet() errno
#define errnoSet(errCode) errno = (errCode)

#endif /* __PDI_INTERNAL_H__ */

