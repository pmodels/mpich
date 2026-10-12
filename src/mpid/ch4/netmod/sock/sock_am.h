/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SOCK_AM_H_INCLUDED
#define SOCK_AM_H_INCLUDED

#include "sock_impl.h"

/* TODO: am_send_hdr and am_isend are not implemented yet */

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr(int rank, MPIR_Comm * comm, int handler_id,
                                                  const void *am_hdr, MPI_Aint am_hdr_sz,
                                                  int src_vci, int dst_vci)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend(int rank, MPIR_Comm * comm, int handler_id,
                                               const void *am_hdr, MPI_Aint am_hdr_sz,
                                               const void *data, MPI_Aint count,
                                               MPI_Datatype datatype, int src_vci, int dst_vci,
                                               MPIR_Request * sreq)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr_reply(MPIR_Comm * comm, int src_rank,
                                                        int handler_id, const void *am_hdr,
                                                        MPI_Aint am_hdr_sz, int src_vci,
                                                        int dst_vci)
{
    return MPIDI_NM_am_send_hdr(src_rank, comm, handler_id, am_hdr, am_hdr_sz, src_vci, dst_vci);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend_reply(MPIR_Comm * comm, int src_rank, int handler_id,
                                                     const void *am_hdr, MPI_Aint am_hdr_sz,
                                                     const void *data, MPI_Aint count,
                                                     MPI_Datatype datatype, int src_vci,
                                                     int dst_vci, MPIR_Request * sreq)
{
    return MPIDI_NM_am_isend(src_rank, comm, handler_id, am_hdr, am_hdr_sz, data, count, datatype,
                             src_vci, dst_vci, sreq);
}

MPL_STATIC_INLINE_PREFIX MPIDIG_recv_data_copy_cb MPIDI_NM_am_get_data_copy_cb(uint32_t attr)
{
    return NULL;
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_NM_am_hdr_max_sz(void)
{
    return MPIDI_SOCK_global.eager_size;
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_NM_am_eager_limit(void)
{
    return MPIDI_SOCK_global.eager_size;
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_NM_am_eager_buf_limit(void)
{
    return MPIDI_SOCK_global.eager_size;
}

MPL_STATIC_INLINE_PREFIX bool MPIDI_NM_am_check_eager(MPI_Aint am_hdr_sz, MPI_Aint data_sz,
                                                      const void *data, MPI_Aint count,
                                                      MPI_Datatype datatype, MPIR_Request * sreq)
{
    return (am_hdr_sz + data_sz) <= MPIDI_SOCK_global.eager_size;
}

MPL_STATIC_INLINE_PREFIX bool MPIDI_NM_am_can_do_tag(MPIR_Request * rreq)
{
    return false;
}

/* am_tag_send and am_tag_recv are never called since MPIDI_NM_am_can_do_tag returns false */
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_tag_send(int rank, MPIR_Comm * comm, int handler_id,
                                                  int tag, const void *buf, MPI_Aint count,
                                                  MPI_Datatype datatype, int src_vci, int dst_vci,
                                                  MPIR_Request * sreq)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_tag_recv(int rank, MPIR_Comm * comm, int handler_id,
                                                  int tag, void *buf, MPI_Aint count,
                                                  MPI_Datatype datatype, int src_vci, int dst_vci,
                                                  MPIR_Request * rreq)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* SOCK_AM_H_INCLUDED */
