/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2019 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "rndv.h"

/* Handle incoming ready to send (RTS) message by setting up a shared memory segment and enqueuing
 * the message into a list to progressed.
 *
 * ctrl_hdr - The struct containing the control message
 */
int MPIDI_POSIX_lmt_ctrl_send_lmt_rts_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *root_comm;

    MPIDI_SHM_lmt_rndv_long_msg_t *lmt_rndv_long_msg = &ctrl_hdr->lmt_rndv_long_msg;
    MPIR_Request *rreq = NULL, *anysource_partner;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_LMT_CTRL_SEND_LMT_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_LMT_CTRL_SEND_LMT_MSG_CB);

    /* Try to match a posted receive request.
     * root_comm cannot be NULL if a posted receive request exists, because
     * we increase its refcount at enqueue time. */
    root_comm = MPIDIG_context_id_to_comm(lmt_rndv_long_msg->context_id);

    if (root_comm) {
        int continue_matching = 1;
        while (TRUE) {
            rreq = MPIDIG_dequeue_posted(lmt_rndv_long_msg->rank, lmt_rndv_long_msg->tag,
                                         lmt_rndv_long_msg->context_id,
                                         &MPIDIG_COMM(root_comm, posted_list));

            if (rreq) {
                int is_cancelled;
                MPIDI_anysrc_try_cancel_partner(rreq, &is_cancelled);
                if (!is_cancelled) {
                    MPIR_Comm_release(root_comm);       /* -1 for posted_list */
                    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
                    continue;
                }

                /* NOTE: NM partner is freed at MPIDI_anysrc_try_cancel_partner, no need to call
                 * MPIDI_anysrc_free_partner during completion. */
            }
            break;
        }
    }

    if (rreq) {
        /* Matching receive was posted */
        MPIR_Comm_release(root_comm);   /* -1 for posted_list */
        rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank) = lmt_rndv_long_msg->rank;
        rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag) = lmt_rndv_long_msg->tag;
        MPIR_STATUS_SET_COUNT(rreq->status, lmt_rndv_long_msg->data_sz);
        MPIDIG_REQUEST(rreq, count) = lmt_rndv_long_msg->data_sz;
        MPIDIG_REQUEST(rreq, context_id) = lmt_rndv_long_msg->context_id;
        MPIDIG_REQUEST(rreq, error_bits) = lmt_rndv_long_msg->error_bits;
        MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr) = (MPIR_Request *) lmt_rndv_long_msg->sreq_ptr;
        /* This is a posted receive -- no need to cleanup rreq.match_req when it completes. */
        MPIDIG_REQUEST(rreq, req->rreq.match_req) = NULL;
        if (lmt_rndv_long_msg->handler_id == MPIDIG_SSEND_REQ)
            MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_PEER_SSEND;
        MPIDI_REQUEST(rreq, is_local) = 1;
        rreq->status.MPI_ERROR = MPI_SUCCESS;

        MPIDI_POSIX_AMREQUEST(rreq, lmt.lmt_data_sz) = lmt_rndv_long_msg->data_sz;
        MPIDI_POSIX_AMREQUEST(rreq, lmt.lmt_msg_offset) = 0;
        MPIDI_POSIX_AMREQUEST(rreq, lmt.lmt_buf_num) = 0;
        MPIDI_POSIX_AMREQUEST(rreq, lmt.lmt_leftover) = 0;

        /* Complete setting up pipelined receive */
        mpi_errno = MPIDI_POSIX_lmt_recv(rreq);

        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        /* Enqueue unexpected receive request */
        rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RECV, 2);
        MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

        /* store CH4 am rreq info */
        MPIDIG_REQUEST(rreq, buffer) = NULL;
        rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank) = lmt_rndv_long_msg->rank;
        rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag) = lmt_rndv_long_msg->tag;
        MPIR_STATUS_SET_COUNT(rreq->status, lmt_rndv_long_msg->data_sz);
        MPIDIG_REQUEST(rreq, count) = lmt_rndv_long_msg->data_sz;
        MPIDIG_REQUEST(rreq, context_id) = lmt_rndv_long_msg->context_id;
        MPIDIG_REQUEST(rreq, error_bits) = lmt_rndv_long_msg->error_bits;
        MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr) = (MPIR_Request *) lmt_rndv_long_msg->sreq_ptr;
        if (lmt_rndv_long_msg->handler_id == MPIDIG_SSEND_REQ)
            MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_PEER_SSEND;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_LONG_RTS;
        MPIDI_REQUEST(rreq, is_local) = 1;
        rreq->status.MPI_ERROR = MPI_SUCCESS;

        MPIDI_POSIX_AMREQUEST(rreq, lmt.lmt_data_sz) = lmt_rndv_long_msg->data_sz;
        MPIDI_POSIX_AMREQUEST(rreq, lmt.lmt_msg_offset) = 0;
        MPIDI_POSIX_AMREQUEST(rreq, lmt.lmt_buf_num) = 0;
        MPIDI_POSIX_AMREQUEST(rreq, lmt.lmt_leftover) = 0;

        if (root_comm) {
            MPIR_Comm_add_ref(root_comm);       /* +1 for unexp_list */
            MPIDIG_enqueue_unexp(rreq, &MPIDIG_COMM(root_comm, unexp_list));
        } else {
            MPIDIG_enqueue_unexp(rreq,
                                 MPIDIG_context_id_to_uelist(MPIDIG_REQUEST(rreq, context_id)));
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_LMT_CTRL_SEND_LMT_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Handle incoming clear to send (CTS) message by setting up a shared memory segment and enqueuing
 * the message into a list to progressed.
 *
 * ctrl_hdr - The struct containing the control message
 */
int MPIDI_POSIX_lmt_ctrl_send_lmt_cts_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_LMT_CTRL_SEND_LMT_ACK_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_LMT_CTRL_SEND_LMT_ACK_CB);

    MPIR_CHKPMEM_DECL(1);

    MPIR_Request *sreq = (MPIR_Request *) ctrl_hdr->lmt_rndv_long_ack.sreq_ptr;
    int serialized_handle_len = ctrl_hdr->lmt_rndv_long_ack.copy_buf_serialized_handle_len;
    char *serialized_handle = ctrl_hdr->lmt_rndv_long_ack.copy_buf_serialized_handle;
    const int grank = MPIDIU_rank_to_lpid(MPIDIG_REQUEST(sreq, rank),
                                          MPIDIG_context_id_to_comm(MPIDIG_REQUEST(sreq,
                                                                                   context_id)));
    int receiver_local_rank = MPIDI_POSIX_global.local_ranks[grank];

    /* Check to see if this copy buffer has been set up before and do so if not */
    if (NULL == rndv_send_copy_bufs[receiver_local_rank]) {
        mpi_errno = MPL_shm_hnd_deserialize(rndv_send_copy_buf_handles[receiver_local_rank],
                                            serialized_handle, serialized_handle_len);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPL_shm_seg_attach(rndv_send_copy_buf_handles[receiver_local_rank],
                                       sizeof(lmt_rndv_copy_buf_t),
                                       (void **) &rndv_send_copy_bufs[receiver_local_rank], 0);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    /* Create a new "wait element" for this lmt transfer in case more than one is started at the
     * same time */
    lmt_rndv_wait_element_t *element;
    MPIR_CHKPMEM_MALLOC(element, lmt_rndv_wait_element_t *, sizeof(lmt_rndv_wait_element_t),
                        mpi_errno, "LMT RNDV wait element", MPL_MEM_SHM);
    element->progress = lmt_rndv_send_progress;
    element->req = sreq;
    GENERIC_Q_ENQUEUE(&rndv_send_queues[receiver_local_rank], element, next);

    /* If this is an ssend, mark that the request has synchronized */
    if (ctrl_hdr->lmt_rndv_long_ack.handler_id & MPIDIG_SSEND_REQ)
        MPID_Request_complete(sreq);

    /* TODO - Create a queue where in progress messages go to shorten the progress polling time. */

    /* Try to make progress on lmt messages before queuing the new one */
    mpi_errno = MPIDI_POSIX_lmt_progress();
    /* Want to capture the output of the progress function, but not return an error immediately
     * because just kicking the progress engine may not indicate a problem with this particular
     * receive. */

    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_LMT_CTRL_SEND_LMT_ACK_CB);
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}
