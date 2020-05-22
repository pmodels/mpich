/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "ch4r_callbacks.h"

static int handle_unexp_cmpl(MPIR_Request * rreq);
static int recv_target_cmpl_cb(MPIR_Request * rreq);

int MPIDIG_do_long_ack(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDIG_send_long_ack_msg_t am_hdr;
    am_hdr.sreq_ptr = (MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr));
    am_hdr.rreq_ptr = rreq;
    MPIR_Assert((void *) am_hdr.sreq_ptr != NULL);

    int rank = MPIDIG_REQUEST(rreq, rank);
    MPIR_Comm *comm = MPIDIG_context_id_to_comm(MPIDIG_REQUEST(rreq, context_id));
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_am_send_hdr(rank, comm, MPIDIG_SEND_LONG_ACK, &am_hdr, sizeof(am_hdr));
#else
    if (MPIDI_REQUEST(rreq, is_local)) {
        mpi_errno = MPIDI_SHM_am_send_hdr(rank, comm, MPIDIG_SEND_LONG_ACK,
                                          &am_hdr, sizeof(am_hdr));
    } else {
        mpi_errno = MPIDI_NM_am_send_hdr(rank, comm, MPIDIG_SEND_LONG_ACK, &am_hdr, sizeof(am_hdr));
    }
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Checks to make sure that the specified request is the next one expected to finish. If it isn't
 * supposed to finish next, it is appended to a list of requests to be retrieved later. */
int MPIDIG_check_cmpl_order(MPIR_Request * req)
{
    int ret = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_CHECK_CMPL_ORDER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_CHECK_CMPL_ORDER);

    if (MPIDIG_REQUEST(req, req->seq_no) == MPL_atomic_load_uint64(&MPIDI_global.exp_seq_no)) {
        MPL_atomic_fetch_add_uint64(&MPIDI_global.exp_seq_no, 1);
        ret = 1;
        goto fn_exit;
    }

    MPIDIG_REQUEST(req, req->request) = req;
    /* MPIDI_CS_ENTER(); */
    DL_APPEND(MPIDI_global.cmpl_list, req->dev.ch4.am.req);
    /* MPIDI_CS_EXIT(); */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_CHECK_CMPL_ORDER);
    return ret;
}

void MPIDIG_progress_compl_list(void)
{
    MPIR_Request *req;
    MPIDIG_req_ext_t *curr, *tmp;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PROGRESS_COMPL_LIST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PROGRESS_COMPL_LIST);

    /* MPIDI_CS_ENTER(); */
  do_check_again:
    DL_FOREACH_SAFE(MPIDI_global.cmpl_list, curr, tmp) {
        if (curr->seq_no == MPL_atomic_load_uint64(&MPIDI_global.exp_seq_no)) {
            DL_DELETE(MPIDI_global.cmpl_list, curr);
            req = (MPIR_Request *) curr->request;
            MPIDIG_REQUEST(req, req->target_cmpl_cb) (req);
            goto do_check_again;
        }
    }
    /* MPIDI_CS_EXIT(); */
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PROGRESS_COMPL_LIST);
}

