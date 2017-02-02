/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_EAGER_IQUEUE_RECV_H_INCLUDED
#define POSIX_EAGER_IQUEUE_RECV_H_INCLUDED

#include "iqueue_impl.h"

static void MPIDI_POSIX_EAGER_IQUEUE_enqueue_cell(MPIDI_POSIX_EAGER_IQUEUE_transport_t * transport,
                                                  MPIDI_POSIX_EAGER_IQUEUE_cell_t * cell)
{
    /* Ingress queue of cells is inverted. Thus, we need to re-invert it. */
    if (cell->prev) {
        MPIDI_POSIX_EAGER_IQUEUE_cell_t *prev =
            MPIDI_POSIX_EAGER_IQUEUE_GET_CELL(transport, cell->prev);
        MPIDI_POSIX_EAGER_IQUEUE_enqueue_cell(transport, prev);
        cell->prev = 0;
    }
    cell->next = NULL;
    if (transport->last_cell) {
        transport->last_cell->next = cell;
    } else {
        transport->first_cell = cell;
    }
    transport->last_cell = cell;
}

static inline MPIDI_POSIX_EAGER_IQUEUE_cell_t
    * MPIDI_POSIX_EAGER_IQUEUE_receive_cell(MPIDI_POSIX_EAGER_IQUEUE_transport_t * transport)
{
    MPIDI_POSIX_EAGER_IQUEUE_cell_t *cell;
    MPIDI_POSIX_EAGER_IQUEUE_terminal_t *terminal;

    if (transport->first_cell) {
        cell = transport->first_cell;
        if (cell->next == NULL) {
            transport->first_cell = NULL;
            transport->last_cell = NULL;
        } else {
            transport->first_cell = cell->next;
        }
        return cell;
    }

    terminal = &transport->terminals[transport->local_rank];

    if (terminal->head) {
        uintptr_t head =
            (uintptr_t) (unsigned int *) OPA_swap_ptr((OPA_ptr_t *) & terminal->head, NULL);

        cell = MPIDI_POSIX_EAGER_IQUEUE_GET_CELL(transport, head);

        if (cell->prev == 0) {
            return cell;
        }

        MPIDI_POSIX_EAGER_IQUEUE_enqueue_cell(transport, cell);

        cell = transport->first_cell;
        MPIR_Assert(cell != NULL);
        MPIR_Assert(cell->next != NULL);
        transport->first_cell = cell->next;

        return cell;
    }

    return NULL;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_eager_recv_begin
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int
MPIDI_POSIX_eager_recv_begin(MPIDI_POSIX_eager_recv_transaction_t * transaction)
{
    MPIDI_POSIX_EAGER_IQUEUE_transport_t *transport;
    MPIDI_POSIX_EAGER_IQUEUE_cell_t *cell;

    transport = MPIDI_POSIX_EAGER_IQUEUE_get_transport();

    cell = MPIDI_POSIX_EAGER_IQUEUE_receive_cell(transport);

    if (cell) {
        transaction->src_grank = transport->local_procs[cell->from];
        transaction->payload = MPIDI_POSIX_EAGER_IQUEUE_CELL_PAYLOAD(cell);
        transaction->payload_sz = cell->payload_size;

        if (likely(cell->type == MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_HEAD)) {
            transaction->msg_hdr = &cell->am_header;
        } else {
            MPIR_Assert(cell->type == MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_TAIL);
            transaction->msg_hdr = NULL;
        }

        transaction->transport.iqueue.pointer_to_cell = cell;

        return MPIDI_POSIX_OK;
    }
    return MPIDI_POSIX_NOK;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_eager_recv_memcpy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void
MPIDI_POSIX_eager_recv_memcpy(MPIDI_POSIX_eager_recv_transaction_t * transaction,
                              void *dst, const void *src, size_t size)
{
    memcpy(dst, src, size);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_eager_recv_commit
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void
MPIDI_POSIX_eager_recv_commit(MPIDI_POSIX_eager_recv_transaction_t * transaction)
{
    MPIDI_POSIX_EAGER_IQUEUE_cell_t *cell;
    cell = (MPIDI_POSIX_EAGER_IQUEUE_cell_t *) transaction->transport.iqueue.pointer_to_cell;
    MPIR_Assert(cell != NULL);
    cell->next = NULL;
    cell->prev = 0;
    OPA_compiler_barrier();
    cell->type = MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_NULL;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_posted_hook(int grank)
{
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_completed_hook(int grank)
{
}

#endif /* POSIX_EAGER_IQUEUE_RECV_H_INCLUDED */
