/******************************************************************************
 * Copyright (c) 2011 Ericsson AB.
 * Copyright (c) 2008 Redback Networks, Inc.
 * This software is the confidential and proprietary information of
 * Redback Networks Inc.
 *
 * Description:
 *
 * Provides a linked list implementation.
******************************************************************************/

#include "pdi_linked_list.h"
#include <stdlib.h>

/******************************************************************************
* FUNCTION: sllCreate - create a singly linked list head
*
* RETURNS:  Pointer to singly linked list head, or NULL if creation failed.
******************************************************************************/
pdi_sl_list_t *sllCreate (void)
{
    pdi_sl_list_t *list_p = (pdi_sl_list_t *) malloc ((unsigned) sizeof (pdi_sl_list_t));

    pdi_sll_init (list_p);                /* initialize list */

    return(list_p);
}

/******************************************************************************
* FUNCTION: pdi_sll_init - initialize singly linked list head
*
* RETURNS: PDI_OK, or PDI_ERROR if intialization failed.
******************************************************************************/
STATUS pdi_sll_init( pdi_sl_list_t *list_p )
{
    list_p->head  = NULL;            /* initialize list */
    list_p->tail  = NULL;

    return(PDI_OK);
}

/******************************************************************************
* FUNCTION: pdi_sll_delete - terminate singly linked list head free associated memory
*
* Terminate the specified list.
*
* RETURNS: PDI_OK, or PDI_ERROR if singly linked list head could not be deallocated.
******************************************************************************/
STATUS pdi_sll_delete( pdi_sl_list_t *list_p )
{
    free ((char *) list_p);      /* free list */
    return PDI_OK;
}

/******************************************************************************
* FUNCTION:  pdi_sll_terminate - terminate singly linked list head
*
* RETURNS: PDI_OK, or PDI_ERROR if singly linked list could not be terminated.
******************************************************************************/
STATUS pdi_sll_terminate( pdi_sl_list_t *list_p UNUSED)
{
    return(PDI_OK);
}

/******************************************************************************
* FUNCTION: pdi_sll_put_at_head - add node to beginning of list
*
* RETURNS: none.
******************************************************************************/
void pdi_sll_put_at_head( pdi_sl_list_t *list_p, SL_NODE *node_p )
{
    if ((node_p->next = list_p->head) == NULL)
        list_p->head = list_p->tail = node_p;
    else
        list_p->head = node_p;
}

/******************************************************************************
* FUNCTION: pdi_sll_put_at_tail - add node to end of list
*
* RETURNS: none.
******************************************************************************/
void pdi_sll_put_at_tail( pdi_sl_list_t *list_p, SL_NODE *node_p )
{
    node_p->next = NULL;

    if (list_p->head == NULL)
        list_p->tail = list_p->head = node_p;
    else
        list_p->tail->next = node_p;
    list_p->tail = node_p;
}

/******************************************************************************
* FUNCTION: sllGet - get (delete and return) first node from list
*
* RETURNS: Pointer to the node gotten, or NULL if the list is empty.
******************************************************************************/
SL_NODE *sllGet( FAST pdi_sl_list_t *list_p )
{
    FAST SL_NODE *node_p;

    if ((node_p = list_p->head) != NULL)
        list_p->head = node_p->next;

    return(node_p);
}

/******************************************************************************
* FUNCTION: pdi_sll_remove - remove specified node in list
*
* RETURNS: none.
******************************************************************************/
void pdi_sll_remove( pdi_sl_list_t *list_p, SL_NODE *delete_node_p, SL_NODE *prev_node_p )
{
    if (prev_node_p == NULL) {
        list_p->head = delete_node_p->next;
        if (list_p->tail == delete_node_p)
            list_p->tail = NULL;
    } else {
        prev_node_p->next = delete_node_p->next;
        if (list_p->tail == delete_node_p)
            list_p->tail = prev_node_p;
    }
}

/******************************************************************************
* FUNCTION: sllPrevious - find and return previous node in list
*
* RETURNS: the previous node in a singly linked list.
******************************************************************************/
SL_NODE *sllPrevious( pdi_sl_list_t *list_p, SL_NODE *node_p )
{
    SL_NODE *pTmpNode = list_p->head;

    if ((pTmpNode == NULL) || (pTmpNode == node_p))
        return(NULL);                  /* no previous node */

    while (pTmpNode->next != NULL) {
        if (pTmpNode->next == node_p)
            return(pTmpNode);

        pTmpNode = pTmpNode->next;
    }

    return(NULL);                  /* node not found */
}

/******************************************************************************
* FUNCTION: pdi_sll_count - report number of nodes in list
*
* RETURNS: Number of nodes in specified list.
******************************************************************************/
int pdi_sll_count( pdi_sl_list_t *list_p )
{
    FAST SL_NODE *node_p = SLL_FIRST (list_p);
    FAST int count = 0;

    while (node_p != NULL) {
        count ++;
        node_p = SLL_NEXT (node_p);
    }

    return(count);
}

/******************************************************************************
* FUNCTION: sllEach - call a routine for each node in a linked list
*
* RETURNS: NULL if finished linked list, or pointer to last iterated SL_NODE
******************************************************************************/
SL_NODE *sllEach( pdi_sl_list_t *list_p, FUNCPTR routine, intptr_t routineArg )
{
    FAST SL_NODE *node_p = SLL_FIRST (list_p);
    FAST SL_NODE *pNext;

    while (node_p != NULL) {
        pNext = SLL_NEXT (node_p);
        if ((* routine) (node_p, routineArg) == FALSE)
            break;
        node_p = pNext;
    }

    return(node_p);    /* return last node */
}
