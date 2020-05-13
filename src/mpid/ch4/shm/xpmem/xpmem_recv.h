/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_do_lmt_coop_copy(const void *src_buf,
                                                          size_t data_sz, void *dest_buf,
                                                          MPL_atomic_int_t * counter_ptr,
                                                          uint64_t req_ptr,
                                                          MPIR_Request * local_req, int *fin_type,
                                                          int *copy_type)
{
    int mpi_errno = MPI_SUCCESS;
    size_t copy_sz;
    int cur_chunk = 0, total_chunk = 0, num_local_copy = 0;
    uint64_t cur_offset;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_DO_LMT_COOP_COPY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_DO_LMT_COOP_COPY);

    total_chunk =
        data_sz / MPIR_CVAR_CH4_XPMEM_COOP_COPY_CHUNK_SIZE +
        (data_sz % MPIR_CVAR_CH4_XPMEM_COOP_COPY_CHUNK_SIZE == 0 ? 0 : 1);

    MPIR_Assert(total_chunk > 1);

    /* TODO: implement OPA_fetch_and_incr_int uint64_t to directly change cur_offset
     * instead of computing it in each copy */
    while ((cur_chunk = MPL_atomic_fetch_add_int(counter_ptr, 1)) < total_chunk) {
        cur_offset = ((uint64_t) cur_chunk) * MPIR_CVAR_CH4_XPMEM_COOP_COPY_CHUNK_SIZE;
        copy_sz =
            cur_offset + MPIR_CVAR_CH4_XPMEM_COOP_COPY_CHUNK_SIZE <=
            data_sz ? MPIR_CVAR_CH4_XPMEM_COOP_COPY_CHUNK_SIZE : data_sz - cur_offset;
        mpi_errno =
            MPIR_Localcopy(((char *) src_buf + cur_offset), copy_sz, MPI_BYTE,
                           ((char *) dest_buf + cur_offset), copy_sz, MPI_BYTE);
        MPIR_ERR_CHECK(mpi_errno);
        num_local_copy++;
    }

    if (cur_chunk == total_chunk)
        *fin_type = MPIDI_XPMEM_LOCAL_FIN;      /* copy is only locally complete */
    else
        *fin_type = MPIDI_XPMEM_BOTH_FIN;       /* copy is done by both sides */

    if (num_local_copy == total_chunk)
        *copy_type = MPIDI_XPMEM_COPY_ALL;      /* the process copies all chunks */
    else if (num_local_copy == 0)
        *copy_type = MPIDI_XPMEM_COPY_ZERO;     /* the process copies zero chunk */
    else
        *copy_type = MPIDI_XPMEM_COPY_MIX;      /* both processes copy a part of chunks */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_DO_LMT_COOP_COPY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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
    int fin_type, copy_type;
    MPIDI_SHM_ctrl_hdr_t ctrl_hdr;
    MPIDI_SHM_ctrl_xpmem_send_lmt_cts_t *slmt_cts_hdr = &ctrl_hdr.xpmem_slmt_cts;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_HANDLE_LMT_COOP_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_HANDLE_LMT_COOP_RECV);

    MPIDI_Datatype_check_size_lb(MPIDIG_REQUEST(rreq, datatype), MPIDIG_REQUEST(rreq, count),
                                 data_sz, dt_true_lb);

    /* Data truncation checking */
    recv_data_sz = MPL_MIN(src_data_sz, data_sz);
    if (src_data_sz > data_sz)
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;

    /* XPMEM internal info */
    slmt_cts_hdr->dest_offset = (uint64_t) MPIDIG_REQUEST(rreq, buffer) + dt_true_lb;
    slmt_cts_hdr->data_sz = recv_data_sz;
    slmt_cts_hdr->dest_lrank = MPIDI_XPMEM_global.local_rank;

    /* Receiver replies CTS packet */
    slmt_cts_hdr->sreq_ptr = sreq_ptr;
    slmt_cts_hdr->rreq_ptr = (uint64_t) rreq;

    /* allocate shm counter */
    MPIDI_XPMEM_REQUEST(rreq, counter_ptr) =
        (MPIDI_XPMEM_cnt_t *) MPIR_Handle_obj_alloc(&MPIDI_XPMEM_cnt_mem);
    MPL_atomic_store_int(&MPIDI_XPMEM_REQUEST(rreq, counter_ptr)->obj.counter, 0);
    if (HANDLE_GET_KIND(MPIDI_XPMEM_REQUEST(rreq, counter_ptr)->obj.handle) == HANDLE_KIND_DIRECT) {
        slmt_cts_hdr->coop_counter_direct_flag = 1;
        slmt_cts_hdr->coop_counter_offset =
            (uint64_t) MPIDI_XPMEM_REQUEST(rreq, counter_ptr) -
            (uint64_t) (&MPIDI_XPMEM_cnt_mem_direct);
    } else {
        slmt_cts_hdr->coop_counter_direct_flag = 0;
        slmt_cts_hdr->coop_counter_offset = (uint64_t) MPIDI_XPMEM_REQUEST(rreq, counter_ptr);
    }

    XPMEM_TRACE("handle_lmt_coop_recv: shm ctrl_id %d, buf_offset 0x%lx, data_sz 0x%lx, "
                "sreq_ptr 0x%lx, rreq_ptr 0x%lx, coop_counter_offset 0x%lx, "
                " local_rank %d, dest %d\n",
                MPIDI_SHM_XPMEM_SEND_LMT_CTS, slmt_cts_hdr->dest_offset, slmt_cts_hdr->data_sz,
                slmt_cts_hdr->sreq_ptr, slmt_cts_hdr->rreq_ptr, slmt_cts_hdr->coop_counter_offset,
                slmt_cts_hdr->dest_lrank, MPIDIG_REQUEST(rreq, rank));

    /* Receiver sends CTS packet to sender */
    mpi_errno =
        MPIDI_SHM_do_ctrl_send(MPIDIG_REQUEST(rreq, rank), comm, MPIDI_SHM_XPMEM_SEND_LMT_CTS,
                               &ctrl_hdr);
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
    mpi_errno =
        MPIDI_XPMEM_do_lmt_coop_copy(src_buf, recv_data_sz,
                                     (char *) dest_buf + dt_true_lb,
                                     &MPIDI_XPMEM_REQUEST(rreq, counter_ptr)->obj.counter, sreq_ptr,
                                     rreq, &fin_type, &copy_type);
    MPIR_ERR_CHECK(mpi_errno);

    /* - For receiver:
     *     case copy_type == LOCAL_FIN:
     *        case fin_type == COPY_ALL: complete rreq and free counter obj when receiving FREE_CNT;
     *        case fin_type == COPY_ZERO: waits SEND_FIN from sender to complete rreq and free counter obj;
     *                                    receiver needs to be notified the completion of sender
     *        case fin_type == MIXED_COPIED: waits SEND_FIN from sender to complete rreq and free counter obj;
     *                                       receiver needs to be notified the completion of sender
     *     case copy_type == BOTH_FIN:
     *        case fin_type == COPY_ALL: complete rreq and free counter obj; send RECV_FIN to sender
     *        case fin_type == COPY_ZERO: complete rreq and free counter obj
     *        case fin_type == MIXED_COPIED: complete rreq and free counter obj; send RECV_FIN to sender */
    if ((fin_type == MPIDI_XPMEM_LOCAL_FIN && copy_type == MPIDI_XPMEM_COPY_ALL) ||
        fin_type == MPIDI_XPMEM_BOTH_FIN) {
        if (fin_type == MPIDI_XPMEM_BOTH_FIN) {
            if (copy_type != MPIDI_XPMEM_COPY_ZERO) {
                MPIDI_SHM_ctrl_hdr_t ack_ctrl_hdr;
                MPIDI_SHM_ctrl_xpmem_send_lmt_recv_fin_t *slmt_fin_hdr =
                    &ack_ctrl_hdr.xpmem_slmt_recv_fin;
                slmt_fin_hdr->req_ptr = sreq_ptr;
                mpi_errno = MPIDI_SHM_do_ctrl_send(MPIDIG_REQUEST(rreq, rank),
                                                   MPIDIG_context_id_to_comm(MPIDIG_REQUEST
                                                                             (rreq, context_id)),
                                                   MPIDI_SHM_XPMEM_SEND_LMT_RECV_FIN,
                                                   &ack_ctrl_hdr);
                MPIR_ERR_CHECK(mpi_errno);
            }
            MPIR_Handle_obj_free(&MPIDI_XPMEM_cnt_mem, MPIDI_XPMEM_REQUEST(rreq, counter_ptr));
        } else {
            int c;
            MPIR_cc_incr(&MPIDI_XPMEM_global.num_pending_cnt, &c);
        }
        MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
        MPID_Request_complete(rreq);
    }

    mpi_errno = MPIDI_XPMEM_seg_deregist(seg_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_HANDLE_LMT_COOP_RECV);
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
    MPIDI_SHM_ctrl_hdr_t ack_ctrl_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_HANDLE_LMT_SINGLE_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_HANDLE_LMT_SINGLE_RECV);

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

    ack_ctrl_hdr.xpmem_slmt_send_fin.req_ptr = sreq_ptr;
    mpi_errno = MPIDI_SHM_do_ctrl_send(MPIDIG_REQUEST(rreq, rank),
                                       MPIDIG_context_id_to_comm(MPIDIG_REQUEST
                                                                 (rreq, context_id)),
                                       MPIDI_SHM_XPMEM_SEND_LMT_RECV_FIN, &ack_ctrl_hdr);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
    MPID_Request_complete(rreq);

    mpi_errno = MPIDI_XPMEM_seg_deregist(seg_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_HANDLE_LMT_SINGLE_RECV);
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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_HANDLE_LMT_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_HANDLE_LMT_RECV);
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_HANDLE_LMT_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* XPMEM_RECV_H_INCLUDED */