static int handle_unexp_cmpl(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, in_use;
    MPIR_Request *match_req = NULL;
    size_t nbytes;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    size_t dt_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_HANDLE_UNEXP_CMPL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_HANDLE_UNEXP_CMPL);

    /* Check if this message has already been claimed by mprobe. */
    /* MPIDI_CS_ENTER(); */
    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_UNEXP_DQUED) {
        /* This request has been claimed by mprobe */
        if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_UNEXP_CLAIMED) {
            /* mrecv has been already called */
            MPIDIG_handle_unexp_mrecv(rreq);
        } else {
            /* mrecv has not been called yet -- just take out the busy flag so that
             * mrecv in future knows this request is ready */
            MPIDIG_REQUEST(rreq, req->status) &= ~MPIDIG_REQ_BUSY;
        }
        /* MPIDI_CS_EXIT(); */
        goto fn_exit;
    }
    /* MPIDI_CS_EXIT(); */

    /* If this request was previously matched, but not handled */
    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_MATCHED) {
        match_req = (MPIR_Request *) MPIDIG_REQUEST(rreq, req->rreq.match_req);

#ifndef MPIDI_CH4_DIRECT_NETMOD
        int is_cancelled;
        mpi_errno = MPIDI_anysrc_try_cancel_partner(match_req, &is_cancelled);
        MPIR_ERR_CHECK(mpi_errno);
        /* `is_cancelled` is assumed to be always true.
         * In typical config, anysrc partners won't occur if matching unexpected
         * message already exist.
         * In workq setup, since we will always progress shm first, when unexpected
         * message match, the NM partner wouldn't have progressed yet, so the cancel
         * should always succeed.
         */
        MPIR_Assert(is_cancelled);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

    /* If we didn't match the request, unmark the busy bit and skip the data movement below. */
    if (!match_req) {
        MPIDIG_REQUEST(rreq, req->status) &= ~MPIDIG_REQ_BUSY;
        goto fn_exit;
    }

    match_req->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
    match_req->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);

    /* Figure out how much data needs to be moved. */
    MPIDI_Datatype_get_info(MPIDIG_REQUEST(match_req, count),
                            MPIDIG_REQUEST(match_req, datatype),
                            dt_contig, dt_sz, dt_ptr, dt_true_lb);
    MPIR_Datatype_get_size_macro(MPIDIG_REQUEST(match_req, datatype), dt_sz);

    /* Make sure this request has the right amount of data in it. */
    if (MPIDIG_REQUEST(rreq, count) > dt_sz * MPIDIG_REQUEST(match_req, count)) {
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        nbytes = dt_sz * MPIDIG_REQUEST(match_req, count);
    } else {
        rreq->status.MPI_ERROR = MPI_SUCCESS;
        nbytes = MPIDIG_REQUEST(rreq, count);   /* incoming message is always count of bytes. */
    }

    MPIR_STATUS_SET_COUNT(match_req->status, nbytes);
    MPIDIG_REQUEST(rreq, count) = dt_sz > 0 ? nbytes / dt_sz : 0;

    /* Perform the data copy (using the datatype engine if necessary for non-contig transfers) */
    if (!dt_contig) {
        MPI_Aint actual_unpack_bytes;
        mpi_errno = MPIR_Typerep_unpack(MPIDIG_REQUEST(rreq, buffer), nbytes,
                                        MPIDIG_REQUEST(match_req, buffer),
                                        MPIDIG_REQUEST(match_req, count),
                                        MPIDIG_REQUEST(match_req, datatype), 0,
                                        &actual_unpack_bytes);
        MPIR_ERR_CHECK(mpi_errno);

        if (actual_unpack_bytes != (MPI_Aint) nbytes) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                             __FUNCTION__, __LINE__,
                                             MPI_ERR_TYPE, "**dtypemismatch", 0);
            match_req->status.MPI_ERROR = mpi_errno;
        }
    } else {
        MPIR_Memcpy((char *) MPIDIG_REQUEST(match_req, buffer) + dt_true_lb,
                    MPIDIG_REQUEST(rreq, buffer), nbytes);
    }

    /* Now that the unexpected message has been completed, unset the status bit. */
    MPIDIG_REQUEST(rreq, req->status) &= ~MPIDIG_REQ_UNEXPECTED;

    /* If this is a synchronous send, send the reply back to the sender to unlock them. */
    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_PEER_SSEND) {
        mpi_errno = MPIDIG_reply_ssend(rreq);
        MPIR_ERR_CHECK(mpi_errno);
    }
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_anysrc_free_partner(match_req);
#endif

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(match_req, datatype));
    MPL_free(MPIDIG_REQUEST(rreq, buffer));
    MPIR_Object_release_ref(rreq, &in_use);
    MPID_Request_complete(rreq);
    MPID_Request_complete(match_req);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_HANDLE_UNEXP_CMPL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This function is called when a receive has completed on the receiver side. The input is the
 * request that has been completed. */
static int recv_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_RECV_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_RECV_TARGET_CMPL_CB);

    /* Check if this request is supposed to complete next or if it should be delayed. */
    if (!MPIDIG_check_cmpl_order(rreq))
        return mpi_errno;

    MPIDIG_recv_finish(rreq);

    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_UNEXPECTED) {
        mpi_errno = handle_unexp_cmpl(rreq);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
    rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);

    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_PEER_SSEND) {
        mpi_errno = MPIDIG_reply_ssend(rreq);
        MPIR_ERR_CHECK(mpi_errno);
    }
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_anysrc_free_partner(rreq);
#endif

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
    if ((MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_LONG_RTS) &&
        MPIDIG_REQUEST(rreq, req->rreq.match_req) != NULL) {
        /* This block is executed only when the receive is enqueued (handoff) &&
         * receive was matched with an unexpected long RTS message.
         * `rreq` is the unexpected message received and `sigreq` is the message
         * that came from CH4 (e.g. MPIDI_recv_safe) */
        MPIR_Request *sigreq = MPIDIG_REQUEST(rreq, req->rreq.match_req);
        sigreq->status = rreq->status;
        MPIR_Request_add_ref(sigreq);
        MPID_Request_complete(sigreq);
        /* Free the unexpected request on behalf of the user */
        MPIR_Request_free(rreq);
    }
    MPID_Request_complete(rreq);
  fn_exit:
    MPIDIG_progress_compl_list();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_RECV_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_send_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_SEND_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_SEND_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_SEND_ORIGIN_CB);
    return mpi_errno;
}

