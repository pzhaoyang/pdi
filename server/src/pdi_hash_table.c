/******************************************************************************
 * Copyright (c) 2011 Ericsson AB.
 * All rights reserved.
 * Copyright (c) 2008 Redback Networks, Inc.
 * This software is the confidential and proprietary information of
 * Redback Networks Inc.
 *
 * Description:
 *
 * Provides a hash table implementation.
******************************************************************************/

#include "pdi_hash_table.h"
#include <string.h>

/******************************************************************************
* FUNCTION: pdi_hash_tbl_create - create a hash table
*
* RETURNS: pdi_hash_id_t, or NULL if hash table could not be created.
******************************************************************************/
pdi_hash_id_t pdi_hash_tbl_create( pdi_hash_id_t     hash_id,
                       pdi_sl_list_t    *list_p,
                       int         sizeLog2,
                       FUNCPTR     keyCmpRtn,
                       FUNCPTR     keyRtn,
                       int         keyArg )
{
    if (hash_id != NULL)
        pdi_hash_tbl_init (hash_id, list_p, sizeLog2, keyCmpRtn, keyRtn, keyArg);

    return(hash_id);                /* return the hash id */
}

/******************************************************************************
* FUNCTION: pdi_hash_tbl_init - initialize a hash table
*
* RETURNS: PDI_OK
******************************************************************************/
STATUS pdi_hash_tbl_init ( pdi_hash_tbl_t    *hash_tbl_p,
                     pdi_sl_list_t     *pTblMem,
                     int         sizeLog2,
                     FUNCPTR     keyCmpRtn,
                     FUNCPTR     keyRtn,
                     int         keyArg )
{
    FAST int ix;

    hash_tbl_p->elements  = 1 << sizeLog2;    /* store number of elements */
    hash_tbl_p->keyCmpRtn = keyCmpRtn;        /* store comparator routine */
    hash_tbl_p->keyRtn    = keyRtn;       /* store hashing function */
    hash_tbl_p->keyArg    = keyArg;       /* store hashing function arg */
    hash_tbl_p->hash_tbl_p  = pTblMem;

    /* initialize all of the linked list heads in the table */

    for (ix = 0; ix < hash_tbl_p->elements; ix++)
        pdi_sll_init (&hash_tbl_p->hash_tbl_p [ix]);

    return(PDI_OK);
}

/******************************************************************************
* FUNCTION: pdi_hash_tbl_put - put a hash node into the specified hash table
*
* RETURNS: PDI_OK, or PDI_ERROR if hash_id is invalid.
******************************************************************************/
STATUS pdi_hash_tbl_put ( pdi_hash_id_t hash_id, pdi_hash_node_t *hash_node_p )
{
    int     index;

    /* invoke hash table's hashing routine to get index into table */

    index = (* hash_id->keyRtn) (hash_id->elements, hash_node_p, hash_id->keyArg);

    /* add hash node to head of linked list */

    pdi_sll_put_at_head (&hash_id->hash_tbl_p [index], hash_node_p);

    return(PDI_OK);
}

/******************************************************************************
* FUNCTION: pdi_hash_tbl_find - find a hash node that matches the specified key
*
* RETURNS: pointer to pdi_hash_node_t, or NULL if no matching hash node is found.
******************************************************************************/
pdi_hash_node_t *pdi_hash_tbl_find( FAST pdi_hash_id_t hash_id,
                        pdi_hash_node_t    *pMatchNode,
                        int          keyCmpArg )
{
    FAST pdi_hash_node_t *pHNode;
    int             ix;

    /* invoke hash table's hashing routine to get index into table */

    ix = (* hash_id->keyRtn) (hash_id->elements, pMatchNode, hash_id->keyArg);

    /* search linked list for above hash index and return matching hash node */

    pHNode = (pdi_hash_node_t *) SLL_FIRST (&hash_id->hash_tbl_p [ix]);

    while ((pHNode != NULL) &&
           !((* hash_id->keyCmpRtn) (pMatchNode, pHNode, keyCmpArg)))
        pHNode = (pdi_hash_node_t *) SLL_NEXT (pHNode);

    return(pHNode);
}

/******************************************************************************
* FUNCTION: pdi_hash_tbl_remove - remove a hash node from a hash table
*
* This routine removes the hash node that matches the specified key.
*
* RETURNS: PDI_OK, or PDI_ERROR if hash_id is invalid or no matching hash node is found.
******************************************************************************/
STATUS pdi_hash_tbl_remove( pdi_hash_id_t hash_id, pdi_hash_node_t *hash_node_p )
{
    pdi_hash_node_t *prev_node_p;
    int       ix;

    /* invoke hash table's hashing routine to get index into table */

    ix = (* hash_id->keyRtn) (hash_id->elements, hash_node_p, hash_id->keyArg);

    prev_node_p = sllPrevious (&hash_id->hash_tbl_p [ix], hash_node_p);

    pdi_sll_remove (&hash_id->hash_tbl_p [ix], hash_node_p, prev_node_p);

    return(PDI_OK);
}

/******************************************************************************
* FUNCTION: pdi_hash_tbl_each - call a routine for each node in a hash table
*
* RETURNS: NULL if traversed whole hash table, or pointer to pdi_hash_node_t that
*          pdi_hash_tbl_each ended with.
******************************************************************************/
pdi_hash_node_t *pdi_hash_tbl_each( pdi_hash_id_t     hash_id,
                        FUNCPTR     routine,
                        intptr_t    routineArg  )
{
    FAST int  ix;
    pdi_hash_node_t *node_p = NULL;

    for (ix = 0; (ix < hash_id->elements) && (node_p == NULL); ix++)
        node_p = (pdi_hash_node_t *)sllEach (&hash_id->hash_tbl_p[ix],routine,routineArg);

    return(node_p);     /* return node we ended with */
}

