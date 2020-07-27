/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_IQUEUE_RECV_H_INCLUDED
#define POSIX_EAGER_IQUEUE_RECV_H_INCLUDED

#include "iqueue_impl.h"
#include "mpidu_genq.h"

MPL_STATIC_INLINE_PREFIX int
MPIDI_POSIX_eager_recv_begin(MPIDI_POSIX_eager_recv_transaction_t * transaction)
{
    MPIDI_POSIX_eager_iqueue_transport_t *transport;
    MPIDI_POSIX_eager_iqueue_cell_t *cell = NULL;
    int ret = MPIDI_POSIX_NOK;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_EAGER_RECV_BEGIN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_EAGER_RECV_BEGIN);

    /* Get the transport with the global variables */
    transport = MPIDI_POSIX_eager_iqueue_get_transport();

    MPIDU_genq_shmem_queue_dequeue(transport->my_terminal, (void **) &cell);

    if (cell) {
        transaction->src_grank = MPIDI_POSIX_global.local_procs[cell->from];
        transaction->payload = MPIDI_POSIX_EAGER_IQUEUE_CELL_GET_PAYLOAD(cell);

        transaction->msg_hdr = &cell->am_header;

        transaction->transport.iqueue.pointer_to_cell = cell;

        ret = MPIDI_POSIX_OK;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_EAGER_RECV_BEGIN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX void
MPIDI_POSIX_eager_recv_memcpy(MPIDI_POSIX_eager_recv_transaction_t * transaction,
                              void *dst, const void *src, size_t size)
{
    MPIR_Typerep_copy(dst, src, size);
}

MPL_STATIC_INLINE_PREFIX void
MPIDI_POSIX_eager_recv_commit(MPIDI_POSIX_eager_recv_transaction_t * transaction)
{
    MPIDI_POSIX_eager_iqueue_cell_t *cell;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_EAGER_RECV_COMMIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_EAGER_RECV_COMMIT);

    cell = (MPIDI_POSIX_eager_iqueue_cell_t *) transaction->transport.iqueue.pointer_to_cell;
    MPIDU_genq_shmem_pool_cell_free(cell);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_EAGER_RECV_COMMIT);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_posted_hook(int grank)
{
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_completed_hook(int grank)
{
}

#endif /* POSIX_EAGER_IQUEUE_RECV_H_INCLUDED */
