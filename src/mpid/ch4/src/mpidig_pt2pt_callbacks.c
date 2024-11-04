/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "mpidig_pt2pt_callbacks.h"

static int handle_unexp_cmpl(MPIR_Request * rreq);
static int recv_target_cmpl_cb(MPIR_Request * rreq);

static int can_do_tag(MPIR_Request * rreq)
{
#ifdef MPIDI_CH4_DIRECT_NETMOD
    return MPIDI_NM_am_can_do_tag();
#else
    return MPIDI_REQUEST(rreq, is_local) ? MPIDI_SHM_am_can_do_tag() : MPIDI_NM_am_can_do_tag();
#endif
}

int MPIDIG_do_cts(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    int local_vci = MPIDIG_REQUEST(rreq, req->local_vci);
    int remote_vci = MPIDIG_REQUEST(rreq, req->remote_vci);
    int source_rank = rreq->status.MPI_SOURCE;

    MPIDIG_send_cts_msg_t am_hdr;
    am_hdr.sreq_ptr = (MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr));
    am_hdr.rreq_ptr = rreq;
    if (can_do_tag(rreq)) {
        am_hdr.tag = MPIDIG_get_next_am_tag(rreq->comm);
        CH4_CALL(am_tag_recv(source_rank, rreq->comm,
                             MPIDIG_TAG_RECV_COMPLETE, am_hdr.tag,
                             MPIDIG_REQUEST(rreq, buffer), MPIDIG_REQUEST(rreq, count),
                             MPIDIG_REQUEST(rreq, datatype), remote_vci, local_vci, rreq),
                 MPIDI_REQUEST(rreq, is_local), mpi_errno);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        am_hdr.tag = -1;
    }
    MPIR_Assert((void *) am_hdr.sreq_ptr != NULL);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "do cts req %p handle=0x%x", rreq, rreq->handle));

    CH4_CALL(am_send_hdr_reply(rreq->comm, source_rank, MPIDIG_SEND_CTS,
                               &am_hdr, sizeof(am_hdr), local_vci, remote_vci),
             MPIDI_REQUEST(rreq, is_local), mpi_errno);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_tag_recv_complete(MPIR_Request * rreq, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* MPI_TAG and MPI_SOURCE are set at the time of matching,
     * we only update the count and MPI_ERROR here */
    rreq->status.MPI_ERROR = status->MPI_ERROR;
    MPIR_STATUS_COPY_COUNT(rreq->status, *status);

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_anysrc_free_partner(rreq);
#endif
    MPID_Request_complete(rreq);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

static int handle_unexp_cmpl(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, in_use;
    MPIR_Request *match_req = NULL;

    MPIR_FUNC_ENTER;

    /* Check if this message has already been claimed by mprobe. */
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
        goto fn_exit;
    }

    /* If this request was previously matched, but not handled */
    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_MATCHED) {
        match_req = (MPIR_Request *) MPIDIG_REQUEST(rreq, req->rreq.match_req);

        /* we have a match_req when ch4 pre-allocates an rreq before MPIDIG_do_irecv.
         * Potentially the pre-allocates request is an any-source receive.
         */
        if (match_req) {
            /* transfer rreq to match_req, release rreq, complete match_req */
            mpi_errno = MPIDIG_handle_unexpected(MPIDIG_REQUEST(match_req, buffer),
                                                 MPIDIG_REQUEST(match_req, count),
                                                 MPIDIG_REQUEST(match_req, datatype), rreq);
            MPIR_ERR_CHECK(mpi_errno);

            MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(match_req, datatype));
            MPIR_Object_release_ref(rreq, &in_use);
            /* MPID_Request_complete(rreq); */
            MPID_Request_complete(match_req);
        } else {
            /* no match_req, just complete rreq */
            MPID_Request_complete(rreq);
        }
    } else {
        /* If we didn't match the request, simply unmark the busy bit as data is ready. */
        MPIDIG_REQUEST(rreq, req->status) &= ~MPIDIG_REQ_BUSY;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This function is called when a receive has completed on the receiver side. The input is the
 * request that has been completed. */
static int recv_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "req %p handle=0x%x", rreq, rreq->handle));

    MPIDIG_recv_finish(rreq);

    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_UNEXPECTED) {
        mpi_errno = handle_unexp_cmpl(rreq);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
    if ((MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_RTS) &&
        MPIDIG_REQUEST(rreq, req->rreq.match_req) != NULL) {
        /* This block is executed only when the receive is enqueued (handoff) &&
         * receive was matched with an unexpected long RTS message.
         * `rreq` is the unexpected message received and `match_req` is the message
         * that came from CH4 (e.g. MPIDI_recv_safe) */
        MPIR_Request *match_req = MPIDIG_REQUEST(rreq, req->rreq.match_req);
        match_req->status = rreq->status;
        MPID_Request_complete(match_req);
        /* Free the unexpected request on behalf of the user */
        MPIDI_CH4_REQUEST_FREE(rreq);
    }
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_anysrc_free_partner(rreq);
#endif
    MPID_Request_complete(rreq);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_send_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPID_Request_complete(sreq);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_send_data_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(sreq, datatype));
    MPID_Request_complete(sreq);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

