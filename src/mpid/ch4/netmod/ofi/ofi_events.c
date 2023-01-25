/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_am_events.h"
#include "ofi_events.h"

/* We can use a generic length fi_info.max_err_data returned by fi_getinfo()
 * However, currently we do not use the error data, we set the length to a
 * value that is generally common across the different providers. */
#define MPIDI_OFI_MAX_ERR_DATA_SIZE 64

static int peek_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int peek_empty_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int send_huge_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * sreq);
static int ssend_ack_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * sreq);
static int chunk_done_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * req);
static int inject_emu_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * req);
static int accept_probe_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int dynproc_done_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int am_isend_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * sreq);
static int am_isend_rdma_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * sreq);
static int am_isend_pipeline_event(int vci, struct fi_cq_tagged_entry *wc,
                                   MPIR_Request * dont_use_me);
static int am_recv_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int am_read_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * dont_use_me);

static int peek_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (MPIDI_OFI_HUGE_SEND & wc->tag) {
        mpi_errno = MPIDI_OFI_peek_huge_event(vci, wc, rreq);
        goto fn_exit;
    }

    rreq->status.MPI_SOURCE = MPIDI_OFI_cqe_get_source(wc, false);
    rreq->status.MPI_TAG = MPIDI_OFI_init_get_tag(wc->tag);
    rreq->status.MPI_ERROR = MPI_SUCCESS;
    MPIR_STATUS_SET_COUNT(rreq->status, wc->len);
    /* util_id should be the last thing to change in rreq. Reason is
     * we use util_id to indicate peek_event has completed and all the
     * relevant values have been copied to rreq. */
    MPL_atomic_release_store_int(&(MPIDI_OFI_REQUEST(rreq, util_id)), MPIDI_OFI_PEEK_FOUND);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

static int peek_empty_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    MPIR_FUNC_ENTER;
    MPIDI_OFI_dynamic_process_request_t *ctrl;

    switch (MPIDI_OFI_REQUEST(rreq, event_id)) {
        case MPIDI_OFI_EVENT_PEEK:
            rreq->status.MPI_ERROR = MPI_SUCCESS;
            /* util_id should be the last thing to change in rreq. Reason is
             * we use util_id to indicate peek_event has completed and all the
             * relevant values have been copied to rreq. */
            MPL_atomic_release_store_int(&(MPIDI_OFI_REQUEST(rreq, util_id)),
                                         MPIDI_OFI_PEEK_NOT_FOUND);
            break;

        case MPIDI_OFI_EVENT_ACCEPT_PROBE:
            ctrl = (MPIDI_OFI_dynamic_process_request_t *) rreq;
            ctrl->done = MPIDI_OFI_PEEK_NOT_FOUND;
            break;

        default:
            MPIR_Assert(0);
            break;
    }

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

