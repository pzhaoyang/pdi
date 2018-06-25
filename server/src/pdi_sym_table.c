/******************************************************************************
* Copyright (c) 2011 Ericsson AB.
* Copyright (c) 2008 Redback Networks, Inc. All rights reserved.
* This software is the confidential and proprietary information of
* Redback Networks Inc.
*
* Description:
*
* The symbol table for use in debug infrastructure.
******************************************************************************/
#include "errno.h"
#include "pdi_sem.h"
#include "pdi_sym_table.h"
#include "pdi_demangler.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* globals */

int mutexOptions = PTHREAD_MUTEX_RECURSIVE;

uint16    pdi_sym_group_default = 0;
FUNCPTR   pdi_sync_sym_add_rtn = (FUNCPTR) NULL;
FUNCPTR   pdi_sync_sym_remove_rtn = (FUNCPTR) NULL;

SYMTAB    pdi_sym_tbl;
SYMTAB_ID pdi_sym_tbl_id;                /* system symbol table */
pdi_hash_tbl_t  pdi_hash_tbl;
pdi_sl_list_t   pdi_hash_list[256]; /* [1 << SYM_TBL_HASH_SIZE_LOG2] */

char *pdi_typeName [] =       /* system symbol table types */
{
    "????",
    "abs",
    "text",
    "data",
    "bss",
};

#define SYM_HFUNC_SEED  1370364821      /* magic seed */

typedef struct      /* RTN_DESC - routine descriptor */ {
    FUNCPTR  routine;    /* user routine passed to pdi_sym_each() */
    intptr_t routineArg; /* user routine arg passed to pdi_sym_each() */
} RTN_DESC;

/******************************************************************************
* FUNCTION: symHFuncName - symbol name hash function
*
* RETURNS: An integer between 0 and (elements - 1).
******************************************************************************/
static int symHFuncName( int elements, SYMBOL *pSymbol, int seed )
{
    const char *tkey;
    int  key = 0;

    /* checksum the string and use a multiplicative hashing function */

    for (tkey = pSymbol->name; *tkey != '\0'; tkey++)
        key = key + (unsigned int) *tkey;

    return(abs(key * seed) % elements);
}

/******************************************************************************
* FUNCTION: symKeyCmpName - compare two symbol's names for equivalence
*
* RETURNS: TRUE if symbols match, FALSE if they differ.
******************************************************************************/
static BOOL symKeyCmpName( SYMBOL *pMatchSymbol, SYMBOL *pSymbol, int maskArg )
{
    SYM_TYPE    mask;                   /* symbol type bits than matter (char)*/

    /*
     * If maskArg is equal to SYM_MASK_EXACT, then check to see if the pointers
     * match exactly.
     */

    if (maskArg == SYM_MASK_EXACT)
        return(pMatchSymbol == pSymbol ? TRUE : FALSE);

    mask = (SYM_TYPE) maskArg;
    return(((pSymbol->type & mask) == (pMatchSymbol->type & mask)) &&
           (strcmp (pMatchSymbol->name, pSymbol->name) == 0));
}

/******************************************************************************
* FUNCTION: pdi_sym_tbl_create - create a symbol table
*
* RETURNS: Symbol table ID, or NULL if memory is insufficient.
******************************************************************************/
SYMTAB_ID pdi_sym_tbl_create( int hashSizeLog2, BOOL sameNameOk )
{
    SYMTAB_ID symTblId = &pdi_sym_tbl;

    if (symTblId != NULL) {
        symTblId->nameHashId = pdi_hash_tbl_create (&pdi_hash_tbl,
                                              pdi_hash_list,
                                              hashSizeLog2,
                                              (FUNCPTR) symKeyCmpName,
                                              (FUNCPTR) symHFuncName,
                                              SYM_HFUNC_SEED);

        if (symTblId->nameHashId == NULL) {   /* pdi_hash_tbl_create failed? */
            return(NULL);
        }

        if (pdi_sym_tbl_init (symTblId, sameNameOk, symTblId->nameHashId) != PDI_OK) {
            return(NULL);
        }
    }

    return(symTblId);              /* return the symbol table ID */
}

