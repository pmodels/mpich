/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2020 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_EAGER_IQUEUE_RECV_H_INCLUDED
#define POSIX_EAGER_IQUEUE_RECV_H_INCLUDED

#include "iqueue_impl.h"

/* Work backward through the queue to find the first cell and put that one in the transport's
 * first_cell position. */
/* Because of the non-tail recursion here, we can't do the normal inlining. Summary discussion here:
 * https://stackoverflow.com/a/32498407/491687 */
static void MPIDI_POSIX_eager_iqueue_enqueue_cell(MPIDI_POSIX_eager_iqueue_transport_t * transport,
                                                  MPIDI_POSIX_eager_iqueue_cell_t * cell)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IQUEUE_ENQUEUE_CELL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IQUEUE_ENQUEUE_CELL);

    /* TODO: is there any better way than recursion? Can we avoid
     * this invertion if we switch to SPSC queue?*/

    /* Ingress queue of cells is inverted. Thus, we need to re-invert it. */
    if (cell->prev) {
        MPIDI_POSIX_eager_iqueue_cell_t *prev =
            MPIDI_POSIX_EAGER_IQUEUE_GET_CELL(transport, cell->prev);
        MPIDI_POSIX_eager_iqueue_enqueue_cell(transport, prev);
        cell->prev = 0;
    }
    cell->next = NULL;
    if (transport->last_cell) {
        transport->last_cell->next = cell;
    } else {
        transport->first_cell = cell;
    }
    transport->last_cell = cell;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IQUEUE_ENQUEUE_CELL);
}

MPL_STATIC_INLINE_PREFIX MPIDI_POSIX_eager_iqueue_cell_t
    * MPIDI_POSIX_eager_iqueue_receive_cell(MPIDI_POSIX_eager_iqueue_transport_t * transport)
{
    MPIDI_POSIX_eager_iqueue_cell_t *cell = NULL;
    MPIDI_POSIX_eager_iqueue_terminal_t *terminal;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IQUEUE_RECEIVE_CELL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IQUEUE_RECEIVE_CELL);

    /* Update the first_cell pointer in the queue if we pull out the first cell */
    if (transport->first_cell) {
        cell = transport->first_cell;
        if (cell->next == NULL) {
            transport->first_cell = NULL;
            transport->last_cell = NULL;
        } else {
            transport->first_cell = cell->next;
        }
    } else {
        /* TODO: current code sets one terminal per receiver. Contention may occur
         * when multiple senders insert cells into the same receiver simultaneously.
         * Can we expand the design to be one terminal per sender-receiver peer? I.e.,
         * the queue will become SPSC, and there will be ppn^2 terminals on each node.
         * How much performance can be improved by this change?*/

        /* If first_cell wasn't set, grab the next cell from the appropriate terminal */
        terminal = &transport->terminals[MPIDI_POSIX_global.my_local_rank];

        if (MPL_atomic_load_ptr(&terminal->head)) {
            void *head = MPL_atomic_swap_ptr(&terminal->head, NULL);

            cell = MPIDI_POSIX_EAGER_IQUEUE_GET_CELL(transport, head);

            /* If the head of the terminal has the prev pointer set, enqueue the current cell and
             * use that one instead. */
            if (cell && cell->prev) {
                MPIDI_POSIX_eager_iqueue_enqueue_cell(transport, cell);

                cell = transport->first_cell;
                MPIR_Assert(cell != NULL);
                MPIR_Assert(cell->next != NULL);
                transport->first_cell = cell->next;
            }
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IQUEUE_RECEIVE_CELL);
    return cell;
}


MPL_STATIC_INLINE_PREFIX int
MPIDI_POSIX_eager_recv_begin(MPIDI_POSIX_eager_recv_transaction_t * transaction)
{
    MPIDI_POSIX_eager_iqueue_transport_t *transport;
    MPIDI_POSIX_eager_iqueue_cell_t *cell;
    int ret = MPIDI_POSIX_NOK;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IQUEUE_RECEIVE_BEGIN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IQUEUE_RECEIVE_BEGIN);

    /* Get the transport with the global variables */
    transport = MPIDI_POSIX_eager_iqueue_get_transport();

    cell = MPIDI_POSIX_eager_iqueue_receive_cell(transport);

    if (cell) {
        transaction->src_grank = MPIDI_POSIX_global.local_procs[cell->from];
        transaction->payload = MPIDI_POSIX_EAGER_IQUEUE_CELL_PAYLOAD(cell);
        transaction->payload_sz = cell->payload_size;

        if (likely(cell->type == MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_HDR)) {
            transaction->msg_hdr = &cell->am_header;
        } else {
            MPIR_Assert(cell->type == MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_DATA);
            transaction->msg_hdr = NULL;
        }

        transaction->transport.iqueue.pointer_to_cell = cell;

        ret = MPIDI_POSIX_OK;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IQUEUE_RECEIVE_BEGIN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX void
MPIDI_POSIX_eager_recv_memcpy(MPIDI_POSIX_eager_recv_transaction_t * transaction,
                              void *dst, const void *src, size_t size)
{
    MPIR_Memcpy(dst, src, size);
}

MPL_STATIC_INLINE_PREFIX void
MPIDI_POSIX_eager_recv_commit(MPIDI_POSIX_eager_recv_transaction_t * transaction)
{
    MPIDI_POSIX_eager_iqueue_cell_t *cell;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IQUEUE_RECV_COMMIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IQUEUE_RECV_COMMIT);

    cell = (MPIDI_POSIX_eager_iqueue_cell_t *) transaction->transport.iqueue.pointer_to_cell;
    MPIR_Assert(cell != NULL);
    cell->next = NULL;
    cell->prev = 0;
    OPA_compiler_barrier();
    cell->type = MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IQUEUE_RECV_COMMIT);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_posted_hook(int grank)
{
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_completed_hook(int grank)
{
}

#endif /* POSIX_EAGER_IQUEUE_RECV_H_INCLUDED */
