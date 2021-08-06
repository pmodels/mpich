/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "ch4r_callbacks.h"

static int handle_unexp_cmpl(MPIR_Request * rreq);
static int recv_target_cmpl_cb(MPIR_Request * rreq);

int MPIDIG_do_cts(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDIG_send_cts_msg_t am_hdr;
    am_hdr.sreq_ptr = (MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr));
    am_hdr.rreq_ptr = rreq;
    MPIR_Assert((void *) am_hdr.sreq_ptr != NULL);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "do cts req %p handle=0x%x", rreq, rreq->handle));

    CH4_CALL(am_send_hdr_reply(rreq->comm, MPIDIG_REQUEST(rreq, rank), MPIDIG_SEND_CTS,
                               &am_hdr, sizeof(am_hdr)), MPIDI_REQUEST(rreq, is_local), mpi_errno);
    MPIR_ERR_CHECK(mpi_errno);

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
            req = curr->request;
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

    mpi_errno = MPIDIG_handle_unexpected(MPIDIG_REQUEST(match_req, buffer),
                                         MPIDIG_REQUEST(match_req, count),
                                         MPIDIG_REQUEST(match_req, datatype), rreq);
    MPIR_ERR_CHECK(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_anysrc_free_partner(match_req);
#endif

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(match_req, datatype));
    MPIR_Object_release_ref(rreq, &in_use);
    /* MPID_Request_complete(rreq); */
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

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "req %p handle=0x%x", rreq, rreq->handle));

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
    if ((MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_RTS) &&
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
        MPIDI_CH4_REQUEST_FREE(rreq);
    }
    MPID_Request_complete(rreq);
  fn_exit:
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

int MPIDIG_send_data_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_SEND_DATA_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_SEND_DATA_ORIGIN_CB);
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(sreq, req->sreq).datatype);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_SEND_DATA_ORIGIN_CB);
    return mpi_errno;
}

int MPIDIG_send_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                              int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    MPIDIG_hdr_t *hdr = (MPIDIG_hdr_t *) am_hdr;
    void *pack_buf = NULL;
    bool do_cts = false;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_SEND_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_SEND_TARGET_MSG_CB);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "HDR: data_sz=%ld, flags=0x%X", hdr->data_sz, hdr->flags));
    /* MPIDI_CS_ENTER(); */
    while (TRUE) {
        rreq =
            MPIDIG_rreq_dequeue(hdr->src_rank, hdr->tag, hdr->context_id,
                                &MPIDI_global.posted_list, MPIDIG_PT2PT_POSTED);
#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (rreq) {
            int is_cancelled;
            mpi_errno = MPIDI_anysrc_try_cancel_partner(rreq, &is_cancelled);
            MPIR_ERR_CHECK(mpi_errno);
            if (!is_cancelled) {
                MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
                continue;
            }
        }
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        break;
    }
    /* MPIDI_CS_EXIT(); */

    if (rreq == NULL) {
        rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RECV, 2);
        MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        /* for unexpected message, always recv as MPI_BYTE into unexpected buffer. They will be
         * set to the recv side datatype and count when it is matched */
        MPIDIG_REQUEST(rreq, datatype) = MPI_BYTE;
        MPIDIG_REQUEST(rreq, count) = hdr->data_sz;
        /* Note the hdr->data_sz means how much data the incoming AM message has, the in_data_sz is
         * how much data there is along with the incoming header. The sender may have decided to
         * send the header and the data separately (for example in RNDV protocol) which would result
         * in in_data_sz == 0 and hdr->data_sz != 0. */
        if (in_data_sz) {
            /* If there is inline data, we allocate unexpected buffer */
            MPIR_Assert(in_data_sz <= MPIR_CVAR_CH4_AM_PACK_BUFFER_SIZE);
            mpi_errno =
                MPIDU_genq_private_pool_alloc_cell(MPIDI_global.unexp_pack_buf_pool, &pack_buf);
            MPIR_Assert(pack_buf);
            MPIDIG_REQUEST(rreq, buffer) = pack_buf;
        } else {
            /* The SEND is a zero byte messeage */
            MPIDIG_REQUEST(rreq, buffer) = NULL;
        }
        MPIDIG_REQUEST(rreq, rank) = hdr->src_rank;
        MPIDIG_REQUEST(rreq, tag) = hdr->tag;
        MPIDIG_REQUEST(rreq, context_id) = hdr->context_id;

        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_UNEXPECTED;
        rreq->status.MPI_ERROR = hdr->error_bits;
        if (hdr->flags & MPIDIG_AM_SEND_FLAGS_RTS) {
            /* this is unexpected RNDV */
            MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr) = hdr->sreq_ptr;
            MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_RTS;
            MPIDIG_REQUEST(rreq, req->rreq.match_req) = NULL;
        } else {
            /* this is unexpected EAGER */
            if (MPIDIG_REQUEST(rreq, buffer) || hdr->data_sz == 0) {
                /* if we allocated unexp buffer (which mean we have inline data), or if the message
                 * is a 0 byte message. Go ahead init recv */

                /* marking the request busy to prevent another thread touching it before transport
                 * finishes and calls the recv_target_cmpl_cb, or transport setup the data_copy_cb.
                 * */
                MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_BUSY;
                MPIDIG_recv_type_init(hdr->data_sz, rreq);
            } else {
                /* We did not allocate unexp buffer because there is no inline data for a non-zero
                 * SEND message. The data copy will be triggered when matching recv is posted, we
                 * only need to set the data_copy_cb provided by the netmod. */
                MPIDIG_recv_data_copy_cb data_copy_cb = NULL;
#ifndef MPIDI_CH4_DIRECT_NETMOD
                if (is_local) {
                    data_copy_cb = MPIDI_SHM_am_get_data_copy_cb();
                } else {
#endif
                    data_copy_cb = MPIDI_NM_am_get_data_copy_cb();
#ifndef MPIDI_CH4_DIRECT_NETMOD
                }
#endif
                MPIDIG_recv_set_data_copy_cb(rreq, data_copy_cb);

            }
        }