/******************************************************************************
* FUNCTION: pdi_sym_tbl_init - initialize a symbol table
*
* RETURNS: PDI_OK, or PDI_ERROR if initialization failed.
******************************************************************************/
STATUS pdi_sym_tbl_init(SYMTAB *pSymTbl, BOOL sameNameOk, pdi_hash_id_t symHashTblId )
{
    if (pdi_sem_create(&pSymTbl->symMutex, mutexOptions) == PDI_ERROR) {
        return(PDI_ERROR);
    }

    pSymTbl->sameNameOk = sameNameOk;       /* name clash policy */
    pSymTbl->nsymbols   = 0;                /* initial number of syms */
    pSymTbl->nameHashId = symHashTblId;     /* fill in hash table ID */

    return(PDI_OK);
}

/******************************************************************************
* FUNCTION: pdi_sym_alloc - allocate and initialize a symbol, including group number
*
* RETURNS: Pointer to symbol, or NULL if out of memory.
******************************************************************************/
SYMBOL *pdi_sym_alloc( SYMTAB_ID   symTblId UNUSED,
                       char        *name,
                       char        *value,
                       SYM_TYPE    type,
                       uint16  group )
{
    SYMBOL *pSymbol;
    char   *pdi_sym_name;
    int    length;

    if (name == NULL)
        return(NULL);              /* null name */

    length = strlen (name);         /* figure out name length */

    pSymbol = (SYMBOL *) malloc ((sizeof(SYMBOL) + length + 1));

    if (pSymbol == NULL)            /* out of memory */
        return(NULL);

    /* copy name after symbol */

    pdi_sym_name = (char *)(pSymbol + 1 /* in this case sizeof (SYMBOL)*/);
    pdi_sym_name[length] = EOS;          /* null terminate string */

    strncpy (pdi_sym_name, name, length);        /* copy name into place */

    pdi_sym_init (pSymbol, pdi_sym_name, value, type, group); /* initialize symbol*/

    return(pSymbol);               /* return symbol ID */
}

/******************************************************************************
* FUNCTION: pdi_sym_init - initialize a symbol, including group number
*
* RETURNS: PDI_OK, or PDI_ERROR if symbol table could not be initialized.
******************************************************************************/
STATUS pdi_sym_init( SYMBOL      *pSymbol,
                char        *name,
                char        *value,
                SYM_TYPE    type,
                uint16  group )
{
    /* fill in symbol */

    pSymbol->name  = name;          /* symbol name */
    pSymbol->value = (void *)value;     /* symbol value */
    pSymbol->type  = type;          /* symbol type */
    pSymbol->group = group;         /* symbol group */

    return(PDI_OK);
}

/******************************************************************************
* FUNCTION: pdi_sym_free - deallocate a symbol
*
* RETURNS: PDI_OK, or PDI_ERROR if invalid symbol table.
******************************************************************************/
STATUS pdi_sym_free( SYMTAB_ID symTblId UNUSED, SYMBOL *pSymbol )
{
    free ( (void *) pSymbol);
    return(PDI_OK);
}

/******************************************************************************
* FUNCTION: symSAdd - create and add a symbol to a symbol table, including a group number
*
* This routine behaves as pdi_sym_add() except it does not check for synchronization
* function pointers. Thus it can be used in loaders to prevent from trying to
* independently synchronize each symbol of a module.
*
* RETURNS: PDI_OK, or PDI_ERROR if the symbol table is invalid
* or there is insufficient memory for the symbol to be allocated.
******************************************************************************/
STATUS symSAdd( SYMTAB_ID symTblId,
                char      *name,
                char      *value,
                SYM_TYPE  type,
                uint16    group )
{
    SYMBOL *pSymbol = pdi_sym_alloc (symTblId, name, value, type, group);

    if (pSymbol == NULL)            /* out of memory? */
        return(PDI_ERROR);

    if (pdi_sym_tbl_add (symTblId, pSymbol) != PDI_OK) {    /* try to add symbol */
        pdi_sym_free (symTblId, pSymbol);        /* deallocate symbol if fail */
        return(PDI_ERROR);
    }

    return(PDI_OK);
}

