/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_AM_H_INCLUDED
#define POSIX_AM_H_INCLUDED

#include "posix_impl.h"

static inline int MPIDI_SHM_am_isend(int rank,
                                     MPIR_Comm * comm,
                                     int handler_id,
                                     const void *am_hdr,
                                     size_t am_hdr_sz,
                                     const void *data,
                                     MPI_Count count,
                                     MPI_Datatype datatype, MPIR_Request * sreq, void *shm_context)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_AM_ISEND);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_AM_ISEND);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_am_isendv(int rank,
                                      MPIR_Comm * comm,
                                      int handler_id,
                                      struct iovec *am_hdr,
                                      size_t iov_len,
                                      const void *data,
                                      MPI_Count count,
                                      MPI_Datatype datatype, MPIR_Request * sreq, void *shm_context)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_AM_ISENDV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_AM_ISENDV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_AM_ISENDV);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_am_isend_reply(MPIR_Context_id_t context_id, int src_rank,
                                           int handler_id,
                                           const void *am_hdr,
                                           size_t am_hdr_sz,
                                           const void *data,
                                           MPI_Count count,
                                           MPI_Datatype datatype, MPIR_Request * sreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_AM_ISEND_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_AM_ISEND_REPLY);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_AM_ISEND_REPLY);
    return MPI_SUCCESS;
}

static inline size_t MPIDI_SHM_am_hdr_max_sz(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_AM_HDR_MAX_SZ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_AM_HDR_MAX_SZ);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_AM_HDR_MAX_SZ);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_am_send_hdr(int rank,
                                        MPIR_Comm * comm,
                                        int handler_id,
                                        const void *am_hdr, size_t am_hdr_sz, void *shm_context)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_AM_SEND_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_AM_SEND_HDR);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_AM_SEND_HDR);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_inject_am(int rank,
                                      MPIR_Comm * comm,
                                      int handler_id,
                                      const void *am_hdr,
                                      size_t am_hdr_sz,
                                      const void *data,
                                      MPI_Count count, MPI_Datatype datatype, void *shm_context)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_INJECT_AM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_INJECT_AM);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_INJECT_AM);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_inject_amv(int rank,
                                       MPIR_Comm * comm,
                                       int handler_id,
                                       struct iovec *am_hdr,
                                       size_t iov_len,
                                       const void *data,
                                       MPI_Count count, MPI_Datatype datatype, void *shm_context)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_INJECT_AMV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_INJECT_AMV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_INJECT_AMV);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_am_send_hdr_reply(MPIR_Context_id_t context_id, int src_rank,
                                              int handler_id, const void *am_hdr, size_t am_hdr_sz)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_AM_SEND_HDR_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_AM_SEND_HDR_REPLY);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_AM_SEND_HDR_REPLY);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_inject_am_reply(MPIR_Context_id_t context_id, int src_rank,
                                            int handler_id,
                                            const void *am_hdr,
                                            size_t am_hdr_sz,
                                            const void *data,
                                            MPI_Count count, MPI_Datatype datatype)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_INJECT_AM_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_INJECT_AM_REPLY);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_INJECT_AM_REPLY);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_inject_amv_reply(MPIR_Context_id_t context_id, int src_rank,
                                             int handler_id,
                                             struct iovec *am_hdr,
                                             size_t iov_len,
                                             const void *data,
                                             MPI_Count count, MPI_Datatype datatype)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_INJECT_AMV_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_INJECT_AMV_REPLY);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_INJECT_AMV_REPLY);
    return MPI_SUCCESS;
}

static inline size_t MPIDI_SHM_am_inject_max_sz(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_AM_INJECT_MAX_SZ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_AM_INJECT_MAX_SZ);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_AM_INJECT_MAX_SZ);
    return 0;
}

static inline int MPIDI_SHM_am_recv(MPIR_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_AM_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_AM_RECV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_AM_RECV);
    return MPI_SUCCESS;
}

#endif /* POSIX_AM_H_INCLUDED */