static int send_huge_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    int c, num_nics;
    MPIR_FUNC_ENTER;

    MPIR_cc_decr(sreq->cc_ptr, &c);

    if (c == 0) {
        MPIR_Comm *comm;
        struct fid_mr **huge_send_mrs;

        comm = sreq->comm;
        num_nics = MPIDI_OFI_COMM(comm).enable_striping ? MPIDI_OFI_global.num_nics : 1;
        huge_send_mrs = MPIDI_OFI_REQUEST(sreq, huge.send_mrs);

        /* Clean up the memory region */
        for (int i = 0; i < num_nics; i++) {
            uint64_t key = fi_mr_key(huge_send_mrs[i]);
            MPIDI_OFI_CALL(fi_close(&huge_send_mrs[i]->fid), mr_unreg);
            if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
                MPIDI_OFI_mr_key_free(MPIDI_OFI_LOCAL_MR_KEY, key);
            }
        }
        MPL_free(huge_send_mrs);

        if (MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer)) {
            MPL_free(MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer));
        }

        MPIR_Datatype_release_if_not_builtin(MPIDI_OFI_REQUEST(sreq, datatype));
        MPIDI_CH4_REQUEST_FREE(sreq);
    }
    /* c != 0, ssend */
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ssend_ack_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * sreq)
{
    MPIDI_OFI_ssendack_request_t *req = (MPIDI_OFI_ssendack_request_t *) sreq;
    MPIR_FUNC_ENTER;
    /* no need to cleanup the packed data buffer, it's done when the main send completes */
    MPIDI_Request_complete_fast(req->signal_req);
    MPL_free(req);
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

static int chunk_done_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * req)
{
    int c;
    MPIR_FUNC_ENTER;

    MPIDI_OFI_chunk_request *creq = (MPIDI_OFI_chunk_request *) req;
    MPIR_cc_decr(creq->parent->cc_ptr, &c);

    if (c == 0)
        MPIDI_CH4_REQUEST_FREE(creq->parent);

    MPL_free(creq);
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

static int inject_emu_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * req)
{
    int incomplete;
    MPIR_FUNC_ENTER;

    MPIR_cc_decr(req->cc_ptr, &incomplete);

    if (!incomplete) {
        MPL_free(MPIDI_OFI_REQUEST(req, util.inject_buf));
        MPIDI_CH4_REQUEST_FREE(req);
        MPIDI_OFI_global.per_vci[vci].am_inflight_inject_emus -= 1;
    }

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

static int accept_probe_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    MPIR_FUNC_ENTER;
    MPIDI_OFI_dynamic_process_request_t *ctrl = (MPIDI_OFI_dynamic_process_request_t *) rreq;
    ctrl->source = MPIDI_OFI_cqe_get_source(wc, false);
    ctrl->tag = MPIDI_OFI_init_get_tag(wc->tag);
    ctrl->msglen = wc->len;
    ctrl->done = MPIDI_OFI_PEEK_FOUND;
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

static int dynproc_done_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    MPIR_FUNC_ENTER;
    MPIDI_OFI_dynamic_process_request_t *ctrl = (MPIDI_OFI_dynamic_process_request_t *) rreq;
    ctrl->done++;
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

static int am_isend_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_header_t *msg_hdr;

    MPIR_FUNC_ENTER;

    msg_hdr = &MPIDI_OFI_AM_SREQ_HDR(sreq, msg_hdr);
    MPID_Request_complete(sreq);

    MPIDU_genq_private_pool_free_cell(MPIDI_global.per_vci[vci].pack_buf_pool,
                                      MPIDI_OFI_AM_SREQ_HDR(sreq, pack_buffer));
    MPIDI_OFI_AM_SREQ_HDR(sreq, pack_buffer) = NULL;
    mpi_errno = MPIDIG_global.origin_cbs[msg_hdr->handler_id] (sreq);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int am_isend_rdma_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPID_Request_complete(sreq);

    /* RDMA_READ will perform origin side completion when ACK arrives */

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

static int am_isend_pipeline_event(int vci, struct fi_cq_tagged_entry *wc,
                                   MPIR_Request * dont_use_me)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_send_pipeline_request_t *ofi_req;
    MPIR_Request *sreq = NULL;

    MPIR_FUNC_ENTER;

    ofi_req = MPL_container_of(wc->op_context, MPIDI_OFI_am_send_pipeline_request_t, context);
    int handler_id = ((MPIDI_OFI_am_header_t *) ofi_req->msg_hdr)->handler_id;
    sreq = ofi_req->sreq;
    MPID_Request_complete(sreq);        /* FIXME: Should not call MPIDI in NM ? */

    MPIDU_genq_private_pool_free_cell(MPIDI_global.per_vci[vci].pack_buf_pool,
                                      ofi_req->pack_buffer);

    MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.per_vci[vci].am_hdr_buf_pool, ofi_req);

    int is_done = MPIDIG_am_send_async_finish_seg(sreq);

    if (is_done) {
        mpi_errno = MPIDIG_global.origin_cbs[handler_id] (sreq);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int am_recv_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_header_t *am_hdr;
    MPIDI_OFI_am_unordered_msg_t *uo_msg = NULL;
    uint16_t expected_seqno, next_seqno;
    MPIR_FUNC_ENTER;

    void *orig_buf = wc->buf;   /* needed in case we will copy the header for alignment fix */
    am_hdr = (MPIDI_OFI_am_header_t *) wc->buf;

#ifdef NEEDS_STRICT_ALIGNMENT
    /* FI_MULTI_RECV may pack the message at lesser alignment, copy the header
     * when that's the case */
#define MAX_HDR_SIZE 280        /* MPIDI_OFI_AM_MSG_HEADER_SIZE + MPIDI_OFI_MAX_AM_HDR_SIZE */
    MPL_COMPILE_TIME_ASSERT(MAX_HDR_SIZE >=
                            MPIDI_OFI_AM_MSG_HEADER_SIZE + MPIDI_OFI_MAX_AM_HDR_SIZE);
    /* if has_alignment_copy is 0 and the message contains extended header, the
     * header needs to be copied out for alignment to access */
    int has_alignment_copy = 0;
    char temp[MAX_HDR_SIZE] MPL_ATTR_ALIGNED(MAX_ALIGNMENT);
    if ((intptr_t) am_hdr & (MAX_ALIGNMENT - 1)) {
        int temp_size = MAX_HDR_SIZE;
        if (temp_size > wc->len) {
            temp_size = wc->len;
        }
        memcpy(temp, orig_buf, temp_size);
        am_hdr = (void *) temp;
        /* confirm alignment (in case MPL_ATTR_ALIGNED didn't work) */
        MPIR_Assert(((intptr_t) am_hdr & (MAX_ALIGNMENT - 1)) == 0);
    }
#endif

    expected_seqno = MPIDI_OFI_am_get_next_recv_seqno(vci, am_hdr->src_id);
    if (am_hdr->seqno != expected_seqno) {
        /* This message came earlier than the one that we were expecting.
         * Put it in the queue to process it later. */
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, TERSE,
                        (MPL_DBG_FDEST,
                         "Expected seqno=%d but got %d (am_type=%d src_rank=%d). "
                         "Enqueueing it to the queue.\n",
                         expected_seqno, am_hdr->seqno, am_hdr->am_type, am_hdr->src_rank));
        mpi_errno = MPIDI_OFI_am_enqueue_unordered_msg(vci, orig_buf);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    /* Received an expected message */
    uint64_t remote_id;
  fn_repeat:
    remote_id = am_hdr->src_id;
    next_seqno = am_hdr->seqno + 1;

    void *p_data;
    switch (am_hdr->am_type) {
        case MPIDI_AMTYPE_SHORT_HDR:
            mpi_errno = MPIDI_OFI_handle_short_am_hdr(am_hdr, am_hdr + 1 /* payload */);

            MPIR_ERR_CHECK(mpi_errno);

            break;

        case MPIDI_AMTYPE_SHORT:
            /* payload always in orig_buf */
            p_data = (char *) orig_buf + sizeof(*am_hdr) + am_hdr->am_hdr_sz;
            mpi_errno = MPIDI_OFI_handle_short_am(am_hdr, am_hdr + 1, p_data);

            MPIR_ERR_CHECK(mpi_errno);

            break;
        case MPIDI_AMTYPE_PIPELINE:
            p_data = (char *) orig_buf + sizeof(*am_hdr) + am_hdr->am_hdr_sz;
            mpi_errno = MPIDI_OFI_handle_pipeline(am_hdr, am_hdr + 1, p_data);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPIDI_AMTYPE_RDMA_READ:
            {
                /* buffer always copied together (there is no payload, just LMT header) */
#ifdef NEEDS_STRICT_ALIGNMENT
                MPIDI_OFI_lmt_msg_payload_t temp_rdma_lmt_msg;
                if (!has_alignment_copy) {
                    memcpy(&temp_rdma_lmt_msg,
                           (char *) orig_buf + sizeof(*am_hdr) + am_hdr->am_hdr_sz,
                           sizeof(MPIDI_OFI_lmt_msg_payload_t));
                    p_data = (void *) &temp_rdma_lmt_msg;
                } else
#endif
                {
                    p_data = (char *) orig_buf + sizeof(*am_hdr) + am_hdr->am_hdr_sz;
                }
                mpi_errno = MPIDI_OFI_handle_rdma_read(am_hdr, am_hdr + 1,
                                                       (MPIDI_OFI_lmt_msg_payload_t *) p_data);

                MPIR_ERR_CHECK(mpi_errno);

                break;
            }
        default:
            MPIR_Assert(0);
    }

    /* For the first iteration (=in case we can process the message just received
     * from OFI immediately), uo_msg is NULL, so freeing it is no-op.
     * Otherwise, free it here before getting another uo_msg. */
    MPL_free(uo_msg);

    /* See if we can process other messages in the queue */
    uo_msg = MPIDI_OFI_am_claim_unordered_msg(vci, remote_id, next_seqno);
    if (uo_msg != NULL) {
        am_hdr = &uo_msg->am_hdr;
        orig_buf = am_hdr;
#ifdef NEEDS_STRICT_ALIGNMENT
        /* alignment is ensured for this unordered message as it copies to a temporary buffer
         * in MPIDI_OFI_am_enqueue_unordered_msg */
        has_alignment_copy = 1;
#endif
        goto fn_repeat;
    }

    /* Record the next expected sequence number */
    MPIDI_OFI_am_set_next_recv_seqno(vci, remote_id, next_seqno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int am_read_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * dont_use_me)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDI_OFI_am_request_t *ofi_req;

    MPIR_FUNC_ENTER;

    ofi_req = MPL_container_of(wc->op_context, MPIDI_OFI_am_request_t, context);
    rreq = (MPIR_Request *) ofi_req->rreq_hdr->rreq_ptr;

    if (ofi_req->rreq_hdr->lmt_type == MPIDI_OFI_AM_LMT_IOV) {
        ofi_req->rreq_hdr->lmt_u.lmt_cntr--;
        if (ofi_req->rreq_hdr->lmt_u.lmt_cntr) {
            goto fn_exit;
        }
    } else if (ofi_req->rreq_hdr->lmt_type == MPIDI_OFI_AM_LMT_UNPACK) {
        int done = MPIDI_OFI_am_lmt_unpack_event(rreq);
        if (!done) {
            goto fn_exit;
        }
    }

    MPIR_Comm *comm;
    if (rreq->kind == MPIR_REQUEST_KIND__RMA) {
        comm = rreq->u.rma.win->comm_ptr;
    } else {
        comm = rreq->comm;
    }
    MPIDI_OFI_lmt_msg_payload_t *lmt_info = (void *) MPIDI_OFI_AM_RREQ_HDR(rreq, am_hdr_buf);
    int local_vci = MPIDIG_REQUEST(rreq, req->local_vci);
    int remote_vci = MPIDIG_REQUEST(rreq, req->remote_vci);
    mpi_errno = MPIDI_OFI_do_am_rdma_read_ack(lmt_info->src_rank, comm, lmt_info->sreq_ptr,
                                              local_vci, remote_vci);

    MPIR_ERR_CHECK(mpi_errno);

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    MPID_Request_complete(rreq);
  fn_exit:
    MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.per_vci[vci].am_hdr_buf_pool, ofi_req);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_dispatch_function(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_SEND) {
        /* Passing the event_id as a parameter; do not need to load it from the
         * request object each time the send_event handler is invoked */
        mpi_errno = MPIDI_OFI_send_event(vci, wc, req, MPIDI_OFI_EVENT_SEND);
        goto fn_exit;
    } else if (MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_RECV) {
        /* Passing the event_id as a parameter; do not need to load it from the
         * request object each time the send_event handler is invoked */
        mpi_errno = MPIDI_OFI_recv_event(vci, wc, req, MPIDI_OFI_EVENT_RECV);
        goto fn_exit;
    } else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_AM_SEND)) {
        mpi_errno = am_isend_event(vci, wc, req);
        goto fn_exit;
    } else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_AM_SEND_RDMA)) {
        mpi_errno = am_isend_rdma_event(vci, wc, req);
        goto fn_exit;
    } else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_AM_SEND_PIPELINE)) {
        mpi_errno = am_isend_pipeline_event(vci, wc, req);
        goto fn_exit;
    } else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_AM_RECV)) {
        if (wc->flags & FI_RECV)
            mpi_errno = am_recv_event(vci, wc, req);

        if (unlikely(wc->flags & FI_MULTI_RECV)) {
            MPIDI_OFI_am_repost_request_t *am = (MPIDI_OFI_am_repost_request_t *) req;
            mpi_errno = MPIDI_OFI_am_repost_buffer(vci, am->index);
        }

        goto fn_exit;
    } else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_AM_READ)) {
        mpi_errno = am_read_event(vci, wc, req);
        goto fn_exit;
    } else if (unlikely(1)) {
        switch (MPIDI_OFI_REQUEST(req, event_id)) {
            case MPIDI_OFI_EVENT_PEEK:
                mpi_errno = peek_event(vci, wc, req);
                break;

            case MPIDI_OFI_EVENT_RECV_HUGE:
                if (wc->tag & MPIDI_OFI_HUGE_SEND) {
                    mpi_errno = MPIDI_OFI_recv_huge_event(vci, wc, req);
                } else {
                    mpi_errno = MPIDI_OFI_recv_event(vci, wc, req, MPIDI_OFI_EVENT_RECV_HUGE);
                }
                break;

            case MPIDI_OFI_EVENT_RECV_PACK:
                mpi_errno = MPIDI_OFI_recv_event(vci, wc, req, MPIDI_OFI_EVENT_RECV_PACK);
                break;

            case MPIDI_OFI_EVENT_RECV_NOPACK:
                mpi_errno = MPIDI_OFI_recv_event(vci, wc, req, MPIDI_OFI_EVENT_RECV_NOPACK);
                break;

            case MPIDI_OFI_EVENT_SEND_HUGE:
                mpi_errno = send_huge_event(vci, wc, req);
                break;

            case MPIDI_OFI_EVENT_SEND_PACK:
                mpi_errno = MPIDI_OFI_send_event(vci, wc, req, MPIDI_OFI_EVENT_SEND_PACK);
                break;

            case MPIDI_OFI_EVENT_SEND_NOPACK:
                mpi_errno = MPIDI_OFI_send_event(vci, wc, req, MPIDI_OFI_EVENT_SEND_NOPACK);
                break;

            case MPIDI_OFI_EVENT_SSEND_ACK:
                mpi_errno = ssend_ack_event(vci, wc, req);
                break;

            case MPIDI_OFI_EVENT_CHUNK_DONE:
                mpi_errno = chunk_done_event(vci, wc, req);
                break;

            case MPIDI_OFI_EVENT_HUGE_CHUNK_DONE:
                mpi_errno = MPIDI_OFI_huge_chunk_done_event(vci, wc, req);
                break;

            case MPIDI_OFI_EVENT_INJECT_EMU:
                mpi_errno = inject_emu_event(vci, wc, req);
                break;

            case MPIDI_OFI_EVENT_DYNPROC_DONE:
                mpi_errno = dynproc_done_event(vci, wc, req);
                break;

            case MPIDI_OFI_EVENT_ACCEPT_PROBE:
                mpi_errno = accept_probe_event(vci, wc, req);
                break;

            case MPIDI_OFI_EVENT_ABORT:
            default:
                mpi_errno = MPI_SUCCESS;
                MPIR_Assert(0);
                break;
        }
    }

  fn_exit:
    return mpi_errno;
}