/******************************************************************************
* FUNCTION: pdi_sym_add - create and add a symbol to a symbol table, including a group number
*
* RETURNS: PDI_OK, or PDI_ERROR if the symbol table is invalid
* or there is insufficient memory for the symbol to be allocated.
******************************************************************************/
STATUS pdi_sym_add( SYMTAB_ID symTblId,
               char      *name,
               char      *value,
               SYM_TYPE  type,
               uint16    group )
{
    SYMBOL *pSymbol = pdi_sym_alloc (symTblId, name, value, type, group);

    if (pSymbol == NULL)            /* out of memory? */
        return(PDI_ERROR);

    if (pdi_sym_tbl_add (symTblId, pSymbol) != PDI_OK) {    /* try to add symbol */
        pdi_sym_free (symTblId, pSymbol);        /* deallocate symbol if fail */
        return(PDI_ERROR);
    }

    /* synchronize host symbol table if necessary */
    if ((pdi_sync_sym_add_rtn != NULL) && (symTblId == pdi_sym_tbl_id))
        (* pdi_sync_sym_add_rtn) (name, value, type, group);

    return(PDI_OK);
}

/******************************************************************************
* FUNCTION: pdi_sym_tbl_add - add a symbol to a symbol table
*
* RETURNS: PDI_OK, or PDI_ERROR if invalid symbol table, or symbol couldn't be added.
******************************************************************************/
STATUS pdi_sym_tbl_add( SYMTAB_ID symTblId, SYMBOL *pSymbol )
{
    pdi_sem_take (&symTblId->symMutex);

    if ((!symTblId->sameNameOk) &&
        (pdi_hash_tbl_find (symTblId->nameHashId, &pSymbol->hash_node,
                      SYM_MASK_EXACT_TYPE) != NULL)) {
        pdi_sem_give (&symTblId->symMutex);       /* release exclusion to table */

        errno = S_symLib_NAME_CLASH;        /* name clashed */
        return(PDI_ERROR);
    }

    /* put the symbol in the table */
    pdi_hash_tbl_put (symTblId->nameHashId, &pSymbol->hash_node);

    /* increment symbol count */
    symTblId->nsymbols ++;          

    /* release exclusion to table */
    pdi_sem_give (&symTblId->symMutex);       

    return(PDI_OK);
}

/******************************************************************************
* FUNCTION: pdi_sym_remove - remove a symbol from a symbol table
*
* RETURNS: PDI_OK, or PDI_ERROR if the symbol is not found
* or could not be deallocated.
******************************************************************************/
STATUS pdi_sym_remove( SYMTAB_ID symTblId, char *name, SYM_TYPE  type )
{
    SYMBOL *pSymbol;

    if (pdi_sym_find_symbol (symTblId, name, NULL,
                       type, SYM_MASK_EXACT_TYPE, &pSymbol) != PDI_OK)
        return(PDI_ERROR);

    if (pdi_sym_tbl_remove (symTblId, pSymbol) != PDI_OK)
        return(PDI_ERROR);

    /* synchronize host symbol table if necessary */

    if ((pdi_sync_sym_remove_rtn != NULL) && (symTblId == pdi_sym_tbl_id))
        (* pdi_sync_sym_remove_rtn) (name, type);

    return(pdi_sym_free (symTblId, pSymbol));
}

/******************************************************************************
* FUNCTION: pdi_sym_tbl_remove - remove and terminate a symbol from a symbol table
*
* RETURNS: PDI_OK, or PDI_ERROR if symbol table invalid, or symbol not found.
******************************************************************************/
STATUS pdi_sym_tbl_remove( SYMTAB_ID symTblId, SYMBOL *pSymbol )
{
    pdi_hash_node_t *node_p;

    pdi_sem_take (&symTblId->symMutex);

    node_p = pdi_hash_tbl_find (symTblId->nameHashId, &pSymbol->hash_node,
                         SYM_MASK_EXACT);

    if (node_p == NULL) {
        pdi_sem_give (&symTblId->symMutex);       /* release exclusion to table */

        errnoSet (S_symLib_SYMBOL_NOT_FOUND);   /* symbol wasn't found */
        return(PDI_ERROR);
    }

    pdi_hash_tbl_remove (symTblId->nameHashId, node_p);

    symTblId->nsymbols--;           /* one less symbol */

    pdi_sem_give (&symTblId->symMutex);       /* release exclusion to table */

    return(PDI_OK);
}

