/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2019 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#ifndef XPMEM_RECV_H_INCLUDED
#define XPMEM_RECV_H_INCLUDED

#include "ch4_impl.h"
#include "shm_control.h"
#include "xpmem_pre.h"
#include "xpmem_seg.h"
#include "xpmem_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_XPMEM_COOP_COPY_CHUNK_SIZE
      category    : CH4
      type        : int
      default     : 32768
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Chunk size is used in XPMEM cooperative copy based point-to-point
        communication. It determines the concurrency of copy and overhead of
        atomic per chunk copy. Best chunk size should balance these two factors.

    - name        : MPIR_CVAR_CH4_XPMEM_COOP_COPY_THRESHOLD
      category    : CH4
      type        : int
      default     : 32768
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        When data size is larger than this threshold, cooperative copy is used;
        if not, we need to fall back to single-side XPMEM copy.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_handle_lmt_coop_recv(uint64_t src_offset,
                                                              size_t src_data_sz,
                                                              uint64_t sreq_ptr, int src_lrank,
                                                              MPIR_Comm * comm, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint dt_true_lb;
    MPIDI_XPMEM_seg_t *seg_ptr = NULL;
    void *src_buf = NULL;
    void *dest_buf = NULL;
    size_t data_sz, recv_data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_XPMEM_HANDLE_LMT_COOP_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_XPMEM_HANDLE_LMT_COOP_RECV);

    MPIDI_Datatype_check_size_lb(MPIDIG_REQUEST(rreq, datatype), MPIDIG_REQUEST(rreq, count),
                                 data_sz, dt_true_lb);

    /* Data truncation checking */
    recv_data_sz = MPL_MIN(src_data_sz, data_sz);
    if (src_data_sz > data_sz)
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;

    /* allocate shm counter */
    MPIDI_XPMEM_cnt_t *counter_ptr;
    counter_ptr = (MPIDI_XPMEM_cnt_t *) MPIR_Handle_obj_alloc(&MPIDI_XPMEM_cnt_mem);
    MPIDI_XPMEM_REQUEST(rreq, counter_ptr) = counter_ptr;

    MPL_atomic_store_int64(&counter_ptr->obj.offset, 0);

    int coop_counter_direct_flag;
    uint64_t coop_counter_offset;
    if (HANDLE_GET_KIND(counter_ptr->obj.handle) == HANDLE_KIND_DIRECT) {
        coop_counter_direct_flag = 1;
        coop_counter_offset = (uint64_t) counter_ptr - (uint64_t) (&MPIDI_XPMEM_cnt_mem_direct);
    } else {
        coop_counter_direct_flag = 0;
        coop_counter_offset = (uint64_t) counter_ptr;
    }

    /* Receiver sends CTS packet to sender */
    int rank = MPIDIG_REQUEST(rreq, rank);
    void *buf = (char *) MPIDIG_REQUEST(rreq, buffer) + dt_true_lb;
    mpi_errno = MPIDI_XPMEM_send_cts(rank, comm, buf, recv_data_sz, sreq_ptr, rreq,
                                     coop_counter_direct_flag, coop_counter_offset);
    MPIR_ERR_CHECK(mpi_errno);

    /* Set receive status */
    MPIR_STATUS_SET_COUNT(rreq->status, recv_data_sz);
    rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
    rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);
    dest_buf = MPIDIG_REQUEST(rreq, buffer);

    mpi_errno =
        MPIDI_XPMEM_seg_regist(src_lrank, src_data_sz, (void *) src_offset, &seg_ptr, &src_buf,
                               &MPIDI_XPMEM_global.segmaps[src_lrank].segcache_ubuf);
    MPIR_ERR_CHECK(mpi_errno);

    XPMEM_TRACE("handle_lmt_coop_recv: handle matched rreq %p [source %d, tag %d, context_id 0x%x],"
                " copy dst %p, src %lx, bytes %ld\n", rreq, MPIDIG_REQUEST(rreq, rank),
                MPIDIG_REQUEST(rreq, tag), MPIDIG_REQUEST(rreq, context_id),
                (char *) MPIDIG_REQUEST(rreq, buffer), src_offset, recv_data_sz);

    /* sender and receiver datatypes are both continuous, perform cooperative copy. */
    mpi_errno = MPIDI_XPMEM_do_recv(src_buf, recv_data_sz, (char *) dest_buf + dt_true_lb,
                                    &MPIDI_XPMEM_REQUEST(rreq, counter_ptr)->obj.offset,
                                    sreq_ptr, rreq);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDI_XPMEM_seg_deregist(seg_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_XPMEM_HANDLE_LMT_COOP_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_handle_lmt_single_recv(uint64_t src_offset,
                                                                size_t src_data_sz,
                                                                uint64_t sreq_ptr, int src_lrank,
                                                                MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_XPMEM_seg_t *seg_ptr = NULL;
    void *src_buf = NULL;
    size_t data_sz, recv_data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_XPMEM_HANDLE_LMT_SINGLE_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_XPMEM_HANDLE_LMT_SINGLE_RECV);

    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, datatype), MPIDIG_REQUEST(rreq, count), data_sz);

    /* Data truncation checking */
    recv_data_sz = MPL_MIN(src_data_sz, data_sz);
    if (src_data_sz > data_sz)
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;

    /* Set receive status */
    MPIR_STATUS_SET_COUNT(rreq->status, recv_data_sz);
    rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
    rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);
    MPIDI_XPMEM_REQUEST(rreq, counter_ptr) = NULL;

    mpi_errno =
        MPIDI_XPMEM_seg_regist(src_lrank, src_data_sz, (void *) src_offset, &seg_ptr, &src_buf,
                               &MPIDI_XPMEM_global.segmaps[src_lrank].segcache_ubuf);
    MPIR_ERR_CHECK(mpi_errno);

    XPMEM_TRACE("handle_lmt_single_recv: handle matched rreq %p [source %d, tag %d, "
                " context_id 0x%x], copy dst %p, src %lx, bytes %ld\n", rreq,
                MPIDIG_REQUEST(rreq, rank), MPIDIG_REQUEST(rreq, tag),
                MPIDIG_REQUEST(rreq, context_id), (char *) MPIDIG_REQUEST(rreq, buffer),
                src_offset, recv_data_sz);

    /* Copy data to receive buffer */
    mpi_errno = MPIR_Localcopy(src_buf, recv_data_sz, MPI_BYTE,
                               MPIDIG_REQUEST(rreq, buffer), MPIDIG_REQUEST(rreq, count),
                               MPIDIG_REQUEST(rreq, datatype));
    MPIR_ERR_CHECK(mpi_errno);

    int rank = MPIDIG_REQUEST(rreq, rank);
    MPIR_Comm *comm = MPIDIG_context_id_to_comm(MPIDIG_REQUEST(rreq, context_id));
    mpi_errno = MPIDI_XPMEM_send_recv_fin(rank, comm, sreq_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
    MPID_Request_complete(rreq);

    mpi_errno = MPIDI_XPMEM_seg_deregist(seg_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_XPMEM_HANDLE_LMT_SINGLE_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_handle_lmt_recv(uint64_t src_offset, size_t data_sz,
                                                         uint64_t sreq_ptr, int src_lrank,
                                                         MPIR_Comm * comm, MPIR_Request * rreq)
{

    int mpi_errno = MPI_SUCCESS;
    int recvtype_iscontig;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_XPMEM_HANDLE_LMT_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_XPMEM_HANDLE_LMT_RECV);
    /* TODO: need to support cooperative copy on non-contig datatype on receiver side. */
    MPIR_Datatype_is_contig(MPIDIG_REQUEST(rreq, datatype), &recvtype_iscontig);
    if (recvtype_iscontig && data_sz > MPIR_CVAR_CH4_XPMEM_COOP_COPY_THRESHOLD) {
        /* Cooperative XPMEM copy */
        mpi_errno =
            MPIDI_XPMEM_handle_lmt_coop_recv(src_offset, data_sz, sreq_ptr, src_lrank, comm, rreq);
    } else {
        mpi_errno =
            MPIDI_XPMEM_handle_lmt_single_recv(src_offset, data_sz, sreq_ptr, src_lrank, rreq);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_XPMEM_HANDLE_LMT_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* XPMEM_RECV_H_INCLUDED */
