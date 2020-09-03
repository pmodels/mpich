/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef STUBSHM_AM_H_INCLUDED
#define STUBSHM_AM_H_INCLUDED

#include "stubshm_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_am_isend(int rank,
                                                    MPIR_Comm * comm,
                                                    int handler_id,
                                                    const void *am_hdr,
                                                    size_t am_hdr_sz,
                                                    const void *data,
                                                    MPI_Count count,
                                                    MPI_Datatype datatype, MPIR_Request * sreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_AM_ISEND);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_AM_ISEND);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_am_isendv(int rank,
                                                     MPIR_Comm * comm,
                                                     int handler_id,
                                                     struct iovec *am_hdr,
                                                     size_t iov_len,
                                                     const void *data,
                                                     MPI_Count count,
                                                     MPI_Datatype datatype, MPIR_Request * sreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_AM_ISENDV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_AM_ISENDV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_AM_ISENDV);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_am_isend_reply(MPIR_Context_id_t context_id,
                                                          int src_rank, int handler_id,
                                                          const void *am_hdr, size_t am_hdr_sz,
                                                          const void *data, MPI_Count count,
                                                          MPI_Datatype datatype,
                                                          MPIR_Request * sreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_AM_ISEND_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_AM_ISEND_REPLY);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_AM_ISEND_REPLY);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_STUBSHM_am_hdr_max_sz(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_AM_HDR_MAX_SZ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_AM_HDR_MAX_SZ);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_AM_HDR_MAX_SZ);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_am_send_hdr(int rank,
                                                       MPIR_Comm * comm,
                                                       int handler_id, const void *am_hdr,
                                                       size_t am_hdr_sz)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_AM_SEND_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_AM_SEND_HDR);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_AM_SEND_HDR);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_am_send_hdr_reply(MPIR_Context_id_t context_id,
                                                             int src_rank, int handler_id,
                                                             const void *am_hdr, size_t am_hdr_sz)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_AM_SEND_HDR_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_AM_SEND_HDR_REPLY);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_AM_SEND_HDR_REPLY);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_STUBSHM_am_eager_limit(void)
{
    MPIR_Assert(0);
    return 0;
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_STUBSHM_am_eager_buf_limit(void)
{
    MPIR_Assert(0);
    return 0;
}

#endif /* STUBSHM_AM_H_INCLUDED */
