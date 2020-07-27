/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_IMPL_H_INCLUDED
#define POSIX_EAGER_IMPL_H_INCLUDED

#ifndef POSIX_EAGER_INLINE
#ifndef POSIX_EAGER_DISABLE_INLINES

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_get_buf(void **eager_buf, size_t * eager_buf_sz)
{
    return MPIDI_POSIX_eager_func->get_buf(eager_buf, eager_buf_sz);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_send(void *eager_buf, size_t data_sz, int grank,
                                                    MPIDI_POSIX_am_header_t * msg_hdr)
{
    return MPIDI_POSIX_eager_func->send(eager_buf, data_sz, grank, msg_hdr);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_recv_begin(MPIDI_POSIX_eager_recv_transaction_t *
                                                          transaction)
{
    return MPIDI_POSIX_eager_func->recv_begin(transaction);
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
