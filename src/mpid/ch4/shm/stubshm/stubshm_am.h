/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef SHM_STUBSHM_AM_H_INCLUDED
#define SHM_STUBSHM_AM_H_INCLUDED

#include "stubshm_impl.h"

static inline int MPIDI_SHM_am_isend(int rank,
                                     MPIR_Comm * comm,
                                     int handler_id,
                                     const void *am_hdr,
                                     size_t am_hdr_sz,
                                     const void *data,
                                     MPI_Count count,
                                     MPI_Datatype datatype, MPIR_Request * sreq, void *shm_context)
{
    MPIR_Assert(0);
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
    MPIR_Assert(0);
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
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline size_t MPIDI_SHM_am_hdr_max_sz(void)
{
    MPIR_Assert(0);
    return 0;
}

static inline int MPIDI_SHM_am_send_hdr(int rank,
                                        MPIR_Comm * comm,
                                        int handler_id,
                                        const void *am_hdr, size_t am_hdr_sz, void *shm_context)
{
    MPIR_Assert(0);
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
    MPIR_Assert(0);
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
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_am_send_hdr_reply(MPIR_Context_id_t context_id, int src_rank,
                                              int handler_id, const void *am_hdr, size_t am_hdr_sz)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_inject_am_reply(MPIR_Context_id_t context_id, int src_rank,
                                            int handler_id,
                                            const void *am_hdr,
                                            size_t am_hdr_sz,
                                            const void *data,
                                            MPI_Count count, MPI_Datatype datatype)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_inject_amv_reply(MPIR_Context_id_t context_id, int src_rank,
                                             int handler_id,
                                             struct iovec *am_hdr,
                                             size_t iov_len,
                                             const void *data,
                                             MPI_Count count, MPI_Datatype datatype)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline size_t MPIDI_SHM_am_inject_max_sz(void)
{
    MPIR_Assert(0);
    return 0;
}

static inline int MPIDI_SHM_am_recv(MPIR_Request * req)
{
    MPIR_Assert(0);
    return 0;
}

#endif /* SHM_STUBSHM_AM_H_INCLUDED */
