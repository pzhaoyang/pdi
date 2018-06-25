#ifndef __LINKED_LIST_H__
#define __LINKED_LIST_H__

#include "pdi_internal.h"

// Header for a linked list. 
typedef struct{
    SL_NODE*                    head;    // header of list
    SL_NODE*                    tail;    // tail of list
} pdi_sl_list_t;

#define SLL_FIRST(list_p)       (((pdi_sl_list_t *)list_p)->head)
#define SLL_LAST(list_p)        (((pdi_sl_list_t *)list_p)->tail)
#define SLL_NEXT(node_p)        (((SL_NODE *)node_p)->next)
#define SLL_EMPTY(list_p)       (((pdi_sl_list_t *)list_p)->head == NULL)

pdi_sl_list_t *sllCreate (void);
SL_NODE *sllEach (pdi_sl_list_t *list_p, FUNCPTR routine, intptr_t routineArg);
SL_NODE *sllGet (pdi_sl_list_t *list_p);
SL_NODE *sllPrevious (pdi_sl_list_t *list_p, SL_NODE *node_p);
STATUS  pdi_sll_delete (pdi_sl_list_t *list_p);
STATUS  pdi_sll_init (pdi_sl_list_t *list_p);
STATUS  pdi_sll_terminate (pdi_sl_list_t *list_p);
int     pdi_sll_count (pdi_sl_list_t *list_p);
void    pdi_sll_put_at_head (pdi_sl_list_t *list_p, SL_NODE *node_p);
void    pdi_sll_put_at_tail (pdi_sl_list_t *list_p, SL_NODE *node_p);
void    pdi_sll_remove (pdi_sl_list_t *list_p, SL_NODE *delete_node_p, SL_NODE *prev_node_p);

#endif
