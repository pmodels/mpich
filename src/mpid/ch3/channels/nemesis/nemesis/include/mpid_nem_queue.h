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

int MPID_nem_network_poll (MPID_nem_poll_dir_t in_or_out);

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

#if defined(MPID_NEM_USE_LOCK_FREE_QUEUES)

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

/* This operation is only safe because this is a single-dequeuer queue impl.
   Assumes that MPID_nem_queue_empty was called immediately prior to fix up any
   shadow head issues. */
#ifndef MPID_NEM_USE_MACROS
static inline MPID_nem_cell_ptr_t
MPID_nem_queue_head (MPID_nem_queue_ptr_t qhead)
{
    return MPID_NEM_REL_TO_ABS(qhead->my_head);
}
#else /*MPID_NEM_USE_MACROS */
#define MPID_nem_queue_head(qhead_) MPID_NEM_REL_TO_ABS((qhead_)->my_head)
#endif

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

#else /* !defined(MPID_NEM_USE_LOCK_FREE_QUEUES) */

/* FIXME This is probably not the most efficient implementation right now.  We
   should probably look at adding the shadow head back in even for the lock-ful
   versions.  We should also look at not locking in some cases (taking advantage
   of the single-dequeuer restriction) and the organization of the pointers and
   the lock in the cache. [goodell@ 2009-01-16] */

/* must be called by exactly one process per queue */
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
    MPID_Thread_mutex_create(&qhead->lock, NULL);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_QUEUE_INIT);
}

static inline void
MPID_nem_queue_enqueue (MPID_nem_queue_ptr_t qhead, MPID_nem_cell_ptr_t element)
{
    MPID_nem_cell_rel_ptr_t r_prev;
    MPID_nem_cell_rel_ptr_t r_element = MPID_NEM_ABS_TO_REL (element);

    MPID_Thread_mutex_lock(&qhead->lock);

    r_prev = qhead->tail;
    qhead->tail = r_element;
    if (MPID_NEM_IS_REL_NULL(r_prev)) {
        qhead->head = r_element;
    }
    else {
        MPID_NEM_REL_TO_ABS(r_prev)->next = r_element;
    }

    MPID_Thread_mutex_unlock(&qhead->lock);
}

/* This operation is only safe because this is a single-dequeuer queue impl. */
static inline MPID_nem_cell_ptr_t
MPID_nem_queue_head (MPID_nem_queue_ptr_t qhead)
{
    MPID_nem_cell_ptr_t retval;
    MPID_Thread_mutex_lock(&qhead->lock);
    retval = MPID_NEM_REL_TO_ABS(qhead->head);
    MPID_Thread_mutex_unlock(&qhead->lock);
    return retval;
}

static inline int
MPID_nem_queue_empty (MPID_nem_queue_ptr_t qhead)
{
    int is_empty = 0;

    MPID_Thread_mutex_lock(&qhead->lock);

    /* the "lock-ful" impl doesn't use the shadow head right now */
    if (MPID_NEM_IS_REL_NULL(qhead->head)) {
        is_empty = 1;
    }

    MPID_Thread_mutex_unlock(&qhead->lock);
    return is_empty;
}

static inline void
MPID_nem_queue_dequeue (MPID_nem_queue_ptr_t qhead, MPID_nem_cell_ptr_t *e)
{
    register MPID_nem_cell_ptr_t _e;
    register MPID_nem_cell_rel_ptr_t _r_e;

    MPID_Thread_mutex_lock(&qhead->lock);

    /* the "lock-ful" impl doesn't use the shadow head right now */
    _r_e = qhead->head;

    _e = MPID_NEM_REL_TO_ABS (_r_e);

    qhead->head = _e->next;
    if (MPID_NEM_IS_REL_NULL(_e->next)) {
        MPID_NEM_SET_REL_NULL(qhead->tail);
    }

    MPID_NEM_SET_REL_NULL (_e->next);
    *e = _e;

    MPID_Thread_mutex_unlock(&qhead->lock);
}

#endif /* !defined(MPID_NEM_USE_LOCK_FREE_QUEUES) */

#endif /* MPID_NEM_QUEUE_H */