/* Handling send_target_msg_cb. There 2 paths, 3 types, totalling 6 outcomes
 *     2 paths: unexpected or matched
 *     3 types: eager, rndv, transport rndv
 *
 * With unexpected path, we create unexp_req and enqueue to unexpected list. If it is
 * eager message, we mark BUSY start data copy. Otherwise, setup callback.
 *
 * With matched path, if it is eager message, start data copy. Otherwise, call cts.
 *
 */

static int match_posted_rreq(int rank, int tag, MPIR_Context_id_t context_id, int vci,
                             bool is_local, MPIR_Request ** req)
{
#ifdef MPIDI_CH4_DIRECT_NETMOD
    *req = MPIDIG_rreq_dequeue(rank, tag, context_id, &MPIDI_global.per_vci[vci].posted_list,
                               is_local, MPIDIG_PT2PT_POSTED);
    return MPI_SUCCESS;
#else
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    *req = NULL;
    while (TRUE) {
        *req = MPIDIG_rreq_dequeue(rank, tag, context_id, &MPIDI_global.per_vci[vci].posted_list,
                                   is_local, MPIDIG_PT2PT_POSTED);
        if (*req) {
            int is_cancelled;
            mpi_errno = MPIDI_anysrc_try_cancel_partner(*req, &is_cancelled);
            MPIR_ERR_CHECK(mpi_errno);
            if (!is_cancelled) {
                MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(*req, datatype));
                continue;
            }
        }
        break;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#endif /* MPIDI_CH4_DIRECT_NETMOD */
}

static int create_unexp_rreq(int rank, int tag, MPIR_Context_id_t context_id,
                             MPI_Aint data_sz, int error_bits, int is_local,
                             int local_vci, int remote_vci, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_Request *rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RECV, 2, local_vci, remote_vci);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    MPIR_REQUEST_SET_INFO(rreq, "create_unexp_rreq: source=%d, tag=%d, data_sz=%ld", rank, tag,
                          data_sz);

    *req = rreq;

    /* for unexpected message, always recv as MPI_BYTE into unexpected buffer. They will be
     * set to the recv side datatype and count when it is matched */
    MPIDIG_REQUEST(rreq, datatype) = MPI_BYTE;
    MPIDIG_REQUEST(rreq, count) = data_sz;
    MPIDIG_REQUEST(rreq, buffer) = NULL;        /* default */
    MPIDIG_REQUEST(rreq, u.recv.context_id) = context_id;

    MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_UNEXPECTED;
    MPIDIG_REQUEST(rreq, req->rreq.match_req) = NULL;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    rreq->status.MPI_SOURCE = rank;
    rreq->status.MPI_TAG = tag;
    rreq->status.MPI_ERROR = error_bits;
    MPIR_STATUS_SET_COUNT(rreq->status, data_sz);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int allocate_unexp_req_pack_buf(MPIR_Request * rreq, MPI_Aint data_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    int vci = MPIDI_Request_get_vci(rreq);
    if (data_sz > 0) {
        void *pack_buf;
        MPIR_Assert(data_sz <= MPIR_CVAR_CH4_PACK_BUFFER_SIZE);
        mpi_errno = MPIDU_genq_private_pool_alloc_cell(MPIDI_global.per_vci[vci].pack_buf_pool,
                                                       &pack_buf);
        MPIR_Assert(pack_buf);
        MPIDIG_REQUEST(rreq, buffer) = pack_buf;
    }
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

static void set_rreq_data_copy_cb(MPIR_Request * rreq, int attr)
{
    MPIR_FUNC_ENTER;
    MPIDIG_recv_data_copy_cb data_copy_cb = NULL;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (attr & MPIDIG_AM_ATTR__IS_LOCAL) {
        data_copy_cb = MPIDI_SHM_am_get_data_copy_cb(attr);
    } else {
        data_copy_cb = MPIDI_NM_am_get_data_copy_cb(attr);
    }
#else
    data_copy_cb = MPIDI_NM_am_get_data_copy_cb(attr);
#endif
    MPIDIG_recv_set_data_copy_cb(rreq, data_copy_cb);
    MPIR_FUNC_EXIT;
}

static void set_rndv_cb(MPIR_Request * rreq, int flags)
{
    int rndv_id = MPIDIG_AM_SEND_GET_RNDV_ID(flags);
    MPIDIG_recv_set_data_copy_cb(rreq, MPIDIG_global.rndv_cbs[rndv_id]);
}

static void call_rndv_cb(MPIR_Request * rreq, int flags)
{
    int rndv_id = MPIDIG_AM_SEND_GET_RNDV_ID(flags);
    MPIDIG_global.rndv_cbs[rndv_id] (rreq);
}

static void set_matched_rreq_fields(MPIR_Request * rreq, int rank, int tag,
                                    MPIR_Context_id_t context_id,
                                    MPI_Aint data_sz, int error_bits, int is_local)
{
    MPIR_FUNC_ENTER;
    MPIDIG_REQUEST(rreq, u.recv.context_id) = context_id;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    rreq->status.MPI_SOURCE = rank;
    rreq->status.MPI_TAG = tag;
    rreq->status.MPI_ERROR = error_bits;
    MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
    MPIR_FUNC_EXIT;
}

static void set_rreq_common(MPIR_Request * rreq, MPIDIG_hdr_t * hdr)
{
    MPIR_FUNC_ENTER;
    if (hdr->flags & MPIDIG_AM_SEND_FLAGS_SYNC) {
        MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr) = hdr->sreq_ptr;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_PEER_SSEND;
    }

    MPIDIG_REQUEST(rreq, rndv_hdr) = NULL;
    MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_IN_PROGRESS;
    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = recv_target_cmpl_cb;
    MPIR_FUNC_EXIT;
}

