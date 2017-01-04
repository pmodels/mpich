/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_EVENTS_H_INCLUDED
#define OFI_EVENTS_H_INCLUDED

#include "ofi_impl.h"
#include "ofi_am_impl.h"
#include "ofi_am_events.h"
#include "ofi_control.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_huge_event(struct fi_cq_tagged_entry *wc,
                                                      MPIR_Request * req);

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_cqe_get_source(struct fi_cq_tagged_entry *wc)
{
    if (MPIDI_OFI_ENABLE_DATA)
        return wc->data;
    else
        return MPIDI_OFI_init_get_source(wc->tag);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_peek_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_peek_event(struct fi_cq_tagged_entry *wc,
                                                  MPIR_Request * rreq)
{
    size_t count;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_NETMOD_PEEK_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_NETMOD_PEEK_EVENT);
    MPIDI_OFI_REQUEST(rreq, util_id) = MPIDI_OFI_PEEK_FOUND;
    rreq->status.MPI_SOURCE = MPIDI_OFI_cqe_get_source(wc);
    rreq->status.MPI_TAG = MPIDI_OFI_init_get_tag(wc->tag);
    count = wc->len;
    rreq->status.MPI_ERROR = MPI_SUCCESS;
    MPIR_STATUS_SET_COUNT(rreq->status, count);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_NETMOD_PEEK_EVENT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_peek_empty_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_peek_empty_event(struct fi_cq_tagged_entry *wc,
                                                        MPIR_Request * rreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_NETMOD_PEEK_EMPTY_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_NETMOD_PEEK_EMPTY_EVENT);
    MPIDI_OFI_dynamic_process_request_t *ctrl;

    switch (MPIDI_OFI_REQUEST(rreq, event_id)) {
    case MPIDI_OFI_EVENT_PEEK:
        MPIDI_OFI_REQUEST(rreq, util_id) = MPIDI_OFI_PEEK_NOT_FOUND;
        rreq->status.MPI_ERROR = MPI_SUCCESS;
        break;

    case MPIDI_OFI_EVENT_ACCEPT_PROBE:
        ctrl = (MPIDI_OFI_dynamic_process_request_t *) rreq;
        ctrl->done = MPIDI_OFI_PEEK_NOT_FOUND;
        break;

    default:
        MPIR_Assert(0);
        break;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_NETMOD_PEEK_EMPTY_EVENT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_recv_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_recv_event(struct fi_cq_tagged_entry *wc,
                                                  MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint last;
    size_t count;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_RECV_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_RECV_EVENT);

    rreq->status.MPI_ERROR = MPI_SUCCESS;
    rreq->status.MPI_SOURCE = MPIDI_OFI_cqe_get_source(wc);
    rreq->status.MPI_TAG = MPIDI_OFI_init_get_tag(wc->tag);
    count = wc->len;
    MPIR_STATUS_SET_COUNT(rreq->status, count);

#ifdef MPIDI_BUILD_CH4_SHM

    if (MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(rreq)) {
        int continue_matching = 1;

        MPIDI_CH4R_anysource_matched(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(rreq), MPIDI_CH4R_NETMOD,
                                     &continue_matching);

        /* It is always possible to cancel a request on shm side w/o an aux thread */

        /* Decouple requests */
        if (unlikely(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(rreq))) {
            MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(rreq)) = NULL;
            MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(rreq) = NULL;
        }

        if (!continue_matching)
            goto fn_exit;
    }

#endif

    if (MPIDI_OFI_REQUEST(rreq, noncontig)) {
        last = count;
        MPID_Segment_unpack(&MPIDI_OFI_REQUEST(rreq, noncontig->segment), 0, &last,
                            MPIDI_OFI_REQUEST(rreq, noncontig->pack_buffer));
        MPL_free(MPIDI_OFI_REQUEST(rreq, noncontig));
        if (last != (MPI_Aint) count) {
            rreq->status.MPI_ERROR =
                MPIR_Err_create_code(MPI_SUCCESS,
                                     MPIR_ERR_RECOVERABLE,
                                     __FUNCTION__, __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
        }
    }

    dtype_release_if_not_builtin(MPIDI_OFI_REQUEST(rreq, datatype));

    /* If syncronous, ack and complete when the ack is done */
    if (unlikely(MPIDI_OFI_is_tag_sync(wc->tag))) {
        uint64_t ss_bits = MPIDI_OFI_init_sendtag(MPIDI_OFI_REQUEST(rreq, util_id),
                                                  MPIDI_OFI_REQUEST(rreq, util_comm->rank),
                                                  rreq->status.MPI_TAG,
                                                  MPIDI_OFI_SYNC_SEND_ACK);
        MPIR_Comm *c = MPIDI_OFI_REQUEST(rreq, util_comm);
        int r = rreq->status.MPI_SOURCE;
        mpi_errno = MPIDI_OFI_send_handler(MPIDI_OFI_EP_TX_TAG(0), NULL, 0, NULL,
                                           MPIDI_OFI_REQUEST(rreq, util_comm->rank),
                                           MPIDI_OFI_comm_to_phys(c, r, MPIDI_OFI_API_TAG),
                                           ss_bits, NULL, MPIDI_OFI_DO_INJECT,
                                           MPIDI_OFI_CALL_NO_LOCK);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    MPIDI_CH4U_request_complete(rreq);

    /* Polling loop will check for truncation */
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_RECV_EVENT);
    return mpi_errno;
  fn_fail:
    rreq->status.MPI_ERROR = mpi_errno;
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_recv_huge_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_recv_huge_event(struct fi_cq_tagged_entry *wc,
                                                       MPIR_Request * rreq)
{
    MPIDI_OFI_huge_recv_t *recv;
    MPIR_Comm *comm_ptr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_RECV_HUGE_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_RECV_HUGE_EVENT);

    /* Look up the receive sequence number and chunk queue */
    comm_ptr = MPIDI_OFI_REQUEST(rreq, util_comm);
    recv =
        (MPIDI_OFI_huge_recv_t *) MPIDI_OFI_map_lookup(MPIDI_OFI_COMM(comm_ptr).huge_recv_counters,
                                                       MPIDI_OFI_cqe_get_source(wc));
    if (recv == MPIDI_OFI_MAP_NOT_FOUND) {
        recv = (MPIDI_OFI_huge_recv_t *) MPL_calloc(sizeof(*recv), 1);
        MPIDI_OFI_map_set(MPIDI_OFI_COMM(comm_ptr).huge_recv_counters,
                          MPIDI_OFI_cqe_get_source(wc), recv);
    }

    recv->event_id = MPIDI_OFI_EVENT_GET_HUGE;
    recv->localreq = rreq;
    recv->done_fn = MPIDI_OFI_recv_event;
    recv->wc = *wc;
    MPIDI_OFI_get_huge_event(NULL, (MPIR_Request *) recv);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_RECV_HUGE_EVENT);
    return MPI_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_send_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_event(struct fi_cq_tagged_entry *wc,
                                                  MPIR_Request * sreq)
{
    int c;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_SEND_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_SEND_EVENT);

    MPIR_cc_decr(sreq->cc_ptr, &c);

    if (c == 0) {
        if (MPIDI_OFI_REQUEST(sreq, noncontig))
            MPL_free(MPIDI_OFI_REQUEST(sreq, noncontig));

        dtype_release_if_not_builtin(MPIDI_OFI_REQUEST(sreq, datatype));
        MPIR_Request_free(sreq);
    }   /* c != 0, ssend */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_SEND_EVENT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_send_huge_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_huge_event(struct fi_cq_tagged_entry *wc,
                                                       MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    int c;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_SEND_EVENT_HUGE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_SEND_EVENT_HUGE);

    MPIR_cc_decr(sreq->cc_ptr, &c);

    if (c == 0) {
        MPIR_Comm *comm;
        void *ptr;
        MPIDI_OFI_huge_counter_t *cntr;
        comm = MPIDI_OFI_REQUEST(sreq, util_comm);
        ptr =
            MPIDI_OFI_map_lookup(MPIDI_OFI_COMM(comm).huge_send_counters,
                                 MPIDI_OFI_REQUEST(sreq, util_id));
        MPIR_Assert(ptr != MPIDI_OFI_MAP_NOT_FOUND);
        cntr = (MPIDI_OFI_huge_counter_t *) ptr;
        cntr->outstanding--;
        if (cntr->outstanding == 0) {
            MPIDI_OFI_send_control_t ctrl;
            uint64_t key;
            int key_back;
            MPIDI_OFI_map_erase(MPIDI_OFI_COMM(comm).huge_send_counters,
                                MPIDI_OFI_REQUEST(sreq, util_id));
            key = fi_mr_key(cntr->mr);
            key_back = (key >> MPIDI_Global.huge_rma_shift);
            MPIDI_OFI_index_allocator_free(MPIDI_OFI_COMM(comm).rma_id_allocator, key_back);
            MPIDI_OFI_CALL_NOLOCK(fi_close(&cntr->mr->fid), mr_unreg);
            MPL_free(ptr);
            ctrl.type = MPIDI_OFI_CTRL_HUGE_CLEANUP;
            MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_do_control_send
                                   (&ctrl, NULL, 0, MPIDI_OFI_REQUEST(sreq, util_id), comm, NULL,
                                    FALSE /* no lock */));
        }

        if (MPIDI_OFI_REQUEST(sreq, noncontig))
            MPL_free(MPIDI_OFI_REQUEST(sreq, noncontig));

        dtype_release_if_not_builtin(MPIDI_OFI_REQUEST(sreq, datatype));
        MPIR_Request_free(sreq);
    }   /* c != 0, ssend */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_SEND_EVENT_HUGE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_ssend_ack_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_ssend_ack_event(struct fi_cq_tagged_entry *wc,
                                                       MPIR_Request * sreq)
{
    int mpi_errno;
    MPIDI_OFI_ssendack_request_t *req = (MPIDI_OFI_ssendack_request_t *) sreq;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_SSEND_ACK_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_SSEND_ACK_EVENT);
    mpi_errno = MPIDI_OFI_send_event(NULL, req->signal_req);
    MPIDI_OFI_ssendack_request_t_tls_free(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_SSEND_ACK_EVENT);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX uintptr_t MPIDI_OFI_recv_rbase(MPIDI_OFI_huge_recv_t * recv)
{
#ifdef USE_OFI_MR_SCALABLE
    return 0;
#else
    return recv->remote_info.send_buf;
#endif
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_get_huge_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_huge_event(struct fi_cq_tagged_entry *wc,
                                                      MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_huge_recv_t *recv = (MPIDI_OFI_huge_recv_t *) req;
    uint64_t remote_key;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_GETHUGE_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_GETHUGE_EVENT);

    if (recv->localreq && recv->cur_offset != 0) {
        size_t bytesSent = recv->cur_offset - MPIDI_Global.max_send;
        size_t bytesLeft = recv->remote_info.msgsize - bytesSent - MPIDI_Global.max_send;
        size_t bytesToGet =
            (bytesLeft <= MPIDI_Global.max_send) ? bytesLeft : MPIDI_Global.max_send;

        if (bytesToGet == 0ULL) {
            MPIDI_OFI_send_control_t ctrl;
            recv->wc.len = recv->cur_offset;
            recv->done_fn(&recv->wc, recv->localreq);
            ctrl.type = MPIDI_OFI_CTRL_HUGEACK;
            MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_do_control_send
                                   (&ctrl, NULL, 0, recv->remote_info.origin_rank, recv->comm_ptr,
                                    recv->remote_info.ackreq, FALSE));
            /* "recv" and maps will be freed in MPIDI_OFI_get_huge_cleanup */
            goto fn_exit;
        }

        if (MPIDI_OFI_ENABLE_MR_SCALABLE)
            remote_key = recv->remote_info.rma_key << MPIDI_Global.huge_rma_shift;
        else
            remote_key = recv->remote_info.rma_key;

        MPIDI_OFI_cntr_incr();
        MPIDI_OFI_CALL_RETRY(fi_read(MPIDI_OFI_EP_TX_RMA(0),    /* endpoint     */
                                     (void *) ((uintptr_t) recv->wc.buf + recv->cur_offset),    /* local buffer */
                                     bytesToGet,        /* bytes        */
                                     NULL,      /* descriptor   */
                                     MPIDI_OFI_comm_to_phys(recv->comm_ptr, recv->remote_info.origin_rank, MPIDI_OFI_API_MSG),  /* Destination  */
                                     MPIDI_OFI_recv_rbase(recv) + recv->cur_offset,     /* remote maddr */
                                     remote_key,        /* Key          */
                                     (void *) &recv->context), rdma_readfrom,   /* Context */
                             MPIDI_OFI_CALL_NO_LOCK);
        recv->cur_offset += bytesToGet;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_GETHUGE_EVENT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_chunk_done_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_chunk_done_event(struct fi_cq_tagged_entry *wc,
                                                        MPIR_Request * req)
{
    int c;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_CHUNK_DONE_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_CHUNK_DONE_EVENT);

    MPIDI_OFI_chunk_request *creq = (MPIDI_OFI_chunk_request *) req;
    MPIR_cc_decr(creq->parent->cc_ptr, &c);

    if (c == 0)
        MPIR_Request_free(creq->parent);

    MPL_free(creq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_CHUNK_DONE_EVENT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_inject_emu_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_inject_emu_event(struct fi_cq_tagged_entry *wc,
                                                        MPIR_Request * req)
{
    int incomplete;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_INJECT_EMU_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_INJECT_EMU_EVENT);

    MPIR_cc_decr(req->cc_ptr, &incomplete);

    if (!incomplete) {
        MPL_free(MPIDI_OFI_REQUEST(req, util.inject_buf));
        MPIR_Request_free(req);
        OPA_decr_int(&MPIDI_Global.am_inflight_inject_emus);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_INJECT_EMU_EVENT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_rma_done_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_rma_done_event(struct fi_cq_tagged_entry *wc,
                                                      MPIR_Request * in_req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_OFI_RMA_DONE_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_OFI_RMA_DONE_EVENT);

    MPIDI_OFI_win_request_t *req = (MPIDI_OFI_win_request_t *) in_req;
    MPIDI_OFI_win_request_complete(req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_OFI_RMA_DONE_EVENT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_accept_probe_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_accept_probe_event(struct fi_cq_tagged_entry *wc,
                                                          MPIR_Request * rreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_OFI_ACCEPT_PROBE_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_OFI_ACCEPT_PROBE_EVENT);
    MPIDI_OFI_dynamic_process_request_t *ctrl = (MPIDI_OFI_dynamic_process_request_t *) rreq;
    ctrl->source = MPIDI_OFI_cqe_get_source(wc);
    ctrl->tag = MPIDI_OFI_init_get_tag(wc->tag);
    ctrl->msglen = wc->len;
    ctrl->done = MPIDI_OFI_PEEK_FOUND;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_OFI_ACCEPT_PROBE_EVENT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_dynproc_done_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_dynproc_done_event(struct fi_cq_tagged_entry *wc,
                                                          MPIR_Request * rreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_OFI_DYNPROC_DONE_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_OFI_DYNPROC_DONE_EVENT);
    MPIDI_OFI_dynamic_process_request_t *ctrl = (MPIDI_OFI_dynamic_process_request_t *) rreq;
    ctrl->done++;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_OFI_DYNPROC_DONE_EVENT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_am_isend_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_isend_event(struct fi_cq_tagged_entry *wc,
                                                      MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_header_t *msg_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_HANDLE_SEND_COMPLETION);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_HANDLE_SEND_COMPLETION);

    msg_hdr = &MPIDI_OFI_AMREQUEST_HDR(sreq, msg_hdr);
    MPID_Request_complete(sreq);       /* FIXME: Should not call MPIDI in NM ? */

    switch (msg_hdr->am_type) {
    case MPIDI_AMTYPE_LMT_ACK:
    case MPIDI_AMTYPE_LMT_REQ:
        goto fn_exit;

    default:
        break;
    }

    if (MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer)) {
        MPL_free(MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer));
        MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer) = NULL;
    }

    mpi_errno = MPIDIG_global.origin_cbs[msg_hdr->handler_id] (sreq);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_HANDLE_SEND_COMPLETION);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_am_recv_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_recv_event(struct fi_cq_tagged_entry *wc,
                                                     MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_header_t *am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_HANDLE_RECV_COMPLETION);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_HANDLE_RECV_COMPLETION);

    am_hdr = (MPIDI_OFI_am_header_t *) wc->buf;

    switch (am_hdr->am_type) {
    case MPIDI_AMTYPE_SHORT_HDR:
        mpi_errno = MPIDI_OFI_handle_short_am_hdr(am_hdr, am_hdr->payload);

        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        break;

    case MPIDI_AMTYPE_SHORT:
        mpi_errno = MPIDI_OFI_handle_short_am(am_hdr);

        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        break;

    case MPIDI_AMTYPE_LMT_REQ:
        mpi_errno = MPIDI_OFI_handle_long_am(am_hdr);

        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        break;

    case MPIDI_AMTYPE_LMT_ACK:
        mpi_errno = MPIDI_OFI_handle_lmt_ack(am_hdr);

        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        break;

    default:
        MPIR_Assert(0);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_HANDLE_RECV_COMPLETION);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_am_read_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_read_event(struct fi_cq_tagged_entry *wc,
                                                     MPIR_Request * dont_use_me)
{
    int mpi_errno = MPI_SUCCESS;
    void *netmod_context = NULL;
    MPIR_Request *rreq;
    MPIDI_OFI_am_request_t *ofi_req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_HANDLE_READ_COMPLETION);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_HANDLE_READ_COMPLETION);

    ofi_req = MPL_container_of(wc->op_context, MPIDI_OFI_am_request_t, context);
    ofi_req->req_hdr->lmt_cntr--;

    if (ofi_req->req_hdr->lmt_cntr)
        goto fn_exit;

    rreq = (MPIR_Request *) ofi_req->req_hdr->rreq_ptr;
    mpi_errno = MPIDI_OFI_dispatch_ack(MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info).src_rank,
                                       MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info).context_id,
                                       MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info).sreq_ptr,
                                       MPIDI_AMTYPE_LMT_ACK, netmod_context);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPID_Request_complete(rreq);       /* FIXME: Should not call MPIDI in NM ? */
    ofi_req->req_hdr->target_cmpl_cb(rreq);
  fn_exit:
    MPIDI_CH4R_release_buf((void *) ofi_req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_HANDLE_READ_COMPLETION);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_am_repost_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_repost_event(struct fi_cq_tagged_entry *wc,
                                                       MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_REPOST_BUFFER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_REPOST_BUFFER);

    mpi_errno = MPIDI_OFI_repost_buffer(wc->op_context, rreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_REPOST_BUFFER);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_dispatch_function(struct fi_cq_tagged_entry *wc,
                                                         MPIR_Request * req, int buffered)
{
    int mpi_errno;

    if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_SEND)) {
        mpi_errno = MPIDI_OFI_send_event(wc, req);
        goto fn_exit;
    }
    else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_RECV)) {
        mpi_errno = MPIDI_OFI_recv_event(wc, req);
        goto fn_exit;
    }
    else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_RMA_DONE)) {
        mpi_errno = MPIDI_OFI_rma_done_event(wc, req);
        goto fn_exit;
    }
    else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_AM_SEND)) {
        mpi_errno = MPIDI_OFI_am_isend_event(wc, req);
        goto fn_exit;
    }
    else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_AM_RECV)) {
        mpi_errno = MPIDI_OFI_am_recv_event(wc, req);

        if (unlikely((wc->flags & FI_MULTI_RECV) && !buffered))
            MPIDI_OFI_am_repost_event(wc, req);

        goto fn_exit;
    }
    else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_AM_READ)) {
        mpi_errno = MPIDI_OFI_am_read_event(wc, req);
        goto fn_exit;
    }
    else if (unlikely(1)) {
        switch (MPIDI_OFI_REQUEST(req, event_id)) {
        case MPIDI_OFI_EVENT_AM_MULTI:
            mpi_errno = MPIDI_OFI_am_repost_event(wc, req);
            break;

        case MPIDI_OFI_EVENT_PEEK:
            mpi_errno = MPIDI_OFI_peek_event(wc, req);
            break;

        case MPIDI_OFI_EVENT_RECV_HUGE:
            mpi_errno = MPIDI_OFI_recv_huge_event(wc, req);
            break;

        case MPIDI_OFI_EVENT_SEND_HUGE:
            mpi_errno = MPIDI_OFI_send_huge_event(wc, req);
            break;

        case MPIDI_OFI_EVENT_SSEND_ACK:
            mpi_errno = MPIDI_OFI_ssend_ack_event(wc, req);
            break;

        case MPIDI_OFI_EVENT_GET_HUGE:
            mpi_errno = MPIDI_OFI_get_huge_event(wc, req);
            break;

        case MPIDI_OFI_EVENT_CHUNK_DONE:
            mpi_errno = MPIDI_OFI_chunk_done_event(wc, req);
            break;

        case MPIDI_OFI_EVENT_INJECT_EMU:
            mpi_errno = MPIDI_OFI_inject_emu_event(wc, req);
            break;

        case MPIDI_OFI_EVENT_DYNPROC_DONE:
            mpi_errno = MPIDI_OFI_dynproc_done_event(wc, req);
            break;

        case MPIDI_OFI_EVENT_ACCEPT_PROBE:
            mpi_errno = MPIDI_OFI_accept_probe_event(wc, req);
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

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_get_buffered
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_buffered(struct fi_cq_tagged_entry *wc, ssize_t num)
{
    int rc = 0;

    if ((MPIDI_Global.cq_buff_head != MPIDI_Global.cq_buff_tail) ||
        !slist_empty(&MPIDI_Global.cq_buff_list)) {
        if (MPIDI_Global.cq_buff_head != MPIDI_Global.cq_buff_tail) {
            wc[0] = MPIDI_Global.cq_buffered[MPIDI_Global.cq_buff_tail].cq_entry;
            MPIDI_Global.cq_buff_tail = (MPIDI_Global.cq_buff_tail + 1) % MPIDI_OFI_NUM_CQ_BUFFERED;
        }
        else {
            MPIDI_OFI_cq_list_t *MPIDI_OFI_cq_list_entry;
            struct slist_entry *entry = slist_remove_head(&MPIDI_Global.cq_buff_list);
            MPIDI_OFI_cq_list_entry = MPL_container_of(entry, MPIDI_OFI_cq_list_t, entry);
            wc[0] = MPIDI_OFI_cq_list_entry->cq_entry;
            MPL_free((void *) MPIDI_OFI_cq_list_entry);
        }

        rc = 1;
    }

    return rc;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_handle_cq_entries
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_handle_cq_entries(struct fi_cq_tagged_entry *wc,
                                                         ssize_t num, int buffered)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIR_Request *req;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_HANDLE_CQ_ENTRIES);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_HANDLE_CQ_ENTRIES);

    for (i = 0; i < num; i++) {
        req = MPIDI_OFI_context_to_request(wc[i].op_context);
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_dispatch_function(&wc[i], req, buffered));
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_HANDLE_CQ_ENTRIES);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_handle_cq_error
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_handle_cq_error(ssize_t ret)
{
    int mpi_errno = MPI_SUCCESS;
    struct fi_cq_err_entry e;
    MPIR_Request *req;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_HANDLE_CQ_ERROR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_HANDLE_CQ_ERROR);

    switch (ret) {
    case -FI_EAVAIL:
        fi_cq_readerr(MPIDI_Global.p2p_cq, &e, 0);

        switch (e.err) {
        case FI_ETRUNC:
            req = MPIDI_OFI_context_to_request(e.op_context);

            switch (req->kind) {
            case MPIR_REQUEST_KIND__SEND:
                mpi_errno = MPIDI_OFI_dispatch_function(NULL, req, 0);
                break;

            case MPIR_REQUEST_KIND__RECV:
                mpi_errno = MPIDI_OFI_dispatch_function((struct fi_cq_tagged_entry *) &e, req, 0);
                req->status.MPI_ERROR = MPI_ERR_TRUNCATE;
                break;

            default:
                MPIR_ERR_SETFATALANDJUMP4(mpi_errno, MPI_ERR_OTHER, "**ofid_poll",
                                          "**ofid_poll %s %d %s %s", __SHORT_FILE__,
                                          __LINE__, FCNAME, fi_strerror(e.err));
            }

            break;

        case FI_ECANCELED:
            req = MPIDI_OFI_context_to_request(e.op_context);
            MPIR_STATUS_SET_CANCEL_BIT(req->status, TRUE);
            break;

        case FI_ENOMSG:
            req = MPIDI_OFI_context_to_request(e.op_context);
            MPIDI_OFI_peek_empty_event(NULL, req);
            break;
        }

        break;

    default:
        MPIR_ERR_SETFATALANDJUMP4(mpi_errno, MPI_ERR_OTHER, "**ofid_poll",
                                  "**ofid_poll %s %d %s %s", __SHORT_FILE__, __LINE__,
                                  FCNAME, fi_strerror(errno));
        break;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_HANDLE_CQ_ERROR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* OFI_EVENTS_H_INCLUDED */