int MPIDI_OFI_handle_cq_error(int vci, int nic, ssize_t ret)
{
    int mpi_errno = MPI_SUCCESS;
    struct fi_cq_err_entry e;
    char err_data[MPIDI_OFI_MAX_ERR_DATA_SIZE];
    MPIR_Request *req;
    ssize_t ret_cqerr;
    MPIR_FUNC_ENTER;

    int ctx_idx = MPIDI_OFI_get_ctx_index(vci, nic);
    switch (ret) {
        case -FI_EAVAIL:
            /* Provide separate error buffer for each thread. This makes the
             * call to fi_cq_readerr threadsafe. If we don't provide the buffer,
             * OFI passes an internal buffer to the threads, which can lead to
             * the threads sharing the buffer. */
            e.err_data = err_data;
            e.err_data_size = sizeof(err_data);
            ret_cqerr = fi_cq_readerr(MPIDI_OFI_global.ctx[ctx_idx].cq, &e, 0);
            /* The error was already consumed, most likely by another thread,
             *  possible in case of lockless MT model */
            if (ret_cqerr == -FI_EAGAIN)
                break;

            switch (e.err) {
                case FI_ETRUNC:
                    req = MPIDI_OFI_context_to_request(e.op_context);

                    switch (req->kind) {
                        case MPIR_REQUEST_KIND__SEND:
                            mpi_errno = MPIDI_OFI_dispatch_function(vci, NULL, req);
                            break;

                        case MPIR_REQUEST_KIND__RECV:
                            req->status.MPI_ERROR = MPI_ERR_TRUNCATE;
                            mpi_errno =
                                MPIDI_OFI_dispatch_function(vci, (struct fi_cq_tagged_entry *) &e,
                                                            req);
                            break;

                        default:
                            MPIR_ERR_SETFATALANDJUMP4(mpi_errno, MPI_ERR_OTHER, "**ofid_poll",
                                                      "**ofid_poll %s %d %s %s", __SHORT_FILE__,
                                                      __LINE__, __func__, fi_strerror(e.err));
                    }

                    break;

                case FI_ECANCELED:
                    req = MPIDI_OFI_context_to_request(e.op_context);
                    /* Clean up the request. Reference MPIDI_OFI_recv_event.
                     * NOTE: assuming only the receive request can be cancelled and reach here
                     */
                    int event_id = MPIDI_OFI_REQUEST(req, event_id);
                    if (event_id == MPIDI_OFI_EVENT_DYNPROC_DONE) {
                        dynproc_done_event(vci, e.op_context, req);
                    } else {
                        /* assume it is a pending recv */
                        MPIR_STATUS_SET_CANCEL_BIT(req->status, TRUE);
                        MPIR_STATUS_SET_COUNT(req->status, 0);
                        if ((event_id == MPIDI_OFI_EVENT_RECV_PACK ||
                             event_id == MPIDI_OFI_EVENT_GET_HUGE) &&
                            MPIDI_OFI_REQUEST(req, noncontig.pack.pack_buffer)) {
                            MPL_free(MPIDI_OFI_REQUEST(req, noncontig.pack.pack_buffer));
                        } else if (MPIDI_OFI_ENABLE_PT2PT_NOPACK &&
                                   event_id == MPIDI_OFI_EVENT_RECV_NOPACK &&
                                   MPIDI_OFI_REQUEST(req, noncontig.nopack)) {
                            MPL_free(MPIDI_OFI_REQUEST(req, noncontig.nopack));
                        }
                        MPIR_Datatype_release_if_not_builtin(MPIDI_OFI_REQUEST(req, datatype));
                        MPIDI_Request_complete_fast(req);
                    }
                    break;

                case FI_ENOMSG:
                    req = MPIDI_OFI_context_to_request(e.op_context);
                    mpi_errno = peek_empty_event(vci, NULL, req);
                    break;

                default:
                    MPIR_ERR_SETFATALANDJUMP4(mpi_errno, MPI_ERR_OTHER, "**ofid_poll",
                                              "**ofid_poll %s %d %s %s", __SHORT_FILE__,
                                              __LINE__, __func__, fi_strerror(e.err));
            }

            break;

        default:
            MPIR_ERR_SETFATALANDJUMP4(mpi_errno, MPI_ERR_OTHER, "**ofid_poll",
                                      "**ofid_poll %s %d %s %s", __SHORT_FILE__, __LINE__,
                                      __func__, fi_strerror(errno));
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