/******************************************************************************
* FUNCTION: pdi_sym_find_symbol - find symbol in a symbol table of equivalent name and type
*
* RETURNS: PDI_OK, or PDI_ERROR if <symTblId> is invalid, <pSymbolId> is NULL,
*          symbol not found (for a search by name), or if <value> is less than
*          the lowest value in the table (for search by value).
******************************************************************************/
STATUS pdi_sym_find_symbol( SYMTAB_ID   symTblId,
                      char *      name,
                      void *      value,
                      SYM_TYPE    type,
                      SYM_TYPE    mask,
                      SYMBOL_ID * pSymbolId )
{
    pdi_hash_node_t *         node_p;      /* node in symbol hash table */
    SYMBOL              keySymbol;  /* dummy symbol for search by name */
    int                 index;      /* counter for search by value */
    SYMBOL *            pSymbol;    /* current symbol, search by value */
    SYMBOL *            pBestSymbol = NULL;
    /* symbol with lower value, matching type */
    char *      pUnder;     /* string for _text, etc., check */
    void *      bestValue = NULL;
        /* current value of symbol with matching type */

    if (symTblId == NULL) {
        errnoSet (S_symLib_INVALID_SYMTAB_ID);
        return(PDI_ERROR);
    }

    if (pSymbolId == NULL) {
        errnoSet (S_symLib_INVALID_SYM_ID_PTR);
        return(PDI_ERROR);
    }

    if (name != NULL) {
        /* Search by name or by name and type: */

        /* fill in keySymbol */

        keySymbol.name = name;          /* match this name */
        keySymbol.type = type;          /* match this type */

        pdi_sem_take (&symTblId->symMutex);

        node_p = pdi_hash_tbl_find (symTblId->nameHashId, &keySymbol.hash_node,
                             (int)mask);

        pdi_sem_give (&symTblId->symMutex);       /* release exclusion to table */

        if (node_p == NULL) {
            errnoSet (S_symLib_SYMBOL_NOT_FOUND);    /* couldn't find symbol */
            return(PDI_ERROR);
        }

        *pSymbolId = (SYMBOL_ID) node_p;

    } else {
        /* Search by value or by value and type: */

        pdi_sem_take (&symTblId->symMutex);

        for (index = 0; index < symTblId->nameHashId->elements; index++) {
            pSymbol =
            (SYMBOL *) SLL_FIRST(&symTblId->nameHashId->hash_tbl_p [index]);

            while (pSymbol != NULL) {         /* list empty */
                if (((pSymbol->type & mask) == (type & mask)) &&
                    (pSymbol->value == value) &&
                    (((pUnder = rindex (pSymbol->name, '_')) == NULL) ||
                     ((strcmp (pUnder, "_text") != 0) &&
                      (strcmp (pUnder, "_data") != 0) &&
                      (strcmp (pUnder, "_bss") != 0) &&
                      (strcmp (pUnder, "_compiled.") != 0))) &&
                    (((pUnder = rindex (pSymbol->name, '.')) == NULL) ||
                     ((strcmp (pUnder, ".o") != 0)))) {
                    /* We've found the entry.  Return it. */

                    *pSymbolId = pSymbol;

                    pdi_sem_give (&symTblId->symMutex);

                    return(PDI_OK);
                }

                else if (((pSymbol->type & mask) == (type & mask)) &&
                         ((pSymbol->value <= value) &&
                          (pSymbol->value > bestValue))) {
                    /*
                     * this symbol is of correct type and closer than last one
                     */

                    bestValue   = pSymbol->value;
                    pBestSymbol = pSymbol;
                }

                pSymbol = (SYMBOL *) SLL_NEXT (&pSymbol->hash_node);
            }
        }

        if (bestValue == NULL || pBestSymbol == NULL) {   /* any closer symbol? */
            pdi_sem_give (&symTblId->symMutex);   /* release exclusion to table */

            errnoSet (S_symLib_SYMBOL_NOT_FOUND);
            return(PDI_ERROR);
        }

        *pSymbolId = pBestSymbol;

        pdi_sem_give (&symTblId->symMutex);       /* release exclusion to table */
    }

    return(PDI_OK);
}

/******************************************************************************
* FUNCTION: pdi_sym_find_by_name - look up a symbol by name
*
* RETURNS: PDI_OK, or PDI_ERROR if the symbol table ID is invalid
* or the symbol cannot be found.
******************************************************************************/
STATUS pdi_sym_find_by_name( SYMTAB_ID symTblId,
                      char        *name,
                      char        **pValue,
                      SYM_TYPE    *pType )
{
    return(pdi_sym_find_by_name_and_type (symTblId, name, pValue, pType,
                                 SYM_MASK_ANY_TYPE, SYM_MASK_ANY_TYPE));
}