#define MSG_MODE_EAGER 1
#define MSG_MODE_RNDV_RTS 2
#define MSG_MODE_TRANSPORT_RNDV 3

int MPIDIG_send_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                              uint32_t attr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_Request *rreq = NULL;
    MPIDIG_hdr_t *hdr = (MPIDIG_hdr_t *) am_hdr;
    int is_local = (attr & MPIDIG_AM_ATTR__IS_LOCAL) ? 1 : 0;
    int remote_vci = MPIDIG_AM_ATTR_SRC_VCI(attr);
    int local_vci = MPIDIG_AM_ATTR_DST_VCI(attr);

    int msg_mode;
    if (hdr->flags & MPIDIG_AM_SEND_FLAGS_RTS) {
        msg_mode = MSG_MODE_RNDV_RTS;
    } else if (attr & MPIDIG_AM_ATTR__IS_RNDV) {
        msg_mode = MSG_MODE_TRANSPORT_RNDV;
    } else {
        msg_mode = MSG_MODE_EAGER;
    }

    mpi_errno = match_posted_rreq(hdr->src_rank, hdr->tag, hdr->context_id,
                                  local_vci, is_local, &rreq);
    MPIR_ERR_CHECK(mpi_errno);

    if (rreq == NULL) {
        /* unexpected path */
        mpi_errno = create_unexp_rreq(hdr->src_rank, hdr->tag, hdr->context_id,
                                      hdr->data_sz, hdr->error_bits, is_local,
                                      local_vci, remote_vci, &rreq);
        MPIR_ERR_CHECK(mpi_errno);
        set_rreq_common(rreq, hdr);

        if (msg_mode == MSG_MODE_EAGER) {
            allocate_unexp_req_pack_buf(rreq, hdr->data_sz);
            /* marking the request busy to prevent recv swapout the request
             * before transport finishes. */
            MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_BUSY;
            MPIDIG_recv_type_init(hdr->data_sz, rreq);
        } else if (msg_mode == MSG_MODE_RNDV_RTS) {
            MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr) = hdr->sreq_ptr;
            MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_RTS;
            if (hdr->rndv_hdr_sz > 0) {
                /* free after we call rndv_cb  */
                void *rndv_hdr = MPL_malloc(hdr->rndv_hdr_sz, MPL_MEM_OTHER);
                MPIR_Assert(rndv_hdr);
                memcpy(rndv_hdr, hdr + 1, hdr->rndv_hdr_sz);
                MPIDIG_REQUEST(rreq, rndv_hdr) = rndv_hdr;
            }
            set_rndv_cb(rreq, hdr->flags);
        } else {        /* MSG_MODE_TRANSPORT_RNDV */
            MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_RTS;
            set_rreq_data_copy_cb(rreq, attr);
        }

        MPIDIG_enqueue_request(rreq, &MPIDI_global.per_vci[local_vci].unexp_list,
                               MPIDIG_PT2PT_UNEXP);
        MPII_UNEXPQ_REMEMBER(rreq, hdr->src_rank, hdr->tag, hdr->context_id);
    } else {
        /* matched path */
        MPIDIG_REQUEST(rreq, req->remote_vci) = remote_vci;
        set_matched_rreq_fields(rreq, hdr->src_rank, hdr->tag, hdr->context_id,
                                hdr->data_sz, hdr->error_bits, is_local);
        set_rreq_common(rreq, hdr);
        if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_PEER_SSEND) {
            mpi_errno = MPIDIG_reply_ssend(rreq);
            MPIR_ERR_CHECK(mpi_errno);
        }

        MPIDIG_recv_type_init(hdr->data_sz, rreq);

        MPIR_REQUEST_SET_INFO(rreq, "matched rreq: source=%d, tag=%d, data_sz=%ld, msg_mode=%d",
                              rreq->status.MPI_SOURCE, rreq->status.MPI_TAG,
                              hdr->data_sz, msg_mode);
        if (msg_mode == MSG_MODE_EAGER) {
            /* either we or transport will finish recv_copy depending on ASYNC attr */
        } else if (msg_mode == MSG_MODE_RNDV_RTS) {
            MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr) = hdr->sreq_ptr;
            MPIDIG_REQUEST(rreq, rndv_hdr) = hdr + 1;
            call_rndv_cb(rreq, hdr->flags);
            MPIDIG_REQUEST(rreq, rndv_hdr) = NULL;
        } else {        /* MSG_MODE_TRANSPORT_RNDV */
            /* transport will finish the rndv */
        }
    }

    if (attr & MPIDIG_AM_ATTR__IS_ASYNC) {
        *req = rreq;
    } else if (msg_mode == MSG_MODE_EAGER) {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_send_data_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                   uint32_t attr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDIG_send_data_msg_t *seg_hdr = (MPIDIG_send_data_msg_t *) am_hdr;

    MPIR_FUNC_ENTER;

    rreq = (MPIR_Request *) seg_hdr->rreq_ptr;
    MPIR_Assert(rreq);

    if (attr & MPIDIG_AM_ATTR__IS_ASYNC) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_ssend_ack_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                   uint32_t attr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    MPIDIG_ssend_ack_msg_t *msg_hdr = (MPIDIG_ssend_ack_msg_t *) am_hdr;
    MPIR_FUNC_ENTER;

    sreq = (MPIR_Request *) msg_hdr->sreq_ptr;
    MPID_Request_complete(sreq);

    if (attr & MPIDIG_AM_ATTR__IS_ASYNC) {
        *req = NULL;
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_send_cts_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                  uint32_t attr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    MPIDIG_send_cts_msg_t *msg_hdr = (MPIDIG_send_cts_msg_t *) am_hdr;
    MPIDIG_send_data_msg_t send_hdr;
    MPIR_FUNC_ENTER;

    sreq = (MPIR_Request *) msg_hdr->sreq_ptr;
    MPIR_Assert(sreq != NULL);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "got cts req handle=0x%x", sreq->handle));

    int local_vci = MPIDIG_REQUEST(sreq, req->local_vci);
    int remote_vci = MPIDIG_REQUEST(sreq, req->remote_vci);
    /* Start the main data transfer */

    if (msg_hdr->tag >= 0) {
        CH4_CALL(am_tag_send(MPIDIG_REQUEST(sreq, u.send.dest), sreq->comm,
                             MPIDIG_SEND_DATA, msg_hdr->tag,
                             MPIDIG_REQUEST(sreq, buffer),
                             MPIDIG_REQUEST(sreq, count),
                             MPIDIG_REQUEST(sreq, datatype), local_vci, remote_vci, sreq),
                 MPIDI_REQUEST(sreq, is_local), mpi_errno);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        send_hdr.rreq_ptr = msg_hdr->rreq_ptr;
        CH4_CALL(am_isend_reply(sreq->comm,
                                MPIDIG_REQUEST(sreq, u.send.dest), MPIDIG_SEND_DATA,
                                &send_hdr, sizeof(send_hdr),
                                MPIDIG_REQUEST(sreq, buffer),
                                MPIDIG_REQUEST(sreq, count),
                                MPIDIG_REQUEST(sreq, datatype), local_vci, remote_vci, sreq),
                 MPIDI_REQUEST(sreq, is_local), mpi_errno);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIR_FUNC_EXIT;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
