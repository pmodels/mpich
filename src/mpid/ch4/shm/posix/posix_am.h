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

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_isend(int rank,
                                                  MPIR_Comm * comm,
                                                  int handler_id,
                                                  const void *am_hdr,
                                                  size_t am_hdr_sz,
                                                  const void *data,
                                                  MPI_Count count,
                                                  MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_ISEND);

    mpi_errno = MPIDI_NM_am_isend(rank, comm, handler_id, am_hdr, am_hdr_sz, data, count, datatype,
                                  sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_ISEND);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_isendv(int rank,
                                                   MPIR_Comm * comm,
                                                   int handler_id,
                                                   struct iovec *am_hdr,
                                                   size_t iov_len,
                                                   const void *data,
                                                   MPI_Count count,
                                                   MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_ISENDV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_ISENDV);

    mpi_errno = MPIDI_NM_am_isendv(rank, comm, handler_id, am_hdr, iov_len, data, count, datatype,
                                   sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_ISENDV);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_isend_reply(MPIR_Context_id_t context_id, int src_rank,
                                                        int handler_id,
                                                        const void *am_hdr,
                                                        size_t am_hdr_sz,
                                                        const void *data,
                                                        MPI_Count count,
                                                        MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_ISEND_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_ISEND_REPLY);

    mpi_errno = MPIDI_NM_am_isend_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz, data,
                                        count, datatype, sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_ISEND_REPLY);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_POSIX_am_hdr_max_sz(void)
{
    size_t val;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_HDR_MAX_SZ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_HDR_MAX_SZ);

    val = MPIDI_NM_am_hdr_max_sz();

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_HDR_MAX_SZ);

    return val;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_send_hdr(int rank,
                                                     MPIR_Comm * comm,
                                                     int handler_id,
                                                     const void *am_hdr, size_t am_hdr_sz)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_SEND_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_SEND_HDR);

    mpi_errno = MPIDI_NM_am_send_hdr(rank, comm, handler_id, am_hdr, am_hdr_sz);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_SEND_HDR);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_send_hdr_reply(MPIR_Context_id_t context_id,
                                                           int src_rank, int handler_id,
                                                           const void *am_hdr, size_t am_hdr_sz)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_SEND_HDR_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_SEND_HDR_REPLY);

    mpi_errno = MPIDI_NM_am_send_hdr_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_SEND_HDR_REPLY);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_recv(MPIR_Request * req)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_RECV);

    mpi_errno = MPIDI_NM_am_recv(req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_RECV);

    return mpi_errno;
}

#endif /* POSIX_AM_H_INCLUDED */