#define KHEAP_ALLOC(nBytes)                                             \
        malloc((nBytes))
#define KHEAP_FREE(pChar)                                               \
        free((pChar))

/******************************************************************************
* FUNCTION: symByCNameFind - find a symbol in a symbol table
*
* RETURNS: PDI_OK, or PDI_ERROR if <symTblId> is invalid or symbol is not found.
******************************************************************************/
STATUS symByCNameFind( SYMTAB_ID   symTblId,
                       char *  name,
                       char ** pValue,
                       SYM_TYPE *  pType )
{
    char *  symBuf = NULL;
    STATUS  retVal;

    if (pdi_sym_find_by_name (symTblId, name, pValue, pType) == PDI_ERROR) {

        /* prepend a '_' and try again */
        if ((symBuf = (char *) KHEAP_ALLOC (strlen (name) + 2)) == NULL)
            return PDI_ERROR;

        *symBuf = '_';
        strcpy (&symBuf[1], name);

        retVal = pdi_sym_find_by_name (symTblId, symBuf, pValue, pType);

        KHEAP_FREE (symBuf);

        return(retVal);
    }

    return(PDI_OK);
}

/******************************************************************************
* FUNCTION: pdi_sym_find_by_cname - find a symbol in a symbol table
*
* RETURNS: PDI_OK, or PDI_ERROR if <symTblId> is invalid or symbol is not found.
******************************************************************************/
STATUS pdi_sym_find_by_cname( SYMTAB_ID   symTblId,
                       char        *name,
                       char        **pValue,
                       SYM_TYPE    *pType )
{
    return symByCNameFind (symTblId, name, pValue, pType);
}

/******************************************************************************
* FUNCTION: pdi_sym_find_by_name_and_type - look up a symbol by name and type
*
* RETURNS: PDI_OK, or PDI_ERROR if the symbol table ID is invalid
* or the symbol is not found.
******************************************************************************/
STATUS pdi_sym_find_by_name_and_type( SYMTAB_ID   symTblId,
                             char        *name,
                             char        **pValue,
                             SYM_TYPE    *pType,
                             SYM_TYPE    sType,
                             SYM_TYPE    mask )
{
    SYMBOL  *pSymbol = NULL;

    if (pdi_sym_find_symbol (symTblId, name, NULL, sType,
                       mask, &pSymbol) == PDI_ERROR)
        return(PDI_ERROR);

    if (pValue != NULL)
        *pValue = (char *) pSymbol->value;

    if (pType != NULL)
        *pType = pSymbol->type;

    return PDI_OK;
}

/******************************************************************************
* FUNCTION: pdi_sym_by_value_find - look up a symbol by value
*
* RETURNS: PDI_OK or PDI_ERROR if <symTblId> is invalid, <pName> is NULL, or
* <value> is less than the lowest value in the table.
******************************************************************************/
STATUS pdi_sym_by_value_find( SYMTAB_ID symTblId,
                       uint32    value,
                       char ** pName,
                       intptr_t *  pValue,
                       SYM_TYPE *  pType )
{
    return(pdi_sym_by_value_and_type_find (symTblId, value, pName, pValue, pType,
                                  SYM_MASK_ANY_TYPE, SYM_MASK_ANY_TYPE));
}

/******************************************************************************
* FUNCTION: pdi_sym_by_value_and_type_find - look up a symbol by value and type
*
* RETURNS: PDI_OK or PDI_ERROR if <symTblId> is invalid, <pName> is NULL, or
* <value> is less than the lowest value in the table.
******************************************************************************/
STATUS pdi_sym_by_value_and_type_find( SYMTAB_ID symTblId,
                              uint32        value,
                              char **     pName,
                              intptr_t *      pValue,
                              SYM_TYPE *      pType,
                              SYM_TYPE        sType,
                              SYM_TYPE        mask )
{
    SYMBOL *    pSymbol = NULL;

    if (pName == NULL)
        return PDI_ERROR;

    if (pdi_sym_find_symbol(symTblId, NULL, (void *)(uintptr_t)value, sType,
                      mask, &pSymbol) != PDI_OK)
        return PDI_ERROR;

    if (pValue != NULL)
        *pValue = (intptr_t) pSymbol->value;

    if (pType != NULL)
        *pType = pSymbol->type;

    if (((*pName) = (char *) malloc (strlen (pSymbol->name) + 1)) == NULL)
        return PDI_ERROR;

    strcpy ((*pName), pSymbol->name);

    return(PDI_OK);
}

