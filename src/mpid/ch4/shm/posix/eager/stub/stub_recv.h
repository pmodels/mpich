/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_STUB_RX_H_INCLUDED
#define POSIX_EAGER_STUB_RX_H_INCLUDED

#include "stub_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_recv_begin(MPIDI_POSIX_eager_recv_transaction_t *
                                                          transaction)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_memcpy(MPIDI_POSIX_eager_recv_transaction_t *
                                                            transaction, void *dst, const void *src,
                                                            size_t size)
{
    MPIR_Assert(0);
    return;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_commit(MPIDI_POSIX_eager_recv_transaction_t *
                                                            transaction)
{
    MPIR_Assert(0);
    return;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_posted_hook(int grank)
{
    MPIR_Assert(0);
    return;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_completed_hook(int grank)
{
    MPIR_Assert(0);
    return;
}

#endif /* POSIX_EAGER_STUB_RX_H_INCLUDED */
