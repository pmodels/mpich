/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* This file includes all callback routines and the completion function of
 * receive callback for send-receive AM. All handlers on the packet issuing
 * side are named with suffix "_origin_cb", and all handlers on the
 * packet receiving side are named with "_target_msg_cb". */

#include "mpidimpl.h"

#include "mpidig.h"
#include "ch4r_callbacks.h"
#include "ch4r_request.h"
#include "ch4r_recv.h"

/* BEGIN internal function decls */
static int handle_unexp_cmpl(MPIR_Request * rreq);
static int do_send_target(void **data, size_t * p_data_sz, int *is_contig,
                          MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request * rreq);
/* END   internal function decls */

#undef FUNCNAME
#define FUNCNAME handle_unexp_cmpl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int handle_unexp_cmpl(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, in_use;
    MPIR_Comm *root_comm;
    MPIR_Request *match_req = NULL;
    size_t count;
    MPI_Aint last;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    size_t dt_sz;
    MPIR_Segment *segment_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_HANDLE_UNEXP_CMPL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_HANDLE_UNEXP_CMPL);

    /* MPIDI_CS_ENTER(); */
    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_UNEXP_DQUED) {
        if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_UNEXP_CLAIMED) {
            MPIDI_handle_unexp_mrecv(rreq);
        }
        /* MPIDI_CS_EXIT(); */
        goto fn_exit;
    }
    /* MPIDI_CS_EXIT(); */

    root_comm = MPIDIG_context_id_to_comm(MPIDIG_REQUEST(rreq, context_id));

    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_MATCHED) {
        match_req = (MPIR_Request *) MPIDIG_REQUEST(rreq, req->rreq.match_req);
    } else {
        /* MPIDI_CS_ENTER(); */
        if (root_comm)
            match_req =
                MPIDIG_dequeue_posted(MPIDIG_REQUEST(rreq, rank), MPIDIG_REQUEST(rreq, tag),
                                      MPIDIG_REQUEST(rreq, context_id),
                                      &MPIDIG_COMM(root_comm, posted_list));

        if (match_req) {
            MPIDIG_delete_unexp(rreq, &MPIDIG_COMM(root_comm, unexp_list));
            /* Decrement the counter twice, one for posted_list and the other for unexp_list */
            MPIR_Comm_release(root_comm);
            MPIR_Comm_release(root_comm);
        }
        /* MPIDI_CS_EXIT(); */
    }

    if (!match_req) {
        MPIDIG_REQUEST(rreq, req->status) &= ~MPIDIG_REQ_BUSY;
        goto fn_exit;
    }

    match_req->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
    match_req->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);

    MPIDI_Datatype_get_info(MPIDIG_REQUEST(match_req, count),
                            MPIDIG_REQUEST(match_req, datatype),
                            dt_contig, dt_sz, dt_ptr, dt_true_lb);
    MPIR_Datatype_get_size_macro(MPIDIG_REQUEST(match_req, datatype), dt_sz);
    MPIR_ERR_CHKANDJUMP(dt_sz == 0, mpi_errno, MPI_ERR_OTHER, "**dtype");

    if (MPIDIG_REQUEST(rreq, count) > dt_sz * MPIDIG_REQUEST(match_req, count)) {
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        count = MPIDIG_REQUEST(match_req, count);
    } else {
        rreq->status.MPI_ERROR = MPI_SUCCESS;
        count = MPIDIG_REQUEST(rreq, count) / dt_sz;
    }

    MPIR_STATUS_SET_COUNT(match_req->status, count * dt_sz);
    MPIDIG_REQUEST(rreq, count) = count;

    if (!dt_contig) {
        segment_ptr = MPIR_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1(segment_ptr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Recv MPIR_Segment_alloc");
        MPIR_Segment_init(MPIDIG_REQUEST(match_req, buffer), count,
                          MPIDIG_REQUEST(match_req, datatype), segment_ptr);

        last = count * dt_sz;
        MPIR_Segment_unpack(segment_ptr, 0, &last, MPIDIG_REQUEST(rreq, buffer));
        MPIR_Segment_free(segment_ptr);
        if (last != (MPI_Aint) (count * dt_sz)) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                             __FUNCTION__, __LINE__,
                                             MPI_ERR_TYPE, "**dtypemismatch", 0);
            match_req->status.MPI_ERROR = mpi_errno;
        }
    } else {
        MPIR_Memcpy((char *) MPIDIG_REQUEST(match_req, buffer) + dt_true_lb,
                    MPIDIG_REQUEST(rreq, buffer), count * dt_sz);
    }

    MPIDIG_REQUEST(rreq, req->status) &= ~MPIDIG_REQ_UNEXPECTED;
    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_PEER_SSEND) {
        mpi_errno = MPIDI_reply_ssend(rreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(match_req, datatype));
    MPL_free(MPIDIG_REQUEST(rreq, buffer));
    MPIR_Object_release_ref(rreq, &in_use);
    MPID_Request_complete(rreq);
    MPID_Request_complete(match_req);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_HANDLE_UNEXP_CMPL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME do_send_target
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int do_send_target(void **data, size_t * p_data_sz, int *is_contig,
                          MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request * rreq)
{
    int dt_contig, n_iov;
    MPI_Aint dt_true_lb, last, num_iov;
    MPIR_Datatype *dt_ptr;
    MPIR_Segment *segment_ptr;
    size_t data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_DO_SEND_TARGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_DO_SEND_TARGET);

    *target_cmpl_cb = MPIDI_recv_target_cmpl_cb;
    MPIDIG_REQUEST(rreq, req->seq_no) = OPA_fetch_and_add_int(&MPIDI_CH4_Global.nxt_seq_no, 1);

    if (p_data_sz == NULL)
        return MPI_SUCCESS;

    MPIDI_Datatype_get_info(MPIDIG_REQUEST(rreq, count), MPIDIG_REQUEST(rreq, datatype), dt_contig,
                            data_sz, dt_ptr, dt_true_lb);
    *is_contig = dt_contig;

    if (dt_contig) {
        *p_data_sz = data_sz;
        *data = (char *) MPIDIG_REQUEST(rreq, buffer) + dt_true_lb;
    } else {
        segment_ptr = MPIR_Segment_alloc();
        MPIR_Assert(segment_ptr);

        MPIR_Segment_init(MPIDIG_REQUEST(rreq, buffer), MPIDIG_REQUEST(rreq, count),
                          MPIDIG_REQUEST(rreq, datatype), segment_ptr);

        if (*p_data_sz > data_sz) {
            rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
            *p_data_sz = data_sz;
        }
        last = data_sz;
        MPIR_Segment_count_contig_blocks(segment_ptr, 0, &last, &num_iov);
        n_iov = (int) num_iov;
        MPIR_Assert(n_iov > 0);
        MPIDIG_REQUEST(rreq, req->iov) =
            (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec), MPL_MEM_BUFFER);
        MPIR_Assert(MPIDIG_REQUEST(rreq, req->iov));

        last = *p_data_sz;
        MPIR_Segment_pack_vector(segment_ptr, 0, &last, MPIDIG_REQUEST(rreq, req->iov), &n_iov);
        if (last != (MPI_Aint) * p_data_sz) {
            rreq->status.MPI_ERROR = MPI_ERR_TYPE;
        }
        *data = MPIDIG_REQUEST(rreq, req->iov);
        *p_data_sz = n_iov;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_RCV_NON_CONTIG;
        MPL_free(segment_ptr);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_DO_SEND_TARGET);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_check_cmpl_order
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_check_cmpl_order(MPIR_Request * req, MPIDIG_am_target_cmpl_cb target_cmpl_cb)
{
    int ret = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CHECK_CMPL_ORDER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CHECK_CMPL_ORDER);

    if (MPIDIG_REQUEST(req, req->seq_no) == (uint64_t) OPA_load_int(&MPIDI_CH4_Global.exp_seq_no)) {
        OPA_incr_int(&MPIDI_CH4_Global.exp_seq_no);
        ret = 1;
        goto fn_exit;
    }

    MPIDIG_REQUEST(req, req->target_cmpl_cb) = (void *) target_cmpl_cb;
    MPIDIG_REQUEST(req, req->request) = (uint64_t) req;
    /* MPIDI_CS_ENTER(); */
    DL_APPEND(MPIDI_CH4_Global.cmpl_list, req->dev.ch4.am.req);
    /* MPIDI_CS_EXIT(); */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CHECK_CMPL_ORDER);
    return ret;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_progress_cmpl_list
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIDI_progress_cmpl_list(void)
{
    MPIR_Request *req;
    MPIDIG_req_ext_t *curr, *tmp;
    MPIDIG_am_target_cmpl_cb target_cmpl_cb;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PROGRESS_CMPL_LIST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PROGRESS_CMPL_LIST);

    /* MPIDI_CS_ENTER(); */
  do_check_again:
    DL_FOREACH_SAFE(MPIDI_CH4_Global.cmpl_list, curr, tmp) {
        if (curr->seq_no == (uint64_t) OPA_load_int(&MPIDI_CH4_Global.exp_seq_no)) {
            DL_DELETE(MPIDI_CH4_Global.cmpl_list, curr);
            req = (MPIR_Request *) curr->request;
            target_cmpl_cb = (MPIDIG_am_target_cmpl_cb) curr->target_cmpl_cb;
            target_cmpl_cb(req);
            goto do_check_again;
        }
    }
    /* MPIDI_CS_EXIT(); */
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PROGRESS_CMPL_LIST);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_recv_target_cmpl_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_recv_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_RECV_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_RECV_TARGET_CMPL_CB);

    if (!MPIDI_check_cmpl_order(rreq, MPIDI_recv_target_cmpl_cb))
        return mpi_errno;

    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_RCV_NON_CONTIG) {
        MPL_free(MPIDIG_REQUEST(rreq, req->iov));
    }

    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_UNEXPECTED) {
        mpi_errno = handle_unexp_cmpl(rreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
    rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);

    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_PEER_SSEND) {
        mpi_errno = MPIDI_reply_ssend(rreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDIU_REQUEST_ANYSOURCE_PARTNER(rreq)) {
        int continue_matching = 1;
        MPIDIU_anysource_matched(MPIDIU_REQUEST_ANYSOURCE_PARTNER(rreq), MPIDI_NETMOD,
                                 &continue_matching);
        if (unlikely(MPIDIU_REQUEST_ANYSOURCE_PARTNER(rreq))) {
            MPIDIU_REQUEST_ANYSOURCE_PARTNER(MPIDIU_REQUEST_ANYSOURCE_PARTNER(rreq)) = NULL;
            MPIDIU_REQUEST_ANYSOURCE_PARTNER(rreq) = NULL;
        }
    }
#endif

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
    MPID_Request_complete(rreq);
  fn_exit:
    MPIDI_progress_cmpl_list();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_RECV_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_send_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_send_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SEND_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SEND_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SEND_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_send_long_lmt_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_send_long_lmt_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SEND_LONG_LMT_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SEND_LONG_LMT_ORIGIN_CB);
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(sreq, req->lreq).datatype);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SEND_LONG_LMT_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_ssend_ack_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_ssend_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SSEND_ACK_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SSEND_ACK_ORIGIN_CB);
    MPID_Request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SSEND_ACK_ORIGIN_CB);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_send_target_msg_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_send_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                             int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                             MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    MPIR_Comm *root_comm;
    MPIDIG_hdr_t *hdr = (MPIDIG_hdr_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SEND_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SEND_TARGET_MSG_CB);
    root_comm = MPIDIG_context_id_to_comm(hdr->context_id);
    if (root_comm) {
        /* MPIDI_CS_ENTER(); */
        rreq = MPIDIG_dequeue_posted(hdr->src_rank, hdr->tag, hdr->context_id,
                                     &MPIDIG_COMM(root_comm, posted_list));
        /* MPIDI_CS_EXIT(); */
    }

    if (rreq == NULL) {
        rreq = MPIDIG_am_request_create(MPIR_REQUEST_KIND__RECV, 2);
        MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIDIG_REQUEST(rreq, datatype) = MPI_BYTE;
        if (p_data_sz) {
            MPIDIG_REQUEST(rreq, buffer) = (char *) MPL_malloc(*p_data_sz, MPL_MEM_BUFFER);
            MPIDIG_REQUEST(rreq, count) = *p_data_sz;
        } else {
            MPIDIG_REQUEST(rreq, buffer) = NULL;
            MPIDIG_REQUEST(rreq, count) = 0;
        }
        MPIDIG_REQUEST(rreq, rank) = hdr->src_rank;
        MPIDIG_REQUEST(rreq, tag) = hdr->tag;
        MPIDIG_REQUEST(rreq, context_id) = hdr->context_id;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_BUSY;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_UNEXPECTED;
        /* MPIDI_CS_ENTER(); */
        if (root_comm) {
            MPIR_Comm_add_ref(root_comm);
            MPIDIG_enqueue_unexp(rreq, &MPIDIG_COMM(root_comm, unexp_list));
        } else {
            MPIDIG_enqueue_unexp(rreq, MPIDIG_context_id_to_uelist(hdr->context_id));
        }
        /* MPIDI_CS_EXIT(); */
    } else {
        /* rreq != NULL <=> root_comm != NULL */
        MPIR_Assert(root_comm);
        /* Decrement the refcnt when popping a request out from posted_list */
        MPIR_Comm_release(root_comm);
        MPIDIG_REQUEST(rreq, rank) = hdr->src_rank;
        MPIDIG_REQUEST(rreq, tag) = hdr->tag;
        MPIDIG_REQUEST(rreq, context_id) = hdr->context_id;
    }

    *req = rreq;

    mpi_errno = do_send_target(data, p_data_sz, is_contig, target_cmpl_cb, rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SEND_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_send_long_req_target_msg_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_send_long_req_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                      int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                      MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    MPIR_Comm *root_comm;
    MPIDIG_hdr_t *hdr = (MPIDIG_hdr_t *) am_hdr;
    MPIDIG_send_long_req_msg_t *lreq_hdr = (MPIDIG_send_long_req_msg_t *) am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SEND_LONG_REQ_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SEND_LONG_REQ_TARGET_MSG_CB);

    root_comm = MPIDIG_context_id_to_comm(hdr->context_id);
    if (root_comm) {
        /* MPIDI_CS_ENTER(); */
        rreq = MPIDIG_dequeue_posted(hdr->src_rank, hdr->tag, hdr->context_id,
                                     &MPIDIG_COMM(root_comm, posted_list));
        /* MPIDI_CS_EXIT(); */
    }

    if (rreq == NULL) {
        rreq = MPIDIG_am_request_create(MPIR_REQUEST_KIND__RECV, 2);
        MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

        MPIDIG_REQUEST(rreq, buffer) = NULL;
        MPIDIG_REQUEST(rreq, datatype) = MPI_BYTE;
        MPIDIG_REQUEST(rreq, count) = lreq_hdr->data_sz;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_LONG_RTS;
        MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr) = lreq_hdr->sreq_ptr;
        MPIDIG_REQUEST(rreq, rank) = hdr->src_rank;
        MPIDIG_REQUEST(rreq, tag) = hdr->tag;
        MPIDIG_REQUEST(rreq, context_id) = hdr->context_id;

        /* MPIDI_CS_ENTER(); */
        if (root_comm) {
            MPIR_Comm_add_ref(root_comm);
            MPIDIG_enqueue_unexp(rreq, &MPIDIG_COMM(root_comm, unexp_list));
        } else {
            MPIDIG_enqueue_unexp(rreq,
                                 MPIDIG_context_id_to_uelist(MPIDIG_REQUEST(rreq, context_id)));
        }
        /* MPIDI_CS_EXIT(); */
    } else {
        /* Matching receive was posted, tell the netmod */
        MPIR_Comm_release(root_comm);   /* -1 for posted_list */
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_LONG_RTS;
        MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr) = lreq_hdr->sreq_ptr;
        MPIDIG_REQUEST(rreq, rank) = hdr->src_rank;
        MPIDIG_REQUEST(rreq, tag) = hdr->tag;
        MPIDIG_REQUEST(rreq, context_id) = hdr->context_id;