/******************************************************************************
* FUNCTION: pdi_sym_find_by_value - look up a symbol by value
*
* RETURNS: PDI_OK, or PDI_ERROR if <symTblId> is invalid or <value> is less
* than the lowest value in the table.
******************************************************************************/
STATUS pdi_sym_find_by_value( SYMTAB_ID   symTblId,
                       uint32   value,
                       char *  name,
                       intptr_t *  pValue,
                       SYM_TYPE *  pType )
{
    return(pdi_sym_find_by_value_and_type (symTblId, value, name, pValue, pType,
                                  SYM_MASK_ANY_TYPE, SYM_MASK_ANY_TYPE));
}

/******************************************************************************
* FUNCTION: pdi_sym_find_by_value_and_type - look up a symbol by value and type
*
* RETURNS: PDI_OK, or PDI_ERROR if <symTblId> is invalid or <value> is less
* than the lowest value in the table.
******************************************************************************/
STATUS pdi_sym_find_by_value_and_type( SYMTAB_ID symTblId,
                              uint32     value,
                              char *        name,
                              intptr_t *    pValue,
                              SYM_TYPE *    pType,
                              SYM_TYPE      sType,
                              SYM_TYPE      mask )
{
    SYMBOL * pSymbol = NULL;

    if (pdi_sym_find_symbol (symTblId, NULL, (void *)(uintptr_t)value, sType,
                       mask, &pSymbol) != PDI_OK) {

        return PDI_ERROR;
    }

    strncpy (name, pSymbol->name, MAX_SYS_SYM_LEN + 1);

    /* Null-terminate the string in case the name was truncated. */

    if (name[MAX_SYS_SYM_LEN] != EOS)
        name[MAX_SYS_SYM_LEN] = EOS;

    if (pValue != NULL)
        *pValue = (intptr_t) pSymbol->value;

    if (pType != NULL)
        *pType = pSymbol->type;

    return(PDI_OK);
}

/******************************************************************************
* FUNCTION: symEachRtn - call a user routine for a hashed symbol
*
* RETURNS: Boolean result of user specified pdi_sym_each routine.
******************************************************************************/
static BOOL symEachRtn( SYMBOL *pSymbol, RTN_DESC *pRtnDesc)
{
    return((* pRtnDesc->routine) (pSymbol->name, (intptr_t) pSymbol->value,
                                  pSymbol->type, pRtnDesc->routineArg,
                                  pSymbol->group, pSymbol));
}

/******************************************************************************
* FUNCTION: pdi_sym_each - call a routine to examine each entry in a symbol table
*
* RETURNS: A pointer to the last symbol reached, or NULL if all symbols are reached.
******************************************************************************/
SYMBOL *pdi_sym_each( SYMTAB_ID symTblId, FUNCPTR routine, intptr_t routineArg )
{
    SYMBOL   *pSymbol;
    RTN_DESC rtnDesc;

    /* fill in a routine descriptor with the routine and argument to call */

    rtnDesc.routine    = routine;
    rtnDesc.routineArg = routineArg;

    pdi_sem_take (&symTblId->symMutex);

    pSymbol = (SYMBOL *) pdi_hash_tbl_each (symTblId->nameHashId, (FUNCPTR)symEachRtn,
                                      (intptr_t) &rtnDesc);

    pdi_sem_give (&symTblId->symMutex);       /* release exclusion to table */

    return(pSymbol);               /* symbol we stopped on */
}

/******************************************************************************
* FUNCTION: symNameValueCmp - compares a symbol value with a name
*
* RETURNS: FALSE if <val> matches <pSym>->value and sets <pSym>->name,
* or TRUE otherwise.
******************************************************************************/
static BOOL symNameValueCmp( char *name, intptr_t val, SYM_TYPE type UNUSED, intptr_t pSym )
{
    if (val == (intptr_t)(((SYMBOL *)pSym)->value)) {
        ((SYMBOL *)pSym)->name = name;
        return(FALSE);
    }
    return(TRUE);
}

