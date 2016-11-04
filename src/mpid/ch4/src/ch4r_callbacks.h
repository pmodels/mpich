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
#ifndef CH4R_CALLBACKS_H_INCLUDED
#define CH4R_CALLBACKS_H_INCLUDED

#include "ch4r_request.h"
#include "ch4r_recv.h"

static inline int recv_cmpl_handler_fn(MPIR_Request * rreq);

#undef FUNCNAME
#define FUNCNAME handle_unexp_cmpl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int handle_unexp_cmpl(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIR_Comm *root_comm;
    MPIR_Request *match_req = NULL;
    uint64_t msg_tag;
    size_t count;
    MPI_Aint last;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    size_t dt_sz;
    MPID_Segment *segment_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_HANDLE_UNEXP_CMPL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_HANDLE_UNEXP_CMPL);

    /* MPIDI_CS_ENTER(); */
    if (MPIDI_CH4U_REQUEST(rreq, req->status) & MPIDI_CH4U_REQ_UNEXP_DQUED) {
        if (MPIDI_CH4U_REQUEST(rreq, req->status) & MPIDI_CH4U_REQ_UNEXP_CLAIMED) {
            MPIDI_CH4I_handle_unexp_mrecv(rreq);
        }
        /* MPIDI_CS_EXIT(); */
        goto fn_exit;
    }
    /* MPIDI_CS_EXIT(); */

    msg_tag = MPIDI_CH4U_REQUEST(rreq, tag);
    root_comm = MPIDI_CH4U_context_id_to_comm(MPIDI_CH4U_get_context(msg_tag));

    if (MPIDI_CH4U_REQUEST(rreq, req->status) & MPIDI_CH4U_REQ_MATCHED) {
        match_req = (MPIR_Request *) MPIDI_CH4U_REQUEST(rreq, req->rreq.match_req);
    }
    else {
        /* MPIDI_CS_ENTER(); */
        if (root_comm)
            match_req =
                MPIDI_CH4U_dequeue_posted(msg_tag, &MPIDI_CH4U_COMM(root_comm, posted_list));

        if (match_req) {
            MPIDI_CH4U_delete_unexp(rreq, &MPIDI_CH4U_COMM(root_comm, unexp_list));
            /* Decrement the counter twice, one for posted_list and the other for unexp_list */
            MPIR_Comm_release(root_comm);
            MPIR_Comm_release(root_comm);
        }
        /* MPIDI_CS_EXIT(); */
    }

    if (!match_req) {
        MPIDI_CH4U_REQUEST(rreq, req->status) &= ~MPIDI_CH4U_REQ_BUSY;
        goto fn_exit;
    }

    match_req->status.MPI_SOURCE = MPIDI_CH4U_REQUEST(rreq, src_rank);
    match_req->status.MPI_TAG = MPIDI_CH4U_get_tag(msg_tag);

    MPIDI_Datatype_get_info(MPIDI_CH4U_REQUEST(match_req, count),
                            MPIDI_CH4U_REQUEST(match_req, datatype),
                            dt_contig, dt_sz, dt_ptr, dt_true_lb);
    MPID_Datatype_get_size_macro(MPIDI_CH4U_REQUEST(match_req, datatype), dt_sz);

    if (MPIDI_CH4U_REQUEST(rreq, count) > dt_sz * MPIDI_CH4U_REQUEST(match_req, count)) {
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        count = MPIDI_CH4U_REQUEST(match_req, count);
    }
    else {
        rreq->status.MPI_ERROR = MPI_SUCCESS;
        count = MPIDI_CH4U_REQUEST(rreq, count) / dt_sz;
    }

    MPIR_STATUS_SET_COUNT(match_req->status, count * dt_sz);
    MPIDI_CH4U_REQUEST(rreq, count) = count;

    if (!dt_contig) {
        segment_ptr = MPIDU_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1(segment_ptr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Recv MPIDU_Segment_alloc");
        MPIDU_Segment_init(MPIDI_CH4U_REQUEST(match_req, buffer), count,
                           MPIDI_CH4U_REQUEST(match_req, datatype), segment_ptr, 0);

        last = count * dt_sz;
        MPIDU_Segment_unpack(segment_ptr, 0, &last, MPIDI_CH4U_REQUEST(rreq, buffer));
        MPIDU_Segment_free(segment_ptr);
        if (last != (MPI_Aint) (count * dt_sz)) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                             __FUNCTION__, __LINE__,
                                             MPI_ERR_TYPE, "**dtypemismatch", 0);
            match_req->status.MPI_ERROR = mpi_errno;
        }
    }
    else {
        MPIR_Memcpy((char *) MPIDI_CH4U_REQUEST(match_req, buffer) + dt_true_lb,
                    MPIDI_CH4U_REQUEST(rreq, buffer), count * dt_sz);
    }

    MPIDI_CH4U_REQUEST(rreq, req->status) &= ~MPIDI_CH4U_REQ_UNEXPECTED;
    if (MPIDI_CH4U_REQUEST(rreq, req->status) & MPIDI_CH4U_REQ_PEER_SSEND) {
        mpi_errno = MPIDI_CH4I_reply_ssend(rreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    dtype_release_if_not_builtin(MPIDI_CH4U_REQUEST(match_req, datatype));
    MPL_free(MPIDI_CH4U_REQUEST(rreq, buffer));
    MPIR_Object_release_ref(rreq, &c);
    MPIDI_CH4I_am_request_complete(rreq);
    MPIDI_CH4I_am_request_complete(match_req);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_HANDLE_UNEXP_CMPL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME do_send_target
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int do_send_target(void **data,
                                 size_t * p_data_sz,
                                 int *is_contig,
                                 MPIDI_NM_am_completion_handler_fn *
                                 cmpl_handler_fn, MPIR_Request * rreq)
{
    int dt_contig, n_iov;
    MPI_Aint dt_true_lb, last, num_iov;
    MPIR_Datatype *dt_ptr;
    MPID_Segment *segment_ptr;
    size_t data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_DO_SEND_TARGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_DO_SEND_TARGET);

    *cmpl_handler_fn = recv_cmpl_handler_fn;
    MPIDI_CH4U_REQUEST(rreq, req->seq_no) = OPA_fetch_and_add_int(&MPIDI_CH4_Global.nxt_seq_no, 1);

    if (p_data_sz == NULL)
        return MPI_SUCCESS;

    MPIDI_Datatype_get_info(MPIDI_CH4U_REQUEST(rreq, count),
                            MPIDI_CH4U_REQUEST(rreq, datatype),
                            dt_contig, data_sz, dt_ptr, dt_true_lb);
    *is_contig = dt_contig;

    if (dt_contig) {
        *p_data_sz = data_sz;
        *data = (char *) MPIDI_CH4U_REQUEST(rreq, buffer) + dt_true_lb;
    }
    else {
        segment_ptr = MPIDU_Segment_alloc();
        MPIR_Assert(segment_ptr);

        MPIDU_Segment_init(MPIDI_CH4U_REQUEST(rreq, buffer),
                           MPIDI_CH4U_REQUEST(rreq, count),
                           MPIDI_CH4U_REQUEST(rreq, datatype), segment_ptr, 0);

        if (*p_data_sz > data_sz) {
            rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
            *p_data_sz = data_sz;
        }
        last = data_sz;
        MPIDU_Segment_count_contig_blocks(segment_ptr, 0, &last, &num_iov);
        n_iov = (int) num_iov;
        MPIR_Assert(n_iov > 0);
        MPIDI_CH4U_REQUEST(rreq, req->iov) =
            (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec));
        MPIR_Assert(MPIDI_CH4U_REQUEST(rreq, req->iov));

        last = *p_data_sz;
        MPIDU_Segment_pack_vector(segment_ptr, 0, &last, MPIDI_CH4U_REQUEST(rreq, req->iov),
                                  &n_iov);
        if (last != (MPI_Aint) * p_data_sz) {
            rreq->status.MPI_ERROR = MPI_ERR_TYPE;
        }
        *data = MPIDI_CH4U_REQUEST(rreq, req->iov);
        *p_data_sz = n_iov;
        MPIDI_CH4U_REQUEST(rreq, req->status) |= MPIDI_CH4U_REQ_RCV_NON_CONTIG;
        MPL_free(segment_ptr);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DO_SEND_TARGET);
    return MPI_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH4I_check_cmpl_order
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4I_check_cmpl_order(MPIR_Request * req,
                                              MPIDI_NM_am_completion_handler_fn cmpl_handler_fn)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4I_CHECK_CMPL_ORDER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4I_CHECK_CMPL_ORDER);

    if (MPIDI_CH4U_REQUEST(req, req->seq_no) ==
        (uint64_t) OPA_load_int(&MPIDI_CH4_Global.exp_seq_no)) {
        OPA_incr_int(&MPIDI_CH4_Global.exp_seq_no);
        return 1;
    }

    MPIDI_CH4U_REQUEST(req, req->cmpl_handler_fn) = (void *) cmpl_handler_fn;
    MPIDI_CH4U_REQUEST(req, req->request) = (uint64_t) req;
    /* MPIDI_CS_ENTER(); */
    MPL_DL_APPEND(MPIDI_CH4_Global.cmpl_list, req->dev.ch4.ch4u.req);
    /* MPIDI_CS_EXIT(); */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4I_CHECK_CMPL_ORDER);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4I_progress_cmpl_list
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_CH4I_progress_cmpl_list(void)
{
    MPIR_Request *req;
    MPIDI_CH4U_req_ext_t *curr, *tmp;
    MPIDI_NM_am_completion_handler_fn cmpl_handler_fn;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4I_PROGRESS_CMPL_LIST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4I_PROGRESS_CMPL_LIST);

    /* MPIDI_CS_ENTER(); */
  do_check_again:
    MPL_DL_FOREACH_SAFE(MPIDI_CH4_Global.cmpl_list, curr, tmp) {
        if (curr->seq_no == (uint64_t) OPA_load_int(&MPIDI_CH4_Global.exp_seq_no)) {
            MPL_DL_DELETE(MPIDI_CH4_Global.cmpl_list, curr);
            req = (MPIR_Request *) curr->request;
            cmpl_handler_fn = (MPIDI_NM_am_completion_handler_fn) curr->cmpl_handler_fn;
            cmpl_handler_fn(req);
            goto do_check_again;
        }
    }
    /* MPIDI_CS_EXIT(); */
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4I_PROGRESS_CMPL_LIST);
}

