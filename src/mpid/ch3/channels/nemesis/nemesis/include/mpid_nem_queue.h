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

int MPID_nem_network_poll(int in_blocking_progress);

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

static inline MPID_nem_cell_rel_ptr_t MPID_NEM_SWAP_REL (MPID_nem_cell_rel_ptr_t *ptr, MPID_nem_cell_rel_ptr_t val)
{
    MPID_nem_cell_rel_ptr_t ret;
    OPA_store_ptr(&ret.p, OPA_swap_ptr(&(ptr->p), OPA_load_ptr(&val.p)));
    return ret;
}

/* do a compare-and-swap with MPID_NEM_RELNULL */
static inline MPID_nem_cell_rel_ptr_t MPID_NEM_CAS_REL_NULL (MPID_nem_cell_rel_ptr_t *ptr, MPID_nem_cell_rel_ptr_t oldv)
{
    MPID_nem_cell_rel_ptr_t ret;
    OPA_store_ptr(&ret.p, OPA_cas_ptr(&(ptr->p), OPA_load_ptr(&oldv.p), MPID_NEM_REL_NULL));
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
#define MPID_nem_queue_empty(qhead)                                                     \
    (                                                                                   \
     MPID_NEM_IS_REL_NULL((qhead)->my_head) ? (                                         \
      MPID_NEM_IS_REL_NULL((qhead)->head) ? 1                                           \
      : ( (qhead)->my_head = (qhead)->head, MPID_NEM_SET_REL_NULL((qhead)->head), 0 )   \
     )                                                                                  \
     : 0                                                                                \
    )
#endif /* MPID_NEM_USE_MACROS */

#ifndef MPID_NEM_USE_MACROS
/* Gets the head */
static inline void
MPID_nem_queue_dequeue (MPID_nem_queue_ptr_t qhead, MPID_nem_cell_ptr_t *e)
{
    MPID_nem_cell_ptr_t _e;
    MPID_nem_cell_rel_ptr_t _r_e;

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
    MPID_nem_cell_ptr_t _e;					\
    MPID_nem_cell_rel_ptr_t _r_e;				\
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

/* FIXME We shouldn't really be using the MPID_Thread_mutex_* code but the
 * MPIDU_Process_locks code is a total mess right now.  In the long term we need
   to resolve this, but in the short run it should be safe on most (all?)
   platforms to use these instead.  Usually they will both boil down to a
   pthread_mutex_t and and associated functions. */
#define MPID_nem_queue_mutex_create MPID_Thread_mutex_create
#define MPID_nem_queue_mutex_lock   MPID_Thread_mutex_lock
#define MPID_nem_queue_mutex_unlock MPID_Thread_mutex_unlock

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
    MPID_nem_queue_mutex_create(&qhead->lock, NULL);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_QUEUE_INIT);
}

static inline void
MPID_nem_queue_enqueue (MPID_nem_queue_ptr_t qhead, MPID_nem_cell_ptr_t element)
{
    MPID_nem_cell_rel_ptr_t r_prev;
    MPID_nem_cell_rel_ptr_t r_element = MPID_NEM_ABS_TO_REL (element);

    MPID_nem_queue_mutex_lock(&qhead->lock);

    r_prev = qhead->tail;
    qhead->tail = r_element;
    if (MPID_NEM_IS_REL_NULL(r_prev)) {
        qhead->head = r_element;
    }
    else {
        MPID_NEM_REL_TO_ABS(r_prev)->next = r_element;
    }

    MPID_nem_queue_mutex_unlock(&qhead->lock);
}

/* This operation is only safe because this is a single-dequeuer queue impl. */
static inline MPID_nem_cell_ptr_t
MPID_nem_queue_head (MPID_nem_queue_ptr_t qhead)
{
    return MPID_NEM_REL_TO_ABS(qhead->my_head);
}

/* Assumption: regular loads & stores are atomic.  This may not be univerally
   true, but it's not uncommon.  We often need to use these "lock-ful" queues on
   platforms where atomics are not yet implemented, so we can't rely on the
   atomics to provide atomic load/store operations for us. */
static inline int
MPID_nem_queue_empty (MPID_nem_queue_ptr_t qhead)
{
    if (MPID_NEM_IS_REL_NULL (qhead->my_head))
    {
        if (MPID_NEM_IS_REL_NULL (qhead->head))
        {
            return 1;
        }
        else
        {
            qhead->my_head = qhead->head;
            MPID_NEM_SET_REL_NULL (qhead->head); /* reset it for next time */
        }
    }

    return 0;
}

static inline void
MPID_nem_queue_dequeue (MPID_nem_queue_ptr_t qhead, MPID_nem_cell_ptr_t *e)
{
    MPID_nem_cell_ptr_t _e;
    MPID_nem_cell_rel_ptr_t _r_e;

    _r_e = qhead->my_head;
    _e = MPID_NEM_REL_TO_ABS (_r_e);


    if (MPID_NEM_IS_REL_NULL(_e->next)) {
        /* a REL_NULL _e->next or writing qhead->tail both require locking */
        MPID_nem_queue_mutex_lock(&qhead->lock);
        qhead->my_head = _e->next;
        /* We have to check _e->next again because it may have changed between
           the time we checked it without the lock and the time that we acquired
           the lock. */
        if (MPID_NEM_IS_REL_NULL(_e->next)) {
            MPID_NEM_SET_REL_NULL(qhead->tail);
        }
        MPID_nem_queue_mutex_unlock(&qhead->lock);
    }
    else { /* !MPID_NEM_IS_REL_NULL(_e->next) */
        /* We don't need to lock because a non-null _e->next can't be changed by
           anyone but us (the dequeuer) and we don't need to modify qhead->tail
           because we aren't removing the last element from the queue. */
        qhead->my_head = _e->next;
    }

    MPID_NEM_SET_REL_NULL (_e->next);
    *e = _e;
}

#endif /* !defined(MPID_NEM_USE_LOCK_FREE_QUEUES) */

#endif /* MPID_NEM_QUEUE_H */
