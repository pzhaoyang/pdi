#ifndef __PDI_SYM_TABLE_H__
#define __PDI_SYM_TABLE_H__

#include <pthread.h>

#include "pdi_internal.h"
#include "pdi_hash_table.h"


#define N_TEXT                  0x4
#define N_DATA                  0x6
#define N_BSS                   0x8
#define N_EXT                   01              // external bit, or'ed in
#define	N_TYPE	                0x1e		    // mask for all the type bits

#define SYM_MASK_NONE	        0x00		    // no bits of symbol type valid
#define SYM_MASK_ALL	        0xff           // all bits of symbol type valid
#define SYM_MASK_EXACT	        0x1ff		    // match symbol pointer exactly
#define SYM_SDA_MASK            0xc0           // for SDA and SDA2 symbols

#define SYM_MASK_ANY_TYPE       SYM_MASK_NONE  // ignore type in searches
#define SYM_MASK_EXACT_TYPE     SYM_MASK_ALL   // match type exactly in searches

#define SYM_TBL_HASH_SIZE_LOG2	8	           // 256 entry hash table symbol table

typedef SYMBOL                  *SYMBOL_ID;
typedef SYMTAB                  *SYMTAB_ID;
// SYMTAB - symbol table
typedef struct symtab{	
    pdi_hash_id_t	            nameHashId;	    // hash table for names
    pthread_mutex_t             symMutex;	    // symbol table mutual exclusion sem
    BOOL	                    sameNameOk;	    // symbol table name clash policy
    int		                    nsymbols;	    // current number of symbols in table
} SYMTAB;

extern unsigned short           pdi_sym_group_default;
extern FUNCPTR                  pdi_sync_sym_add_rtn;
extern FUNCPTR                  pdi_sync_sym_remove_rtn;
extern SYMTAB_ID                pdi_sym_tbl_id;
extern SYMTAB                   pdi_sym_tbl;
extern pdi_hash_tbl_t           pdi_hash_tbl;
extern pdi_sl_list_t            pdi_hash_list[];
extern char*                    pdi_typeName[];


SYMBOL *pdi_sym_each (SYMTAB_ID symTblId, FUNCPTR routine, intptr_t routineArg);
const char *pdi_sym_name (SYMTAB_ID symTbl, char *value);
STATUS 	pdi_sym_show (SYMTAB_ID pSymTbl, char *substr);
STATUS 	pdi_sym_free (SYMTAB_ID symTblId, SYMBOL *pSymbol);
STATUS  symSAdd (SYMTAB_ID symTblId, char *name, char *value, SYM_TYPE type, unsigned short group);
STATUS  pdi_sym_add (SYMTAB_ID symTblId, char *name, char *value, SYM_TYPE type, unsigned short group);
STATUS 	pdi_sym_tbl_add (SYMTAB_ID symTblId, SYMBOL *pSymbol);
STATUS 	pdi_sym_remove (SYMTAB_ID symTblId, char *name, SYM_TYPE type);
STATUS 	pdi_sym_tbl_remove (SYMTAB_ID symTblId, SYMBOL *pSymbol);
STATUS 	pdi_sym_init (SYMBOL *pSymbol, char *name, char *value, SYM_TYPE type, unsigned short group);
SYMBOL *pdi_sym_alloc (SYMTAB_ID symTblId, char *name, char *value, SYM_TYPE type, unsigned short group);
STATUS 	pdi_sym_tbl_init (SYMTAB *pSymTbl, BOOL sameNameOk, pdi_hash_id_t symHashTblId);
SYMTAB_ID pdi_sym_tbl_create (int hashSizeLog2, BOOL sameNameOk);
STATUS 	pdi_sym_find_by_name (SYMTAB_ID symTblId, char *name, char **pValue, SYM_TYPE *pType);
STATUS 	pdi_sym_find_by_value (SYMTAB_ID symTblId, unsigned int value, char *name, intptr_t *pValue, SYM_TYPE *pType);
STATUS 	pdi_sym_find_by_cname (SYMTAB_ID symTblId, char *name, char **pValue, SYM_TYPE *pType);
STATUS 	pdi_sym_find_by_name_and_type (SYMTAB_ID symTblId, char *name, char **pValue, SYM_TYPE *pType, SYM_TYPE sType, SYM_TYPE mask);
STATUS 	pdi_sym_find_by_value_and_type (SYMTAB_ID symTblId, unsigned int value, char *name, intptr_t *pValue, SYM_TYPE *pType, SYM_TYPE sType, SYM_TYPE mask);
STATUS 	pdi_sym_by_value_find (SYMTAB_ID symTblId, unsigned int value, char **pName, intptr_t *pValue, SYM_TYPE *pType);
STATUS 	pdi_sym_by_value_and_type_find (SYMTAB_ID symTblId, unsigned int value, char **pName, intptr_t *pValue, SYM_TYPE *pType,SYM_TYPE sType, SYM_TYPE mask);
STATUS pdi_sym_find_symbol( SYMTAB_ID symTblId, char * name, void * value, SYM_TYPE type, SYM_TYPE mask, SYMBOL_ID * pSymbolId );
STATUS symByCNameFind( SYMTAB_ID symTblId, char * name, char ** pValue, SYM_TYPE *	pType );
STATUS pdi_sym_name_get( SYMBOL_ID  symbolId, const char ** pName );
STATUS pdi_sym_value_get( SYMBOL_ID symbolId, void ** pValue );
STATUS symTypeGet( SYMBOL_ID  symbolId, SYM_TYPE * pType );

#endif