int MPIDIG_send_long_lmt_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_SEND_LONG_LMT_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_SEND_LONG_LMT_ORIGIN_CB);
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(sreq, req->lreq).datatype);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_SEND_LONG_LMT_ORIGIN_CB);
    return mpi_errno;
}

int MPIDIG_ssend_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_SSEND_ACK_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_SSEND_ACK_ORIGIN_CB);
    MPID_Request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_SSEND_ACK_ORIGIN_CB);
    return mpi_errno;
}


int MPIDIG_send_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                              int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    MPIR_Comm *root_comm;
    MPIDIG_hdr_t *hdr = (MPIDIG_hdr_t *) am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_SEND_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_SEND_TARGET_MSG_CB);
    root_comm = MPIDIG_context_id_to_comm(hdr->context_id);
    if (root_comm) {
      root_comm_retry:
        /* MPIDI_CS_ENTER(); */
        while (TRUE) {
            rreq = MPIDIG_dequeue_posted(hdr->src_rank, hdr->tag, hdr->context_id,
                                         is_local, &MPIDIG_COMM(root_comm, posted_list));
#ifndef MPIDI_CH4_DIRECT_NETMOD
            if (rreq) {
                int is_cancelled;
                mpi_errno = MPIDI_anysrc_try_cancel_partner(rreq, &is_cancelled);
                MPIR_ERR_CHECK(mpi_errno);
                if (!is_cancelled) {
                    MPIR_Comm_release(root_comm);       /* -1 for posted_list */
                    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
                    continue;
                }
            }
#endif /* MPIDI_CH4_DIRECT_NETMOD */
            break;
        }
        /* MPIDI_CS_EXIT(); */
    }

    if (rreq == NULL) {
        rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RECV, 2);
        MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIDIG_REQUEST(rreq, datatype) = MPI_BYTE;
        if (in_data_sz) {
            MPIDIG_REQUEST(rreq, buffer) = (char *) MPL_malloc(in_data_sz, MPL_MEM_BUFFER);
            MPIDIG_REQUEST(rreq, count) = in_data_sz;
        } else {
            MPIDIG_REQUEST(rreq, buffer) = NULL;
            MPIDIG_REQUEST(rreq, count) = 0;
        }
        MPIDIG_REQUEST(rreq, rank) = hdr->src_rank;
        MPIDIG_REQUEST(rreq, tag) = hdr->tag;
        MPIDIG_REQUEST(rreq, context_id) = hdr->context_id;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_BUSY;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_UNEXPECTED;
#ifndef MPIDI_CH4_DIRECT_NETMOD
        MPIDI_REQUEST(rreq, is_local) = is_local;