#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (MPIDIU_REQUEST(rreq, is_local))
            mpi_errno = MPIDI_SHM_am_recv(rreq);
        else
#endif
        {
            mpi_errno = MPIDI_NM_am_recv(rreq);
        }

        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SEND_LONG_REQ_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_send_long_lmt_target_msg_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_send_long_lmt_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                      int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                      MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_Request *rreq;
    MPIDIG_send_long_lmt_msg_t *lmt_hdr = (MPIDIG_send_long_lmt_msg_t *) am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SEND_LONG_LMT_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SEND_LONG_LMT_TARGET_MSG_CB);

    rreq = (MPIR_Request *) lmt_hdr->rreq_ptr;
    MPIR_Assert(rreq);
    mpi_errno = do_send_target(data, p_data_sz, is_contig, target_cmpl_cb, rreq);
    *req = rreq;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SEND_LONG_LMT_TARGET_MSG_CB);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_ssend_target_msg_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_ssend_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                              int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                              MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDIG_ssend_req_msg_t *msg_hdr = (MPIDIG_ssend_req_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SSEND_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SSEND_TARGET_MSG_CB);

    mpi_errno =
        MPIDI_send_target_msg_cb(handler_id, am_hdr, data, p_data_sz, is_contig, target_cmpl_cb,
                                 req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_Assert(req);
    MPIDIG_REQUEST(*req, req->rreq.peer_req_ptr) = msg_hdr->sreq_ptr;
    MPIDIG_REQUEST(*req, req->status) |= MPIDIG_REQ_PEER_SSEND;
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SSEND_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_ssend_ack_target_msg_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_ssend_ack_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                  int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                  MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    MPIDIG_ssend_ack_msg_t *msg_hdr = (MPIDIG_ssend_ack_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SSEND_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SSEND_ACK_TARGET_MSG_CB);

    sreq = (MPIR_Request *) msg_hdr->sreq_ptr;
    MPID_Request_complete(sreq);

    if (req)
        *req = NULL;
    if (target_cmpl_cb)
        *target_cmpl_cb = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SSEND_ACK_TARGET_MSG_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_send_long_ack_target_msg_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_send_long_ack_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                      int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                      MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    MPIDIG_send_long_ack_msg_t *msg_hdr = (MPIDIG_send_long_ack_msg_t *) am_hdr;
    MPIDIG_send_long_lmt_msg_t send_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SEND_LONG_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SEND_LONG_ACK_TARGET_MSG_CB);

    sreq = (MPIR_Request *) msg_hdr->sreq_ptr;
    MPIR_Assert(sreq != NULL);

    /* Start the main data transfer */
    send_hdr.rreq_ptr = msg_hdr->rreq_ptr;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDIU_REQUEST(sreq, is_local))
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

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (target_cmpl_cb)
        *target_cmpl_cb = MPIDI_send_origin_cb;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SEND_LONG_ACK_TARGET_MSG_CB);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_comm_abort_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_comm_abort_origin_cb(MPIR_Request * sreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_COMM_ABORT_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_COMM_ABORT_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_COMM_ABORT_ORIGIN_CB);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_comm_abort_target_msg_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_comm_abort_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                   int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                   MPIR_Request ** req)
{
    MPIDIG_hdr_t *hdr = (MPIDIG_hdr_t *) am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_COMM_ABORT_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_COMM_ABORT_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_COMM_ABORT_TARGET_MSG_CB);
    MPL_exit(hdr->tag);
    return MPI_SUCCESS;
}
