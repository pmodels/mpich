/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef SHM_SHMAM_EAGER_IMPL_H_INCLUDED
#define SHM_SHMAM_EAGER_IMPL_H_INCLUDED

#ifndef SHMAM_EAGER_DIRECT
#ifndef SHMAM_EAGER_DISABLE_INLINES

MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_init(int rank, int grank, int size)
{
    return MPIDI_SHMAM_eager_func->init(rank, grank, size);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_finalize()
{
    return MPIDI_SHMAM_eager_func->finalize();
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_SHMAM_eager_threshold()
{
    return MPIDI_SHMAM_eager_func->threshold();
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_connect(int grank)
{
    return MPIDI_SHMAM_eager_func->connect(grank);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_listen(int *grank_p)
{
    return MPIDI_SHMAM_eager_func->listen(grank_p);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_accept(int grank)
{
    return MPIDI_SHMAM_eager_func->accept(grank);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_send(int grank,
                                                    MPIDI_SHM_am_header_t ** msg_hdr,
                                                    struct iovec **iov,
                                                    size_t * iov_num, int is_blocking)
{
    return MPIDI_SHMAM_eager_func->send(grank, msg_hdr, iov, iov_num, is_blocking);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_recv_begin(MPIDI_SHMAM_eager_recv_transaction_t *
                                                          transaction)
{
    return MPIDI_SHMAM_eager_func->recv_begin(transaction);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_SHMAM_eager_recv_memcpy(MPIDI_SHMAM_eager_recv_transaction_t *
                                                            transaction, void *dst, const void *src,
                                                            size_t size)
{
    return MPIDI_SHMAM_eager_func->recv_memcpy(transaction, dst, src, size);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_SHMAM_eager_recv_commit(MPIDI_SHMAM_eager_recv_transaction_t *
                                                            transaction)
{
    return MPIDI_SHMAM_eager_func->recv_commit(transaction);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_SHMAM_eager_recv_posted_hook(int grank)
{
    return MPIDI_SHMAM_eager_func->recv_posted_hook(grank);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_SHMAM_eager_recv_completed_hook(int grank)
{
    return MPIDI_SHMAM_eager_func->recv_completed_hook(grank);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_SHMAM_eager_anysource_posted_hook()
{
    return MPIDI_SHMAM_eager_func->anysource_posted_hook();
}

MPL_STATIC_INLINE_PREFIX void MPIDI_SHMAM_eager_anysource_completed_hook()
{
    return MPIDI_SHMAM_eager_func->anysource_completed_hook();
}

#endif /* SHMAM_EAGER_DISABLE_INLINES  */

#else /* SHMAM_EAGER_DIRECT */

#define __shmam_eager_direct_stub__  0
#define __shmam_eager_direct_fbox__  1

#if SHMAM_EAGER_DIRECT==__shmam_eager_direct_stub__
#include "../stub/shmam_eager_direct.h"
#elif SHMAM_EAGER_DIRECT==__shmam_eager_direct_fbox__
#include "../fbox/shmam_eager_direct.h"
#else
#error "No direct shmam eager included"
#endif

#endif /* SHMAM_EAGER_DIRECT */

#endif /* SHM_SHMAM_EAGER_IMPL_H_INCLUDED */
