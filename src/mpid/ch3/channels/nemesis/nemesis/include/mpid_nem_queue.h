/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_NEM_QUEUE_H
#define MPID_NEM_QUEUE_H
#include "mpid_nem_datatypes.h"
#include "mpid_nem_defs.h"
#include "mpid_nem_atomics.h"

#define MPID_nem_dump_cell_mpich2(cell, index)  MPID_nem_dump_cell_mpich2__((cell),(index),__FILE__,__LINE__) 

void MPID_nem_dump_cell_mpich2__( MPID_nem_cell_ptr_t cell, int, char* ,int);
void MPID_nem_dump_cell_mpich( MPID_nem_cell_ptr_t cell, int);

#undef FUNCNAME
#define FUNCNAME MPID_nem_cell_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPID_nem_cell_init(MPID_nem_cell_ptr_t cell)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_CELL_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_CELL_INIT);

    MPID_NEM_SET_REL_NULL(cell->next);
    memset((void *)&cell->pkt, 0, sizeof(MPID_nem_pkt_header_t));

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_CELL_INIT);
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_queue_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPID_nem_queue_init(MPID_nem_queue_ptr_t qhead)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_QUEUE_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_QUEUE_INIT);

    MPID_NEM_SET_REL_NULL(qhead->head);
    MPID_NEM_SET_REL_NULL(qhead->my_head);
    MPID_NEM_SET_REL_NULL(qhead->tail);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_QUEUE_INIT);
}

int MPID_nem_network_poll (MPID_nem_poll_dir_t in_or_out);

#define MPID_NEM_USE_SHADOW_HEAD
#define MPID_NEM_USE_MACROS

static inline MPID_nem_cell_rel_ptr_t MPID_NEM_SWAP_REL (volatile MPID_nem_cell_rel_ptr_t *ptr, MPID_nem_cell_rel_ptr_t val)
{
    MPID_nem_cell_rel_ptr_t ret;
    ret.p = MPIDU_Atomic_swap_char_ptr(&(ptr->p), val.p);
    return ret;
}

/* do a compare-and-swap with MPID_NEM_RELNULL */
static inline MPID_nem_cell_rel_ptr_t MPID_NEM_CAS_REL_NULL (volatile MPID_nem_cell_rel_ptr_t *ptr, MPID_nem_cell_rel_ptr_t oldv)
{
    MPID_nem_cell_rel_ptr_t ret;
    ret.p = MPIDU_Atomic_cas_char_ptr(&(ptr->p), oldv.p, MPID_NEM_REL_NULL);
    return ret;
}

#ifndef MPID_NEM_USE_MACROS
static inline void
MPID_nem_queue_enqueue (MPID_nem_queue_ptr_t qhead, MPID_nem_cell_ptr_t element)
{
    MPID_nem_cell_rel_ptr_t r_prev;
    MPID_nem_cell_rel_ptr_t r_element = MPID_NEM_ABS_TO_REL (element);

    r_prev = MPID_NEM_SWAP_REL (&(qhead->tail), r_element);

    if (MPID_NEM_IS_REL_NULL (r_prev))
    {
	qhead->head = r_element;
    }
    else
    {
	MPID_NEM_REL_TO_ABS (r_prev)->next = r_element;
    }
}
#else /*MPID_NEM_USE_MACROS */
#define MPID_nem_queue_enqueue(qhead, element) do {			\
    MPID_nem_cell_rel_ptr_t r_prev;					\
    MPID_nem_cell_rel_ptr_t r_element = MPID_NEM_ABS_TO_REL (element);	\
									\
    r_prev = MPID_NEM_SWAP_REL (&((qhead)->tail), r_element);		\
									\
    if (MPID_NEM_IS_REL_NULL (r_prev))					\
    {									\
	(qhead)->head = r_element;					\
    }									\
    else								\
    {									\
	MPID_NEM_REL_TO_ABS (r_prev)->next = r_element;			\
    }									\
} while (0)
#endif /*MPID_NEM_USE_MACROS */

#ifndef MPID_NEM_USE_MACROS
static inline int
MPID_nem_queue_empty (MPID_nem_queue_ptr_t qhead)
{
    if (MPID_NEM_IS_REL_NULL (qhead->my_head))
    {
	if (MPID_NEM_IS_REL_NULL (qhead->head))
	    return 1;
	else
	{
	    qhead->my_head = qhead->head;
	    MPID_NEM_SET_REL_NULL (qhead->head); /* reset it for next time */
	}
    }
    return 0;
}

#else /*MPID_NEM_USE_MACROS */
#define MPID_nem_queue_empty(qhead) ({						\
    int __ret = 0;								\
    if (MPID_NEM_IS_REL_NULL (qhead->my_head))					\
    {										\
	if (MPID_NEM_IS_REL_NULL (qhead->head))					\
	    __ret = 1;								\
	else									\
	{									\
	    qhead->my_head = qhead->head;					\
	    MPID_NEM_SET_REL_NULL (qhead->head); /* reset it for next time */	\
	}									\
    }										\
    __ret;									\
})
#endif /* MPID_NEM_USE_MACROS */

#ifndef MPID_NEM_USE_MACROS
/* Gets the head */
static inline void
MPID_nem_queue_dequeue (MPID_nem_queue_ptr_t qhead, MPID_nem_cell_ptr_t *e)
{
    register MPID_nem_cell_ptr_t _e;
    register MPID_nem_cell_rel_ptr_t _r_e;

    _r_e = qhead->my_head;

    _e = MPID_NEM_REL_TO_ABS (_r_e);
    if (!MPID_NEM_IS_REL_NULL(_e->next))
    {
	qhead->my_head = _e->next;
    }
    else
    {
	MPID_nem_cell_rel_ptr_t old_tail;
      
	MPID_NEM_SET_REL_NULL (qhead->my_head);	

	old_tail = MPID_NEM_CAS_REL_NULL (&(qhead->tail), _r_e);

	if (!MPID_NEM_REL_ARE_EQUAL (old_tail, _r_e))
	{
	    while (MPID_NEM_IS_REL_NULL (_e->next))
	    {
		SKIP;
	    }
	    qhead->my_head = _e->next;
	}
    }
    MPID_NEM_SET_REL_NULL (_e->next);
    *e = _e;
}

#else /* MPID_NEM_USE_MACROS */
#define MPID_nem_queue_dequeue(qhead, e) do {				\
    register MPID_nem_cell_ptr_t _e;					\
    register MPID_nem_cell_rel_ptr_t _r_e;				\
									\
    _r_e = (qhead)->my_head;						\
    _e = MPID_NEM_REL_TO_ABS (_r_e);					\
									\
    if (!MPID_NEM_IS_REL_NULL (_e->next))				\
    {									\
	(qhead)->my_head = _e->next;					\
    }									\
    else								\
    {									\
	MPID_nem_cell_rel_ptr_t old_tail;				\
									\
	MPID_NEM_SET_REL_NULL ((qhead)->my_head);			\
									\
	old_tail = MPID_NEM_CAS_REL_NULL (&((qhead)->tail), _r_e);	\
									\
	if (!MPID_NEM_REL_ARE_EQUAL (old_tail, _r_e))			\
	{								\
	    while (MPID_NEM_IS_REL_NULL (_e->next))			\
	    {								\
		SKIP;							\
	    }								\
	    (qhead)->my_head = _e->next;				\
	}								\
    }									\
    MPID_NEM_SET_REL_NULL (_e->next);					\
    *(e) = _e;								\
} while (0)                                             
#endif /* MPID_NEM_USE_MACROS */

#endif /* MPID_NEM_QUEUE_H */
