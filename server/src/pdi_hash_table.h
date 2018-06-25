/******************************************************************************
 * Copyright (c) 2008 Redback Networks, Inc. All rights reserved.
 * This software is the confidential and proprietary information of
 * Redback Networks Inc.
 *
 * Description:
 *
 * The hash table for use in process debug infrastructure (PDI).
******************************************************************************/

#ifndef __PDI_HASH_TABLE_H__
#define __PDI_HASH_TABLE_H__

#include "pdi_internal.h"
#include "pdi_linked_list.h"

typedef struct 
    {
    int		elements;		/* number of elements in table */
    FUNCPTR	keyCmpRtn;		/* comparator function */
    FUNCPTR	keyRtn;			/* hash function */
    int		keyArg;			/* hash function argument */
    pdi_sl_list_t	*hash_tbl_p;		/* pointer to hash table array */
    } pdi_hash_tbl_t;

typedef SL_NODE pdi_hash_node_t;	/* pdi_hash_node_t */

typedef pdi_hash_tbl_t *pdi_hash_id_t;

pdi_hash_id_t     pdi_hash_tbl_create (pdi_hash_id_t hash_id, pdi_sl_list_t *list_p, int sizeLog2, FUNCPTR keyCmpRtn, FUNCPTR keyRtn, int keyArg);
pdi_hash_node_t * pdi_hash_tbl_each (pdi_hash_id_t hash_id, FUNCPTR routine, intptr_t routineArg);
pdi_hash_node_t * pdi_hash_tbl_find (pdi_hash_id_t hash_id, pdi_hash_node_t *pMatchNode, int keyCmpArg);
STATUS      pdi_hash_tbl_init (pdi_hash_tbl_t *hash_tbl_p, pdi_sl_list_t *pTblMem, int sizeLog2, FUNCPTR keyCmpRtn, FUNCPTR keyRtn, int keyArg);
STATUS      pdi_hash_tbl_put (pdi_hash_id_t hash_id, pdi_hash_node_t *hash_node_p);
STATUS      pdi_hash_tbl_remove (pdi_hash_id_t hash_id, pdi_hash_node_t *hash_node_p);

#endif /* __PDI_HASH_TABLE_H__ */