#endif
        MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);
        if (root_comm) {
            MPIR_Comm_add_ref(root_comm);
            MPIDIG_enqueue_unexp(rreq, &MPIDIG_COMM(root_comm, unexp_list));
        } else {
            MPIR_Comm *root_comm_again;
            /* This branch means that last time we checked, there was no communicator
             * associated with the arriving message.
             * In a multi-threaded environment, it is possible that the communicator
             * has been created since we checked root_comm last time.
             * If that is the case, the new message must be put into a queue in
             * the new communicator. Otherwise that message will be lost forever.
             * Here that strategy is to query root_comm again, and if found,
             * simply re-execute the per-communicator enqueue logic above. */
            root_comm_again = MPIDIG_context_id_to_comm(hdr->context_id);
            if (unlikely(root_comm_again != NULL)) {
                MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);
                MPL_free(MPIDIG_REQUEST(rreq, buffer));
                MPIR_Request_free(rreq);
                MPID_Request_complete(rreq);
                rreq = NULL;
                root_comm = root_comm_again;
                goto root_comm_retry;
            }
            MPIDIG_enqueue_unexp(rreq, MPIDIG_context_id_to_uelist(hdr->context_id));
        }
        MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);
    } else {
        /* rreq != NULL <=> root_comm != NULL */
        MPIR_Assert(root_comm);
        /* Decrement the refcnt when popping a request out from posted_list */
        MPIR_Comm_release(root_comm);
        MPIDIG_REQUEST(rreq, rank) = hdr->src_rank;
        MPIDIG_REQUEST(rreq, tag) = hdr->tag;
        MPIDIG_REQUEST(rreq, context_id) = hdr->context_id;
    }

    rreq->status.MPI_ERROR = hdr->error_bits;
    MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_IN_PROGRESS;

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = recv_target_cmpl_cb;
    MPIDIG_REQUEST(rreq, req->seq_no) = MPL_atomic_fetch_add_uint64(&MPIDI_global.nxt_seq_no, 1);

    MPIDIG_recv_type_init(in_data_sz, rreq);

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_SEND_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_send_long_req_target_msg_cb(int handler_id, void *am_hdr, void *data,
                                       MPI_Aint in_data_sz, int is_local, int is_async,
                                       MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    MPIR_Comm *root_comm;
    MPIDIG_hdr_t *hdr = (MPIDIG_hdr_t *) am_hdr;
    MPIDIG_send_long_req_msg_t *lreq_hdr = (MPIDIG_send_long_req_msg_t *) am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_SEND_LONG_REQ_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_SEND_LONG_REQ_TARGET_MSG_CB);

    root_comm = MPIDIG_context_id_to_comm(hdr->context_id);
  root_comm_retry:
    if (root_comm) {
        /* MPIDI_CS_ENTER(); */
        while (TRUE) {
            rreq = MPIDIG_dequeue_posted(hdr->src_rank, hdr->tag, hdr->context_id,
                                         is_local, &MPIDIG_COMM(root_comm, posted_list));
#ifndef MPIDI_CH4_DIRECT_NETMOD
            if (rreq) {
                int is_cancelled;
                mpi_errno = MPIDI_anysrc_try_cancel_partner(rreq, &is_cancelled);
                MPIR_ERR_CHECK(mpi_errno);
                if (!is_cancelled) {
                    MPIR_Comm_release(root_comm);       /* -1 for posted_list */
                    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
                    continue;
                }
            }
#endif /* MPIDI_CH4_DIRECT_NETMOD */
            break;
        }
        /* MPIDI_CS_EXIT(); */
    }

    if (rreq == NULL) {
        rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RECV, 2);
        MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

        MPIDIG_REQUEST(rreq, buffer) = NULL;
        MPIDIG_REQUEST(rreq, datatype) = MPI_BYTE;
        MPIDIG_REQUEST(rreq, count) = lreq_hdr->data_sz;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_LONG_RTS;
        MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr) = lreq_hdr->sreq_ptr;
        MPIDIG_REQUEST(rreq, rank) = hdr->src_rank;
        MPIDIG_REQUEST(rreq, tag) = hdr->tag;
        MPIDIG_REQUEST(rreq, context_id) = hdr->context_id;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_IN_PROGRESS;