/******************************************************************************
* FUNCTION: pdi_sym_name - get the name associated with a symbol value
*
* RETURNS: A pointer to the symbol name, or
* NULL if symbol cannot be found or <symTbl> doesn't exist.
******************************************************************************/
const char *pdi_sym_name( SYMTAB_ID symTbl, char *value)
{
    SYMBOL sym;

    sym.value = (void *)value;                  /* initialize symbol */
    sym.name  = NULL;

    (void)pdi_sym_each (symTbl, (FUNCPTR) symNameValueCmp, (intptr_t) (&sym));

    return(sym.name);
}

/******************************************************************************
* FUNCTION: pdi_sym_name_get - get name of a symbol
*
* RETURNS: PDI_OK, or PDI_ERROR if either <pName> or <symbolId> is NULL.
******************************************************************************/
STATUS pdi_sym_name_get( SYMBOL_ID symbolId, const char ** pName)
{
    if ((symbolId == NULL) || (pName == NULL))
        return PDI_ERROR;

    *pName = symbolId->name;

    return PDI_OK;
}

/******************************************************************************
* FUNCTION: pdi_sym_value_get - get value of a symbol
*
* RETURNS: PDI_OK, or PDI_ERROR if either <pValue> or <symbolId> is NULL.
******************************************************************************/
STATUS pdi_sym_value_get( SYMBOL_ID symbolId, void ** pValue )
{
    if ((symbolId == NULL) || (pValue == NULL))
        return PDI_ERROR;

    *pValue = symbolId->value;

    return PDI_OK;
}

/******************************************************************************
* FUNCTION: symTypeGet - get type of a symbol
*
* RETURNS: PDI_OK, or PDI_ERROR if either <pType> or <symbolId> is NULL.
******************************************************************************/
STATUS symTypeGet(SYMBOL_ID  symbolId, SYM_TYPE * pType)
{
    if ((symbolId == NULL) || (pType == NULL))
        return PDI_ERROR;

    *pType = symbolId->type;

    return PDI_OK;
}

/******************************************************************************
* FUNCTION: strMatch - find an occurrence of a string in another string
*
* RETURNS: A pointer to the occurrence or 0 if it doesn't find one.
******************************************************************************/
static char *strMatch( FAST char *str1, FAST char *str2 )
{
    FAST int str2Length = strlen (str2);
    FAST int ntries = strlen (str1) - str2Length;

    for (; ntries >= 0; str1++, --ntries) {
        if (strncmp (str1, str2, str2Length) == 0)
            return(str1);  /* we've found a match */
    }

    return(NULL);
}

/******************************************************************************
* FUNCTION: symPrint - prints the symbol if the symbol's name contains <substr>
*
* RETURNS: TRUE.
******************************************************************************/
static BOOL symPrint( char * name, int val, int8 type UNUSED, char * substr )
{
    char         demangled [MAX_SYS_SYM_LEN + 1];
    const char * nameToPrint;

    if (substr == NULL || strMatch (name, substr) != NULL) {
        nameToPrint = pdi_cplus_demangle (name, demangled, MAX_SYS_SYM_LEN + 1, COMPLETE);
        printf ("%-40s 0x%08x\n", nameToPrint, val);
    }

    return(TRUE);
}

/******************************************************************************
* FUNCTION: pdi_sym_show - show the symbols of specified symbol table with matching substring
*
* RETURNS: PDI_OK, or PDI_ERROR if invalid symbol table id.
******************************************************************************/
STATUS pdi_sym_show( SYMTAB * pSymTbl, char * substr)
{
    if (substr == NULL) {
        printf ("%-20s: %-10d\n", "Number of Symbols", pSymTbl->nsymbols);
        printf ("%-20s: %-10p\n", "Symbol Mutex Ptr", &pSymTbl->symMutex);
        printf ("%-20s: %-10p\n", "Symbol Hash Id", pSymTbl->nameHashId);
        printf ("%-20s: %-10s\n", "Name Clash Policy",
                (pSymTbl->sameNameOk) ? "Allowed" : "Disallowed");
    } else {
        pdi_sym_each (pSymTbl, (FUNCPTR) symPrint, (intptr_t) substr);
    }

    return(PDI_OK);
}


