/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "xpmem_pre.h"
#include "xpmem_impl.h"
#include "xpmem_recv.h"
#include "xpmem_control.h"

int MPIDI_XPMEM_ctrl_send_lmt_recv_fin_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = (MPIR_Request *) ctrl_hdr->xpmem_slmt_recv_fin.req_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_CTRL_SEND_LMT_RECV_FIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_CTRL_SEND_LMT_RECV_FIN_CB);

    XPMEM_TRACE("send_lmt_recv_fin_cb: complete sreq %p\n", sreq);
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(sreq, datatype));
    MPID_Request_complete(sreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_CTRL_SEND_LMT_RECV_FIN_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_XPMEM_ctrl_send_lmt_send_fin_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = (MPIR_Request *) ctrl_hdr->xpmem_slmt_send_fin.req_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_CTRL_SEND_LMT_SEND_FIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_CTRL_SEND_LMT_SEND_FIN_CB);

    XPMEM_TRACE("send_lmt_send_fin_cb: complete rreq %p\n", rreq);

    /* send_fin_cb can only be triggered in cooperative copy, so
     * MPIDI_XPMEM_REQUEST(rreq, counter_ptr) must be set by receiver */
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
    MPIR_Handle_obj_free(&MPIDI_XPMEM_cnt_mem, MPIDI_XPMEM_REQUEST(rreq, counter_ptr));
    MPID_Request_complete(rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_CTRL_SEND_LMT_SEND_FIN_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_XPMEM_ctrl_send_lmt_rts_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_SHM_ctrl_xpmem_send_lmt_rts_t *slmt_rts_hdr = &ctrl_hdr->xpmem_slmt_rts;
    MPIR_Request *rreq = NULL;
    MPIR_Comm *root_comm;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_CTRL_SEND_LMT_RTS_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_CTRL_SEND_LMT_RTS_CB);

    XPMEM_TRACE("send_lmt_rts_cb: received src_offset 0x%lx, data_sz 0x%lx, sreq_ptr 0x%lx, "
                "src_lrank %d, match info[src_rank %d, tag %d, context_id 0x%x]\n",
                slmt_rts_hdr->src_offset, slmt_rts_hdr->data_sz, slmt_rts_hdr->sreq_ptr,
                slmt_rts_hdr->src_lrank, slmt_rts_hdr->src_rank, slmt_rts_hdr->tag,
                slmt_rts_hdr->context_id);

    /* Try to match a posted receive request.
     * root_comm cannot be NULL if a posted receive request exists, because
     * we increase its refcount at enqueue time. */
    root_comm = MPIDIG_context_id_to_comm(slmt_rts_hdr->context_id);
    if (root_comm) {
        while (TRUE) {
            rreq = MPIDIG_dequeue_posted(slmt_rts_hdr->src_rank, slmt_rts_hdr->tag,
                                         slmt_rts_hdr->context_id, 1,
                                         &MPIDIG_COMM(root_comm, posted_list));
#ifndef MPIDI_CH4_DIRECT_NETMOD
            if (rreq) {
                int is_cancelled;
                MPIDI_anysrc_try_cancel_partner(rreq, &is_cancelled);
                if (!is_cancelled) {
                    MPIR_Comm_release(root_comm);       /* -1 for posted_list */
                    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
                    continue;
                }
                /* NOTE: NM partner is freed at MPIDI_anysrc_try_cancel_partner,
                 * no need to call MPIDI_anysrc_free_partner at completions
                 */
            }
#endif
            break;
        }
    }

    if (rreq) {
        /* Matching receive was posted */
        MPIR_Comm_release(root_comm);   /* -1 for posted_list */
        MPIDIG_REQUEST(rreq, rank) = slmt_rts_hdr->src_rank;
        MPIDIG_REQUEST(rreq, tag) = slmt_rts_hdr->tag;
        MPIDIG_REQUEST(rreq, context_id) = slmt_rts_hdr->context_id;

        /* Complete XPMEM receive */
        mpi_errno = MPIDI_XPMEM_handle_lmt_recv(slmt_rts_hdr->src_offset, slmt_rts_hdr->data_sz,
                                                slmt_rts_hdr->sreq_ptr, slmt_rts_hdr->src_lrank,
                                                root_comm, rreq);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* Enqueue unexpected receive request */
        rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RECV, 2);
        MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

        /* store CH4 am rreq info */
        MPIDIG_REQUEST(rreq, buffer) = NULL;
        MPIDIG_REQUEST(rreq, datatype) = MPI_BYTE;
        MPIDIG_REQUEST(rreq, count) = slmt_rts_hdr->data_sz;
        MPIDIG_REQUEST(rreq, rank) = slmt_rts_hdr->src_rank;
        MPIDIG_REQUEST(rreq, tag) = slmt_rts_hdr->tag;
        MPIDIG_REQUEST(rreq, context_id) = slmt_rts_hdr->context_id;
        MPIDI_REQUEST(rreq, is_local) = 1;

        /* store XPMEM internal info */
        MPIDI_XPMEM_REQUEST(rreq, unexp_rreq).src_offset = slmt_rts_hdr->src_offset;
        MPIDI_XPMEM_REQUEST(rreq, unexp_rreq).data_sz = slmt_rts_hdr->data_sz;
        MPIDI_XPMEM_REQUEST(rreq, unexp_rreq).sreq_ptr = slmt_rts_hdr->sreq_ptr;
        MPIDI_XPMEM_REQUEST(rreq, unexp_rreq).src_lrank = slmt_rts_hdr->src_lrank;
        MPIDI_SHM_REQUEST(rreq, status) |= MPIDI_SHM_REQ_XPMEM_SEND_LMT;

        if (root_comm) {
            MPIR_Comm_add_ref(root_comm);       /* +1 for unexp_list */
            MPIDIG_enqueue_unexp(rreq, &MPIDIG_COMM(root_comm, unexp_list));
        } else {
            MPIDIG_enqueue_unexp(rreq,
                                 MPIDIG_context_id_to_uelist(MPIDIG_REQUEST(rreq, context_id)));
        }

        XPMEM_TRACE("send_lmt_rts_cb: enqueue unexpected, rreq=%p\n", rreq);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_CTRL_SEND_LMT_RTS_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_XPMEM_ctrl_send_lmt_cts_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    MPIDI_SHM_ctrl_xpmem_send_lmt_cts_t *slmt_cts_hdr = &ctrl_hdr->xpmem_slmt_cts;
    MPIDI_XPMEM_seg_t *seg_ptr = NULL;
    MPIDI_XPMEM_seg_t *counter_seg_ptr = NULL;
    void *dest_buf = NULL;
    void *src_buf = NULL;
    size_t data_sz ATTRIBUTE((unused));
    MPI_Aint dt_true_lb;
    MPIDI_XPMEM_cnt_t *counter_ptr = NULL;
    int fin_type, copy_type;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_CTRL_SEND_LMT_CTS_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_CTRL_SEND_LMT_CTS_CB);

    /* Sender gets the CTS packet from receiver and need to perform copy */
    sreq = (MPIR_Request *) slmt_cts_hdr->sreq_ptr;

    mpi_errno =
        MPIDI_XPMEM_seg_regist(slmt_cts_hdr->dest_lrank, slmt_cts_hdr->data_sz,
                               (void *) slmt_cts_hdr->dest_offset, &seg_ptr, &dest_buf,
                               &MPIDI_XPMEM_global.segmaps[slmt_cts_hdr->dest_lrank].segcache_ubuf);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_Datatype_check_size_lb(MPIDIG_REQUEST(sreq, datatype), MPIDIG_REQUEST(sreq, count),
                                 data_sz, dt_true_lb);

    src_buf = (void *) ((char *) MPIDIG_REQUEST(sreq, buffer) + dt_true_lb);

    if (slmt_cts_hdr->coop_counter_direct_flag) {
        counter_ptr = (MPIDI_XPMEM_cnt_t *) ((char *)
                                             MPIDI_XPMEM_global.
                                             coop_counter_direct[slmt_cts_hdr->dest_lrank] +
                                             slmt_cts_hdr->coop_counter_offset);
    } else {
        MPIDI_XPMEM_seg_regist(slmt_cts_hdr->dest_lrank,
                               sizeof(MPIDI_XPMEM_cnt_t),
                               (void *) slmt_cts_hdr->coop_counter_offset,
                               &counter_seg_ptr, (void **) &counter_ptr,
                               &MPIDI_XPMEM_global.segmaps[slmt_cts_hdr->dest_lrank].segcache_cnt);
    }

    /* sender and receiver datatypes are both continuous, perform cooperative copy. */
    mpi_errno =
        MPIDI_XPMEM_do_lmt_coop_copy(src_buf, slmt_cts_hdr->data_sz, dest_buf,
                                     &counter_ptr->obj.counter, slmt_cts_hdr->rreq_ptr, sreq,
                                     &fin_type, &copy_type);
    MPIR_ERR_CHECK(mpi_errno);

    /* - For sender:
     *     case copy_type == LOCAL_FIN:
     *        case fin_type == COPY_ALL: complete sreq; receiver complete rreq without sending RECV_FIN
     *        case fin_type == COPY_ZERO: waits RECV_FIN from receiver to complete sreq;
     *                                    receiver detects completion of copy, send RECV_FIN to sender
     *        case fin_type == MIXED_COPIED: waits RECV_FIN from receiver to complete sreq;
     *                                       receiver detects completion of copy, send RECV_FIN to sender
     *     case copy_type == BOTH_FIN:
     *        case fin_type == COPY_ALL: sends SEND_FIN to receiver and complete sreq
     *        case fin_type == COPY_ZERO: sends FREE_CNT to receiver with counter info and complete sreq;
     *                                    receiver does not free counter obj and needs to know sender
     *                                    finishes cooperative copy
     *        case fin_type == MIXED_COPIED: sends SEND_FIN to receiver and complete sreq;
     *                                       receiver is waiting SEND_FIN to complete rreq */
    if ((fin_type == MPIDI_XPMEM_LOCAL_FIN && copy_type == MPIDI_XPMEM_COPY_ALL) ||
        fin_type == MPIDI_XPMEM_BOTH_FIN) {
        if (fin_type == MPIDI_XPMEM_BOTH_FIN) {
            int ctrl_id;
            MPIDI_SHM_ctrl_hdr_t ack_ctrl_hdr;

            if (copy_type == MPIDI_XPMEM_COPY_ZERO) {
                ack_ctrl_hdr.xpmem_slmt_cnt_free.coop_counter_direct_flag =
                    slmt_cts_hdr->coop_counter_direct_flag;
                ack_ctrl_hdr.xpmem_slmt_cnt_free.coop_counter_offset =
                    slmt_cts_hdr->coop_counter_offset;
                ctrl_id = MPIDI_SHM_XPMEM_SEND_LMT_CNT_FREE;
            } else {
                ack_ctrl_hdr.xpmem_slmt_send_fin.req_ptr = slmt_cts_hdr->rreq_ptr;
                ctrl_id = MPIDI_SHM_XPMEM_SEND_LMT_SEND_FIN;
            }

            mpi_errno = MPIDI_SHM_do_ctrl_send(MPIDIG_REQUEST(sreq, rank),
                                               MPIDIG_context_id_to_comm(MPIDIG_REQUEST
                                                                         (sreq, context_id)),
                                               ctrl_id, &ack_ctrl_hdr);
            MPIR_ERR_CHECK(mpi_errno);
        }
        MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(sreq, datatype));
        MPID_Request_complete(sreq);
    }

    mpi_errno = MPIDI_XPMEM_seg_deregist(seg_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    if (counter_seg_ptr) {
        mpi_errno = MPIDI_XPMEM_seg_deregist(counter_seg_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_CTRL_SEND_LMT_CTS_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_XPMEM_ctrl_send_lmt_cnt_free_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIDI_XPMEM_cnt_t *counter_ptr;
    MPIDI_SHM_ctrl_xpmem_send_lmt_cnt_free_t *xpmem_slmt_cnt_free = &ctrl_hdr->xpmem_slmt_cnt_free;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_CTRL_SEND_LMT_CNT_FREE_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_CTRL_SEND_LMT_CNT_FREE_CB);

    if (xpmem_slmt_cnt_free->coop_counter_direct_flag)
        counter_ptr = (MPIDI_XPMEM_cnt_t *) ((char *) MPIDI_XPMEM_cnt_mem_direct +
                                             xpmem_slmt_cnt_free->coop_counter_offset);
    else
        counter_ptr = (MPIDI_XPMEM_cnt_t *) xpmem_slmt_cnt_free->coop_counter_offset;
    MPIR_Handle_obj_free(&MPIDI_XPMEM_cnt_mem, counter_ptr);
    MPIR_cc_decr(&MPIDI_XPMEM_global.num_pending_cnt, &c);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_CTRL_SEND_LMT_CNT_FREE_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
