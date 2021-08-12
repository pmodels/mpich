/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_IMPL_H_INCLUDED
#define POSIX_EAGER_IMPL_H_INCLUDED

#ifndef POSIX_EAGER_INLINE
#ifndef POSIX_EAGER_DISABLE_INLINES

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_send(int grank, MPIDI_POSIX_am_header_t * msg_hdr,
                                                    const void *am_hdr, MPI_Aint am_hdr_sz,
                                                    const void *buf, MPI_Aint count,
                                                    MPI_Datatype datatype, MPI_Aint offset,
                                                    int src_vsi, int dst_vsi, MPI_Aint * bytes_sent)
{
    return MPIDI_POSIX_eager_func->send(grank, msg_hdr, am_hdr, am_hdr_sz, buf, count, datatype,
                                        offset, src_vsi, dst_vsi, bytes_sent);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_recv_begin(int vsi,
                                                          MPIDI_POSIX_eager_recv_transaction_t *
                                                          transaction)
{
    return MPIDI_POSIX_eager_func->recv_begin(vsi, transaction);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_memcpy(MPIDI_POSIX_eager_recv_transaction_t *
                                                            transaction, void *dst, const void *src,
                                                            size_t size)
{
    MPIDI_POSIX_eager_func->recv_memcpy(transaction, dst, src, size);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_commit(MPIDI_POSIX_eager_recv_transaction_t *
                                                            transaction)
{
    MPIDI_POSIX_eager_func->recv_commit(transaction);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_posted_hook(int grank)
{
    MPIDI_POSIX_eager_func->recv_posted_hook(grank);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_completed_hook(int grank)
{
    MPIDI_POSIX_eager_func->recv_completed_hook(grank);
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_POSIX_eager_payload_limit(void)
{
    return MPIDI_POSIX_eager_func->payload_limit();
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_POSIX_eager_buf_limit(void)
{
    return MPIDI_POSIX_eager_func->buf_limit();
}

#endif /* POSIX_EAGER_DISABLE_INLINES  */

#else /* POSIX_EAGER_INLINE */

#define __posix_eager_inline_stub__  0
#define __posix_eager_inline_iqueue__  1

#if POSIX_EAGER_INLINE==__posix_eager_inline_stub__
#include "../stub/posix_eager_inline.h"
#elif POSIX_EAGER_INLINE==__posix_eager_inline_iqueue__
#include "../iqueue/posix_eager_inline.h"
#else
#error "No direct posix eager included"
#endif

#endif /* POSIX_EAGER_INLINE */

#endif /* POSIX_EAGER_IMPL_H_INCLUDED */