#ifndef MPIDI_CH4_DIRECT_NETMOD
        MPIDI_REQUEST(rreq, is_local) = is_local;
#endif
        MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);
        MPIDIG_enqueue_request(rreq, &MPIDI_global.unexp_list, MPIDIG_PT2PT_UNEXP);
        MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);
    } else {
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, "posted req %p handle=0x%x", rreq, rreq->handle));

        MPIDIG_REQUEST(rreq, rank) = hdr->src_rank;
        MPIDIG_REQUEST(rreq, tag) = hdr->tag;
        MPIDIG_REQUEST(rreq, context_id) = hdr->context_id;
#ifndef MPIDI_CH4_DIRECT_NETMOD
        MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

        rreq->status.MPI_ERROR = hdr->error_bits;
        if (hdr->flags & MPIDIG_AM_SEND_FLAGS_RTS) {
            /* this is expected RNDV, init a special recv into unexp buffer */
            MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr) = hdr->sreq_ptr;
            MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_RTS;
            MPIDIG_REQUEST(rreq, req->rreq.match_req) = NULL;
            do_cts = true;
        }
        MPIDIG_recv_type_init(hdr->data_sz, rreq);
    }

    if (hdr->flags & MPIDIG_AM_SEND_FLAGS_SYNC) {
        MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr) = hdr->sreq_ptr;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_PEER_SSEND;
    }

    MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_IN_PROGRESS;

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = recv_target_cmpl_cb;

    if (is_async) {
        if (hdr->flags & MPIDIG_AM_SEND_FLAGS_RTS) {
            *req = NULL;
        } else {
            *req = rreq;
        }
    } else {
        if (!(hdr->flags & MPIDIG_AM_SEND_FLAGS_RTS)) {
            MPIDIG_recv_copy(data, rreq);
            MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
        }
    }

    if (do_cts) {
        MPIDIG_do_cts(rreq);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_SEND_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_send_data_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                   int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDIG_send_data_msg_t *seg_hdr = (MPIDIG_send_data_msg_t *) am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_SEND_DATA_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_SEND_DATA_TARGET_MSG_CB);

    rreq = (MPIR_Request *) seg_hdr->rreq_ptr;
    MPIR_Assert(rreq);

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_SEND_DATA_TARGET_MSG_CB);
    return mpi_errno;
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

int MPIDIG_send_cts_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                  int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    MPIDIG_send_cts_msg_t *msg_hdr = (MPIDIG_send_cts_msg_t *) am_hdr;
    MPIDIG_send_data_msg_t send_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_SEND_CTS_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_SEND_CTS_TARGET_MSG_CB);

    sreq = (MPIR_Request *) msg_hdr->sreq_ptr;
    MPIR_Assert(sreq != NULL);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "got cts req handle=0x%x", sreq->handle));

    /* Start the main data transfer */
    send_hdr.rreq_ptr = msg_hdr->rreq_ptr;
    CH4_CALL(am_isend_reply(sreq->comm,
                            MPIDIG_REQUEST(sreq, rank), MPIDIG_SEND_DATA,
                            &send_hdr, sizeof(send_hdr),
                            MPIDIG_REQUEST(sreq, req->sreq).src_buf,
                            MPIDIG_REQUEST(sreq, req->sreq).count,
                            MPIDIG_REQUEST(sreq, req->sreq).datatype, sreq),
             MPIDI_REQUEST(sreq, is_local), mpi_errno);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_SEND_CTS_TARGET_MSG_CB);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
