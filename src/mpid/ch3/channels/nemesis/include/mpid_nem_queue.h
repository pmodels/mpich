/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* TODO figure out how to rewrite some/all of this queue code to use
 * explicit relaxed atomic operations */

#ifndef MPID_NEM_QUEUE_H_INCLUDED
#define MPID_NEM_QUEUE_H_INCLUDED
#include "mpid_nem_datatypes.h"
#include "mpid_nem_defs.h"
#include "mpid_nem_atomics.h"

int MPID_nem_network_poll(int in_blocking_progress);

/* Assertion macros for nemesis queues.  We don't use the normal
 * assertion macros because we don't usually want to assert several
 * times per queue operation.  These assertions serve more as structured
 * comments that can easily transformed into being real assertions */
#if 0
#define MPID_nem_q_assert(a_) \
    do {                                                             \
        if (unlikely(!(a_))) {                                       \
            MPID_nem_q_assert_fail(#a_, __FILE__, __LINE__);         \
        }                                                            \
    } while (0)
#define MPID_nem_q_assert_fail(a_str_, file_, line_) \
    do {/*nothing*/} while(0)
#else
#define MPID_nem_q_assert(a_) \
    do {/*nothing*/} while (0)
#endif

static inline void MPID_nem_cell_init(MPID_nem_cell_ptr_t cell)
{

    MPIR_FUNC_ENTER;

    MPID_NEM_SET_REL_NULL(cell->next);
    memset((void *)&cell->header, 0, sizeof(MPID_nem_pkt_header_t));

    MPIR_FUNC_EXIT;
}

static inline void MPID_nem_queue_init(MPID_nem_queue_ptr_t qhead)
{

    MPIR_FUNC_ENTER;

    MPID_NEM_SET_REL_NULL(qhead->head);
    MPID_NEM_SET_REL_NULL(qhead->my_head);
    MPID_NEM_SET_REL_NULL(qhead->tail);

    MPIR_FUNC_EXIT;
}

#define MPID_NEM_USE_SHADOW_HEAD

static inline MPID_nem_cell_rel_ptr_t MPID_NEM_SWAP_REL (MPID_nem_cell_rel_ptr_t *ptr, MPID_nem_cell_rel_ptr_t val)
{
    MPID_nem_cell_rel_ptr_t ret;
    MPL_atomic_release_store_ptr(&ret.p, MPL_atomic_swap_ptr(&(ptr->p), MPL_atomic_acquire_load_ptr(&val.p)));
    return ret;
}

/* do a compare-and-swap with MPID_NEM_RELNULL */
static inline MPID_nem_cell_rel_ptr_t MPID_NEM_CAS_REL_NULL (MPID_nem_cell_rel_ptr_t *ptr, MPID_nem_cell_rel_ptr_t oldv)
{
    MPID_nem_cell_rel_ptr_t ret;
    MPL_atomic_release_store_ptr(&ret.p, MPL_atomic_cas_ptr(&(ptr->p), MPL_atomic_acquire_load_ptr(&oldv.p), MPID_NEM_REL_NULL));
    return ret;
}

static inline void
MPID_nem_queue_enqueue (MPID_nem_queue_ptr_t qhead, MPID_nem_cell_ptr_t element)
{
    MPID_nem_cell_rel_ptr_t r_prev;
    MPID_nem_cell_rel_ptr_t r_element = MPID_NEM_ABS_TO_REL (element);

    /* the _dequeue can break if this does not hold */
    MPID_nem_q_assert(MPID_NEM_IS_REL_NULL(element->next));

    /* Orders payload and e->next=NULL w.r.t. the SWAP, updating head, and
     * updating prev->next.  We assert e->next==NULL above, but it may have been
     * done by us in the preceding _dequeue operation.
     *
     * The SWAP itself does not need to be ordered w.r.t. the payload because
     * the consumer does not directly inspect the tail.  But the subsequent
     * update to the head or e->next field does need to be ordered w.r.t. the
     * payload or the consumer may read incorrect data. */
    MPL_atomic_write_barrier();

    /* enqueue at tail */
    r_prev = MPID_NEM_SWAP_REL (&(qhead->tail), r_element);
    if (MPID_NEM_IS_REL_NULL (r_prev))
    {
        /* queue was empty, element is the new head too */

        /* no write barrier needed, we believe atomic SWAP with a control
         * dependence (if) will enforce ordering between the SWAP and the head
         * assignment */
        qhead->head = r_element;
    }
    else
    {
        /* queue was not empty, swing old tail's next field to point to
         * our element */
        MPID_nem_q_assert(MPID_NEM_IS_REL_NULL(MPID_NEM_REL_TO_ABS(r_prev)->next));

        /* no write barrier needed, we believe atomic SWAP with a control
         * dependence (if/else) will enforce ordering between the SWAP and the
         * prev->next assignment */
        MPID_NEM_REL_TO_ABS (r_prev)->next = r_element;
    }
}

/* This operation is only safe because this is a single-dequeuer queue impl.
   Assumes that MPID_nem_queue_empty was called immediately prior to fix up any
   shadow head issues. */
static inline MPID_nem_cell_ptr_t
MPID_nem_queue_head (MPID_nem_queue_ptr_t qhead)
{
    MPID_nem_q_assert(MPID_NEM_IS_REL_NULL(qhead->head));
    return MPID_NEM_REL_TO_ABS(qhead->my_head);
}

static inline int
MPID_nem_queue_empty (MPID_nem_queue_ptr_t qhead)
{
    /* outside of this routine my_head and head should never both
     * contain a non-null value */
    MPID_nem_q_assert(MPID_NEM_IS_REL_NULL(qhead->my_head) ||
                      MPID_NEM_IS_REL_NULL(qhead->head));

    if (MPID_NEM_IS_REL_NULL (qhead->my_head))
    {
        /* the order of comparison between my_head and head does not
         * matter, no read barrier needed here */
        if (MPID_NEM_IS_REL_NULL (qhead->head))
        {
            /* both null, nothing in queue */
            return 1;
        }
        else
        {
            /* shadow head null and head has value, move the value to
             * our private shadow head and zero the real head */
            qhead->my_head = qhead->head;
            /* no barrier needed, my_head is entirely private to consumer */
            MPID_NEM_SET_REL_NULL (qhead->head);
        }
    }

    /* the following assertions are present at the beginning of _dequeue:
    MPID_nem_q_assert(!MPID_NEM_IS_REL_NULL(qhead->my_head));
    MPID_nem_q_assert( MPID_NEM_IS_REL_NULL(qhead->head));
    */
    return 0;
}


/* Gets the head */
static inline void
MPID_nem_queue_dequeue (MPID_nem_queue_ptr_t qhead, MPID_nem_cell_ptr_t *e)
{
    MPID_nem_cell_ptr_t _e;
    MPID_nem_cell_rel_ptr_t _r_e;

    /* _empty always called first, moving head-->my_head */
    MPID_nem_q_assert(!MPID_NEM_IS_REL_NULL(qhead->my_head));
    MPID_nem_q_assert( MPID_NEM_IS_REL_NULL(qhead->head));

    _r_e = qhead->my_head;
    _e = MPID_NEM_REL_TO_ABS (_r_e);

    /* no barrier needed, my_head is private to consumer, plus
     * head/my_head and _e->next are ordered by a data dependency */
    if (!MPID_NEM_IS_REL_NULL(_e->next))
    {
        qhead->my_head = _e->next;
    }
    else
    {
        /* we've reached the end (tail) of the queue */
        MPID_nem_cell_rel_ptr_t old_tail;

        MPID_NEM_SET_REL_NULL (qhead->my_head);
        /* no barrier needed, the caller doesn't need any ordering w.r.t.
         * my_head or the tail */
        old_tail = MPID_NEM_CAS_REL_NULL (&(qhead->tail), _r_e);

        if (!MPID_NEM_REL_ARE_EQUAL (old_tail, _r_e))
        {
            /* FIXME is a barrier needed here because of the control-only dependency? */
            while (MPID_NEM_IS_REL_NULL (_e->next))
            {
                SKIP;
            }
            /* no read barrier needed between loads from the same location */
            qhead->my_head = _e->next;
        }
    }
    MPID_NEM_SET_REL_NULL (_e->next);

    /* Conservative read barrier here to ensure loads from head are ordered
     * w.r.t. payload reads by the caller.  The McKenney "whymb" document's
     * Figure 11 indicates that we don't need a barrier, but we are currently
     * unconvinced of this.  Further work, ideally using more formal methods,
     * should justify removing this.  (note that this barrier won't cost us
     * anything on many platforms, esp. x86) */
    MPL_atomic_read_barrier();

    *e = _e;
}

#endif /* MPID_NEM_QUEUE_H_INCLUDED */