#undef FUNCNAME
#define FUNCNAME recv_cmpl_handler_fn
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int recv_cmpl_handler_fn(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_RECV_CMPL_HANDLER_FN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_RECV_CMPL_HANDLER_FN);

    if (!MPIDI_CH4I_check_cmpl_order(rreq, recv_cmpl_handler_fn))
        return mpi_errno;

    if (MPIDI_CH4U_REQUEST(rreq, req->status) & MPIDI_CH4U_REQ_RCV_NON_CONTIG) {
        MPL_free(MPIDI_CH4U_REQUEST(rreq, req->iov));
    }

    if (MPIDI_CH4U_REQUEST(rreq, req->status) & MPIDI_CH4U_REQ_UNEXPECTED) {
        mpi_errno = handle_unexp_cmpl(rreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    rreq->status.MPI_SOURCE = MPIDI_CH4U_REQUEST(rreq, src_rank);
    rreq->status.MPI_TAG = MPIDI_CH4U_get_tag(MPIDI_CH4U_REQUEST(rreq, tag));

    if (MPIDI_CH4U_REQUEST(rreq, req->status) & MPIDI_CH4U_REQ_PEER_SSEND) {
        mpi_errno = MPIDI_CH4I_reply_ssend(rreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

#ifdef MPIDI_BUILD_CH4_SHM
    if (MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(rreq)) {
        int continue_matching = 1;
        MPIDI_CH4R_anysource_matched(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(rreq), MPIDI_CH4R_NETMOD,
                                     &continue_matching);
        if (unlikely(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(rreq))) {
            MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(rreq)) = NULL;
            MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(rreq) = NULL;
        }
    }
#endif

    dtype_release_if_not_builtin(MPIDI_CH4U_REQUEST(rreq, datatype));
    MPIDI_CH4I_am_request_complete(rreq);
  fn_exit:
    MPIDI_CH4I_progress_cmpl_list();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_RECV_CMPL_HANDLER_FN);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_send_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_send_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_SEND_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_SEND_TX_HANDLER);
    MPIDI_CH4I_am_request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_SEND_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_send_long_lmt_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_send_long_lmt_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_SEND_LONG_LMT_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_SEND_LONG_LMT_TX_HANDLER);
    dtype_release_if_not_builtin(MPIDI_CH4U_REQUEST(sreq, req->lreq).datatype);
    MPIDI_CH4I_am_request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_SEND_LONG_LMT_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_ssend_ack_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_ssend_ack_origin_cmpl_handler(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_SSEND_ACK_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_SSEND_ACK_TX_HANDLER);
    MPIDI_CH4I_am_request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_SSEND_ACK_TX_HANDLER);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_send_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_send_target_handler(int handler_id, void *am_hdr,
                                                 void **data,
                                                 size_t * p_data_sz,
                                                 int *is_contig,
                                                 MPIDI_NM_am_completion_handler_fn *
                                                 cmpl_handler_fn, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    MPIR_Comm *root_comm;
    MPIDI_CH4U_hdr_t *hdr = (MPIDI_CH4U_hdr_t *) am_hdr;
    MPIR_Context_id_t context_id;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_SEND_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_SEND_HANDLER);
    context_id = MPIDI_CH4U_get_context(hdr->msg_tag);
    root_comm = MPIDI_CH4U_context_id_to_comm(context_id);
    if (root_comm) {
        /* MPIDI_CS_ENTER(); */
        rreq = MPIDI_CH4U_dequeue_posted(hdr->msg_tag, &MPIDI_CH4U_COMM(root_comm, posted_list));
        /* MPIDI_CS_EXIT(); */
    }

    if (rreq == NULL) {
        rreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__RECV);
        MPIDI_CH4U_REQUEST(rreq, datatype) = MPI_BYTE;
        if (p_data_sz) {
            MPIDI_CH4U_REQUEST(rreq, buffer) = (char *) MPL_malloc(*p_data_sz);
            MPIDI_CH4U_REQUEST(rreq, count) = *p_data_sz;
        }
        else {
            MPIDI_CH4U_REQUEST(rreq, buffer) = NULL;
            MPIDI_CH4U_REQUEST(rreq, count) = 0;
        }
        MPIDI_CH4U_REQUEST(rreq, tag) = hdr->msg_tag;
        MPIDI_CH4U_REQUEST(rreq, src_rank) = hdr->src_rank;
        MPIDI_CH4U_REQUEST(rreq, req->status) |= MPIDI_CH4U_REQ_BUSY;
        MPIDI_CH4U_REQUEST(rreq, req->status) |= MPIDI_CH4U_REQ_UNEXPECTED;
        /* MPIDI_CS_ENTER(); */
        if (root_comm) {
            MPIR_Comm_add_ref(root_comm);
            MPIDI_CH4U_enqueue_unexp(rreq, &MPIDI_CH4U_COMM(root_comm, unexp_list));
        }
        else {
            MPIDI_CH4U_enqueue_unexp(rreq, MPIDI_CH4U_context_id_to_uelist(context_id));
        }
        /* MPIDI_CS_EXIT(); */
    }
    else {
        /* rreq != NULL <=> root_comm != NULL */
        MPIR_Assert(root_comm);
        /* Decrement the refcnt when popping a request out from posted_list */
        MPIR_Comm_release(root_comm);
        MPIDI_CH4U_REQUEST(rreq, src_rank) = hdr->src_rank;
        MPIDI_CH4U_REQUEST(rreq, tag) = hdr->msg_tag;
    }

    *req = rreq;

    mpi_errno = do_send_target(data, p_data_sz, is_contig, cmpl_handler_fn, rreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_SEND_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_send_long_req_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_send_long_req_target_handler(int handler_id, void *am_hdr,
                                                          void **data,
                                                          size_t * p_data_sz,
                                                          int *is_contig,
                                                          MPIDI_NM_am_completion_handler_fn *
                                                          cmpl_handler_fn, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    MPIR_Comm *root_comm;
    MPIDI_CH4U_hdr_t *hdr = (MPIDI_CH4U_hdr_t *) am_hdr;
    MPIDI_CH4U_send_long_req_msg_t *lreq_hdr = (MPIDI_CH4U_send_long_req_msg_t *) am_hdr;
    MPIR_Context_id_t context_id;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_SEND_LONG_REQ_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_SEND_LONG_REQ_HANDLER);

    context_id = MPIDI_CH4U_get_context(hdr->msg_tag);
    root_comm = MPIDI_CH4U_context_id_to_comm(context_id);
    if (root_comm) {
        /* MPIDI_CS_ENTER(); */
        rreq = MPIDI_CH4U_dequeue_posted(hdr->msg_tag, &MPIDI_CH4U_COMM(root_comm, posted_list));
        /* MPIDI_CS_EXIT(); */
    }

    if (rreq == NULL) {
        rreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__RECV);

        MPIDI_CH4U_REQUEST(rreq, buffer) = NULL;
        MPIDI_CH4U_REQUEST(rreq, datatype) = MPI_BYTE;
        MPIDI_CH4U_REQUEST(rreq, count) = lreq_hdr->data_sz;
        MPIDI_CH4U_REQUEST(rreq, req->status) |= MPIDI_CH4U_REQ_LONG_RTS;
        MPIDI_CH4U_REQUEST(rreq, req->rreq.peer_req_ptr) = lreq_hdr->sreq_ptr;
        MPIDI_CH4U_REQUEST(rreq, tag) = hdr->msg_tag;
        MPIDI_CH4U_REQUEST(rreq, src_rank) = hdr->src_rank;

        /* MPIDI_CS_ENTER(); */
        if (root_comm) {
            MPIR_Comm_add_ref(root_comm);
            MPIDI_CH4U_enqueue_unexp(rreq, &MPIDI_CH4U_COMM(root_comm, unexp_list));
        }
        else {
            MPIDI_CH4U_enqueue_unexp(rreq, MPIDI_CH4U_context_id_to_uelist(context_id));
        }
        /* MPIDI_CS_EXIT(); */
    }
    else {
        /* Matching receive was posted, tell the netmod */
        MPIR_Comm_release(root_comm);   /* -1 for posted_list */
        MPIDI_CH4U_REQUEST(rreq, req->status) |= MPIDI_CH4U_REQ_LONG_RTS;
        MPIDI_CH4U_REQUEST(rreq, req->rreq.peer_req_ptr) = lreq_hdr->sreq_ptr;
        MPIDI_CH4U_REQUEST(rreq, tag) = hdr->msg_tag;
        MPIDI_CH4U_REQUEST(rreq, src_rank) = hdr->src_rank;
        mpi_errno = MPIDI_NM_am_recv(rreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_SEND_LONG_REQ_HANDLER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_send_long_lmt_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_send_long_lmt_target_handler(int handler_id, void *am_hdr,
                                                          void **data,
                                                          size_t * p_data_sz,
                                                          int *is_contig,
                                                          MPIDI_NM_am_completion_handler_fn *
                                                          cmpl_handler_fn, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_Request *rreq;
    MPIDI_CH4U_send_long_lmt_msg_t *lmt_hdr = (MPIDI_CH4U_send_long_lmt_msg_t *) am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_SEND_LONG_LMT_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_SEND_LONG_LMT_HANDLER);

    rreq = (MPIR_Request *) lmt_hdr->rreq_ptr;
    MPIR_Assert(rreq);
    mpi_errno = do_send_target(data, p_data_sz, is_contig, cmpl_handler_fn, rreq);
    *req = rreq;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_SEND_LONG_LMT_HANDLER);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_ssend_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_ssend_target_handler(int handler_id, void *am_hdr,
                                                  void **data,
                                                  size_t * p_data_sz,
                                                  int *is_contig,
                                                  MPIDI_NM_am_completion_handler_fn *
                                                  cmpl_handler_fn, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_CH4U_ssend_req_msg_t *msg_hdr = (MPIDI_CH4U_ssend_req_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_SSEND_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_SSEND_HANDLER);

    mpi_errno = MPIDI_CH4U_send_target_handler(handler_id, am_hdr,
                                               data, p_data_sz, is_contig, cmpl_handler_fn, req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_Assert(req);
    MPIDI_CH4U_REQUEST(*req, req->rreq.peer_req_ptr) = msg_hdr->sreq_ptr;
    MPIDI_CH4U_REQUEST(*req, req->status) |= MPIDI_CH4U_REQ_PEER_SSEND;
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_SSEND_HANDLER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_ssend_ack_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_ssend_ack_target_handler(int handler_id, void *am_hdr,
                                                      void **data,
                                                      size_t * p_data_sz, int *is_contig,
                                                      MPIDI_NM_am_completion_handler_fn *
                                                      cmpl_handler_fn, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    MPIDI_CH4U_ssend_ack_msg_t *msg_hdr = (MPIDI_CH4U_ssend_ack_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_SSEND_ACK_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_SSEND_ACK_HANDLER);

    sreq = (MPIR_Request *) msg_hdr->sreq_ptr;
    MPIDI_CH4I_am_request_complete(sreq);

    if (req)
        *req = NULL;
    if (cmpl_handler_fn)
        *cmpl_handler_fn = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_SSEND_ACK_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_send_long_ack_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_send_long_ack_target_handler(int handler_id, void *am_hdr,
                                                          void **data,
                                                          size_t * p_data_sz, int *is_contig,
                                                          MPIDI_NM_am_completion_handler_fn *
                                                          cmpl_handler_fn, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    MPIDI_CH4U_send_long_ack_msg_t *msg_hdr = (MPIDI_CH4U_send_long_ack_msg_t *) am_hdr;
    MPIDI_CH4U_send_long_lmt_msg_t send_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_SEND_LONG_ACK_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_SEND_LONG_ACK_HANDLER);

    sreq = (MPIR_Request *) msg_hdr->sreq_ptr;
    MPIR_Assert(sreq != NULL);

    /* Start the main data transfer */
    send_hdr.rreq_ptr = msg_hdr->rreq_ptr;
    mpi_errno =
        MPIDI_NM_am_isend_reply(MPIDI_CH4U_get_context(MPIDI_CH4U_REQUEST(sreq, req->lreq).msg_tag),
                                MPIDI_CH4U_REQUEST(sreq, src_rank), MPIDI_CH4U_SEND_LONG_LMT,
                                &send_hdr, sizeof(send_hdr),
                                MPIDI_CH4U_REQUEST(sreq, req->lreq).src_buf,
                                MPIDI_CH4U_REQUEST(sreq, req->lreq).count,
                                MPIDI_CH4U_REQUEST(sreq, req->lreq).datatype, sreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (cmpl_handler_fn)
        *cmpl_handler_fn = MPIDI_CH4U_send_origin_cmpl_handler;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_SEND_LONG_ACK_HANDLER);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4R_CALLBACKS_H_INCLUDED */
