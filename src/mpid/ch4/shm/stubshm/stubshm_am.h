/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef STUBSHM_AM_H_INCLUDED
#define STUBSHM_AM_H_INCLUDED

#include "stubshm_impl.h"

static inline int MPIDI_STUBSHM_am_isend(int rank,
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

static inline int MPIDI_STUBSHM_am_isendv(int rank,
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

static inline int MPIDI_STUBSHM_am_isend_reply(MPIR_Context_id_t context_id, int src_rank,
                                               int handler_id,
                                               const void *am_hdr,
                                               size_t am_hdr_sz,
                                               const void *data,
                                               MPI_Count count,
                                               MPI_Datatype datatype, MPIR_Request * sreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_AM_ISEND_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_AM_ISEND_REPLY);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_AM_ISEND_REPLY);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_am_isend_pipeline(MPIR_Context_id_t context_id,
                                                             int src_rank,
                                                             MPIDI_STUBSHM_am_header_kind_t kind,
                                                             int handler_id,
                                                             const void *am_hdr,
                                                             size_t am_hdr_sz,
                                                             const void *data,
                                                             MPI_Count count,
                                                             MPI_Datatype datatype,
                                                             MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_AM_ISEND_PIPELINE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_AM_ISEND_PIPELINE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_AM_ISEND_PIPELINE);

    return mpi_errno;
}

static inline size_t MPIDI_STUBSHM_am_hdr_max_sz(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_AM_HDR_MAX_SZ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_AM_HDR_MAX_SZ);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_AM_HDR_MAX_SZ);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_am_send_hdr(int rank,
                                            MPIR_Comm * comm,
                                            int handler_id, const void *am_hdr, size_t am_hdr_sz)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_AM_SEND_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_AM_SEND_HDR);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_AM_SEND_HDR);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_am_send_hdr_reply(MPIR_Context_id_t context_id, int src_rank,
                                                  int handler_id, const void *am_hdr,
                                                  size_t am_hdr_sz)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_AM_SEND_HDR_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_AM_SEND_HDR_REPLY);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_AM_SEND_HDR_REPLY);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_am_isend_pipeline_rts(int rank, MPIR_Comm * comm, int handler_id,
                                                      const void *am_hdr, size_t am_hdr_sz,
                                                      const void *data, MPI_Count count,
                                                      MPI_Datatype datatype, MPIR_Request * sreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_AM_ISEND_PIPELINE_RTS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_AM_ISEND_PIPELINE_RTS);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_AM_ISEND_PIPELINE_RTS);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_am_isend_pipeline_seg(MPIR_Context_id_t context_id, int src_rank,
                                                      int handler_id, const void *am_hdr,
                                                      size_t am_hdr_sz)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_AM_ISEND_PIPELINE_SEG);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_AM_ISEND_PIPELINE_SEG);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_AM_ISEND_PIPELINE_SEG);
    return MPI_SUCCESS;
}

static inline size_t MPIDI_STUBSHM_am_eager_limit(void)
{
    MPIR_Assert(0);
    return 0;
}

static inline size_t MPIDI_STUBSHM_am_eager_buf_limit(void)
{
    MPIR_Assert(0);
    return 0;
}

static inline int MPIDI_STUBSHM_am_choose_protocol(const void *buf, MPI_Count count,
                                                   MPI_Datatype datatype, size_t am_ext_sz,
                                                   int handler_id)
{
    int protocol = 0;

    MPIR_Assert(0);

    return protocol;
}

#endif /* STUBSHM_AM_H_INCLUDED */