#ifndef MPIDI_CH4_DIRECT_NETMOD
        MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

        MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);
        if (root_comm) {
            MPIR_Comm_add_ref(root_comm);
            MPIDIG_enqueue_unexp(rreq, &MPIDIG_COMM(root_comm, unexp_list));
        } else {
            MPIR_Comm *root_comm_again;
            /* This branch means that last time we checked, there was no communicator
             * associated with the arriving message.
             * In a multi-threaded environment, it is possible that the communicator
             * has been created since we checked root_comm last time.
             * If that is the case, the new message must be put into a queue in
             * the new communicator. Otherwise that message will be lost forever.
             * Here that strategy is to query root_comm again, and if found,
             * simply re-execute the per-communicator enqueue logic above. */
            root_comm_again = MPIDIG_context_id_to_comm(hdr->context_id);
            if (unlikely(root_comm_again != NULL)) {
                MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);
                MPL_free(MPIDIG_REQUEST(rreq, buffer));
                MPIR_Request_free(rreq);
                MPID_Request_complete(rreq);
                rreq = NULL;
                root_comm = root_comm_again;
                goto root_comm_retry;
            }
            MPIDIG_enqueue_unexp(rreq,
                                 MPIDIG_context_id_to_uelist(MPIDIG_REQUEST(rreq, context_id)));
        }
        MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);
    } else {
        /* Matching receive was posted */
        rreq->comm = root_comm;
        /* NOTE: we are skipping MPIR_Comm_release for taking off posted_list since we are holding
         * the reference to root_comm in the rreq. We need to hold on to this reference so the comm
         * may remain valid by the time we send ack (using the comm).
         */
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_LONG_RTS;
        MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr) = lreq_hdr->sreq_ptr;
        MPIDIG_REQUEST(rreq, rank) = hdr->src_rank;
        MPIDIG_REQUEST(rreq, tag) = hdr->tag;
        MPIDIG_REQUEST(rreq, context_id) = hdr->context_id;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_IN_PROGRESS;
        /* Mark `match_req` as NULL so that we know nothing else to complete when
         * `unexp_req` finally completes. (See MPIDI_recv_target_cmpl_cb) */
        MPIDIG_REQUEST(rreq, req->rreq.match_req) = NULL;

        mpi_errno = MPIDIG_do_long_ack(rreq);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (is_async)
        *req = NULL;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_SEND_LONG_REQ_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_send_long_lmt_target_msg_cb(int handler_id, void *am_hdr, void *data,
                                       MPI_Aint in_data_sz, int is_local, int is_async,
                                       MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDIG_send_long_lmt_msg_t *lmt_hdr = (MPIDIG_send_long_lmt_msg_t *) am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_SEND_LONG_LMT_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_SEND_LONG_LMT_TARGET_MSG_CB);

    rreq = (MPIR_Request *) lmt_hdr->rreq_ptr;
    MPIR_Assert(rreq);

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = recv_target_cmpl_cb;
    MPIDIG_REQUEST(rreq, req->seq_no) = MPL_atomic_fetch_add_uint64(&MPIDI_global.nxt_seq_no, 1);

    MPIDIG_recv_type_init(in_data_sz, rreq);

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_SEND_LONG_LMT_TARGET_MSG_CB);

    return mpi_errno;
}

int MPIDIG_ssend_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                               int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDIG_ssend_req_msg_t *msg_hdr = (MPIDIG_ssend_req_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_SSEND_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_SSEND_TARGET_MSG_CB);

    /* receive message as normal send. Set is_async to 1 so we can update status */
    MPIR_Request *rreq;
    mpi_errno = MPIDIG_send_target_msg_cb(handler_id, am_hdr, data, in_data_sz, is_local, 1, &rreq);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Assert(rreq);
    MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr) = msg_hdr->sreq_ptr;
    MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_PEER_SSEND;

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_SSEND_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_ssend_ack_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                   int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    MPIDIG_ssend_ack_msg_t *msg_hdr = (MPIDIG_ssend_ack_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_SSEND_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_SSEND_ACK_TARGET_MSG_CB);

    sreq = (MPIR_Request *) msg_hdr->sreq_ptr;
    MPID_Request_complete(sreq);

    if (is_async)
        *req = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_SSEND_ACK_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_send_long_ack_target_msg_cb(int handler_id, void *am_hdr, void *data,
                                       MPI_Aint in_data_sz, int is_local, int is_async,
                                       MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    MPIDIG_send_long_ack_msg_t *msg_hdr = (MPIDIG_send_long_ack_msg_t *) am_hdr;
    MPIDIG_send_long_lmt_msg_t send_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_SEND_LONG_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_SEND_LONG_ACK_TARGET_MSG_CB);

    sreq = (MPIR_Request *) msg_hdr->sreq_ptr;
    MPIR_Assert(sreq != NULL);

    /* Start the main data transfer */
    send_hdr.rreq_ptr = msg_hdr->rreq_ptr;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(sreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_isend_reply(MPIDIG_REQUEST(sreq, req->lreq).context_id,
                                     MPIDIG_REQUEST(sreq, rank), MPIDIG_SEND_LONG_LMT,
                                     &send_hdr, sizeof(send_hdr),
                                     MPIDIG_REQUEST(sreq, req->lreq).src_buf,
                                     MPIDIG_REQUEST(sreq, req->lreq).count,
                                     MPIDIG_REQUEST(sreq, req->lreq).datatype, sreq);
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_isend_reply(MPIDIG_REQUEST(sreq, req->lreq).context_id,
                                    MPIDIG_REQUEST(sreq, rank), MPIDIG_SEND_LONG_LMT,
                                    &send_hdr, sizeof(send_hdr),
                                    MPIDIG_REQUEST(sreq, req->lreq).src_buf,
                                    MPIDIG_REQUEST(sreq, req->lreq).count,
                                    MPIDIG_REQUEST(sreq, req->lreq).datatype, sreq);
    }

    MPIR_ERR_CHECK(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_SEND_LONG_ACK_TARGET_MSG_CB);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
