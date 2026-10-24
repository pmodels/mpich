/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SOCK_AM_H_INCLUDED
#define SOCK_AM_H_INCLUDED

#include "sock_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr(int rank, MPIR_Comm * comm, int handler_id,
                                                  const void *am_hdr, MPI_Aint am_hdr_sz,
                                                  int src_vci, int dst_vci)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_SOCK_send(MPIDIU_comm_rank_to_av(comm, rank), handler_id, am_hdr, am_hdr_sz,
                                NULL, 0, NULL, src_vci, dst_vci, NULL);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend(int rank, MPIR_Comm * comm, int handler_id,
                                               const void *am_hdr, MPI_Aint am_hdr_sz,
                                               const void *data, MPI_Aint count,
                                               MPI_Datatype datatype, int src_vci, int dst_vci,
                                               MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int dt_contig;
    MPI_Aint data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    MPL_pointer_attr_t attr;
    MPIR_GPU_query_pointer_attr(data, &attr);
    if (MPL_gpu_attr_is_dev(&attr)) {
        /* we can't write to a socket from device memory, pack it to host memory */
        dt_contig = 0;
    }

    const void *send_data;
    void *pack_buf = NULL;
    if (dt_contig || data_sz == 0) {
        /* send directly from the user buffer */
        send_data = MPIR_get_contig_ptr(data, dt_true_lb);
    } else {
        pack_buf = MPL_malloc(data_sz, MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP(!pack_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

        MPI_Aint actual_pack_bytes;
        mpi_errno = MPIR_Typerep_pack(data, count, datatype, 0, pack_buf, data_sz,
                                      &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(actual_pack_bytes == data_sz);

        send_data = pack_buf;
    }

    /* MPIDI_SOCK_send takes over pack_buf */
    mpi_errno = MPIDI_SOCK_send(MPIDIU_comm_rank_to_av(comm, rank), handler_id, am_hdr, am_hdr_sz,
                                send_data, data_sz, pack_buf, src_vci, dst_vci, sreq);
    pack_buf = NULL;
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    MPL_free(pack_buf);
    goto fn_exit;
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
