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

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_progress_cmpl_list
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_CH4U_progress_cmpl_list(void)
{
    MPIR_Request *req;
    MPIDI_CH4U_req_ext_t *curr, *tmp;
    MPIDI_NM_am_completion_handler_fn cmpl_handler_fn;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_PROGRESS_CMPL_LIST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_PROGRESS_CMPL_LIST);

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_PROGRESS_CMPL_LIST);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_check_cmpl_order
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_check_cmpl_order(MPIR_Request * req,
                                              MPIDI_NM_am_completion_handler_fn cmpl_handler_fn)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_CHECK_CMPL_ORDER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_CHECK_CMPL_ORDER);

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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_CHECK_CMPL_ORDER);
    return 0;
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
#define FUNCNAME MPIDI_CH4U_put_ack_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_put_ack_origin_cmpl_handler(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_PUT_ACK_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_PUT_ACK_TX_HANDLER);
    MPIDI_CH4I_am_request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_PUT_ACK_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_get_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_get_cmpl_handler(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS, i, c;
    size_t data_sz, offset;
    MPIDI_CH4U_get_ack_msg_t get_ack;
    struct iovec *iov;
    char *p_data;
    uintptr_t base;
    MPIR_Win *win;
    MPIR_Context_id_t context_id;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_GET_CMPL_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_GET_CMPL_HANDLER);

    if (!MPIDI_CH4U_check_cmpl_order(req, MPIDI_CH4U_get_cmpl_handler))
        return mpi_errno;

    base = MPIDI_CH4U_REQUEST(req, req->greq.addr);

    MPIR_cc_incr(req->cc_ptr, &c);
    get_ack.greq_ptr = MPIDI_CH4U_REQUEST(req, req->greq.greq_ptr);
    win = MPIDI_CH4U_REQUEST(req, req->greq.win_ptr);
    context_id = MPIDI_CH4U_win_to_context(win);
    if (MPIDI_CH4U_REQUEST(req, req->greq.n_iov) == 0) {
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
        if (MPIDI_SHM == MPIDI_CH4U_REQUEST(req, caller)) {
            mpi_errno = MPIDI_SHM_am_isend_reply(context_id,
                                                 MPIDI_CH4U_REQUEST(req, src_rank),
                                                 MPIDI_CH4U_GET_ACK,
                                                 &get_ack, sizeof(get_ack),
                                                 (void *) MPIDI_CH4U_REQUEST(req, req->greq.addr),
                                                 MPIDI_CH4U_REQUEST(req, req->greq.count),
                                                 MPIDI_CH4U_REQUEST(req, req->greq.datatype), req);
        }
        else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
            mpi_errno = MPIDI_NM_am_isend_reply(context_id,
                                                MPIDI_CH4U_REQUEST(req, src_rank),
                                                MPIDI_CH4U_GET_ACK,
                                                &get_ack, sizeof(get_ack),
                                                (void *) MPIDI_CH4U_REQUEST(req, req->greq.addr),
                                                MPIDI_CH4U_REQUEST(req, req->greq.count),
                                                MPIDI_CH4U_REQUEST(req, req->greq.datatype), req);
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
        }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
        MPIDI_CH4I_am_request_complete(req);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    iov = (struct iovec *) MPIDI_CH4U_REQUEST(req, req->greq.dt_iov);

    data_sz = 0;
    for (i = 0; i < MPIDI_CH4U_REQUEST(req, req->greq.n_iov); i++) {
        data_sz += iov[i].iov_len;
    }

    p_data = (char *) MPL_malloc(data_sz);
    MPIR_Assert(p_data);

    offset = 0;
    for (i = 0; i < MPIDI_CH4U_REQUEST(req, req->greq.n_iov); i++) {
        /* Adjust a window base address */
        iov[i].iov_base = (char *) iov[i].iov_base + base;
        MPIR_Memcpy(p_data + offset, iov[i].iov_base, iov[i].iov_len);
        offset += iov[i].iov_len;
    }

    MPL_free(MPIDI_CH4U_REQUEST(req, req->greq.dt_iov));
    MPIDI_CH4U_REQUEST(req, req->greq.dt_iov) = (void *) p_data;

#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    if (MPIDI_SHM == MPIDI_CH4U_REQUEST(req, caller)) {
        mpi_errno = MPIDI_SHM_am_isend_reply(context_id,
                                             MPIDI_CH4U_REQUEST(req, src_rank),
                                             MPIDI_CH4U_GET_ACK,
                                             &get_ack, sizeof(get_ack), p_data, data_sz, MPI_BYTE,
                                             req);
    }
    else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
        mpi_errno = MPIDI_NM_am_isend_reply(context_id,
                                            MPIDI_CH4U_REQUEST(req, src_rank),
                                            MPIDI_CH4U_GET_ACK,
                                            &get_ack, sizeof(get_ack), p_data, data_sz, MPI_BYTE,
                                            req);
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
    MPIDI_CH4I_am_request_complete(req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPIDI_CH4U_progress_cmpl_list();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_GET_CMPL_HANDLER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_acc_ack_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_acc_ack_origin_cmpl_handler(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACC_ACK_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACC_ACK_TX_HANDLER);
    MPIDI_CH4I_am_request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACC_ACK_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_get_acc_ack_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_get_acc_ack_origin_cmpl_handler(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_GET_ACC_ACK_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_GET_ACC_ACK_TX_HANDLER);
    MPL_free(MPIDI_CH4U_REQUEST(req, req->areq.data));

    win = MPIDI_CH4U_REQUEST(req, req->areq.win_ptr);
    /* MPIDI_CS_ENTER(); */
    OPA_decr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */

    MPIDI_CH4I_am_request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_GET_ACC_ACK_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_cswap_ack_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_cswap_ack_origin_cmpl_handler(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_CSWAP_ACK_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_CSWAP_ACK_TX_HANDLER);

    MPL_free(MPIDI_CH4U_REQUEST(req, req->creq.data));
    win = MPIDI_CH4U_REQUEST(req, req->creq.win_ptr);
    /* MPIDI_CS_ENTER(); */
    OPA_decr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */

    MPIDI_CH4I_am_request_complete(req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_CSWAP_ACK_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_get_ack_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_get_ack_origin_cmpl_handler(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_GET_ACK_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_GET_ACK_TX_HANDLER);

    if (MPIDI_CH4U_REQUEST(req, req->greq.dt_iov)) {
        MPL_free(MPIDI_CH4U_REQUEST(req, req->greq.dt_iov));
    }

    win = MPIDI_CH4U_REQUEST(req, req->greq.win_ptr);
    /* MPIDI_CS_ENTER(); */
    OPA_decr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */

    MPIDI_CH4I_am_request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_GET_ACK_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_put_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_put_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_PUT_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_PUT_TX_HANDLER);
    MPIDI_CH4I_am_request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_PUT_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_cswap_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_cswap_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_CSWAP_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_CSWAP_TX_HANDLER);
    MPIDI_CH4I_am_request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_CSWAP_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_acc_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_acc_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACC_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACC_TX_HANDLER);
    MPIDI_CH4I_am_request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACC_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_put_data_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_put_data_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_PUT_DATA_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_PUT_DATA_TX_HANDLER);
    MPIDI_CH4I_am_request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_PUT_DATA_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_acc_data_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_acc_data_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACC_DATA_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACC_DATA_TX_HANDLER);
    MPIDI_CH4I_am_request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACC_DATA_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_put_iov_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_put_iov_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_PUT_IOV_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_PUT_IOV_TX_HANDLER);
    MPIDI_CH4I_am_request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_PUT_IOV_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_acc_iov_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_acc_iov_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACC_IOV_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACC_IOV_TX_HANDLER);
    MPIDI_CH4I_am_request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACC_IOV_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_get_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_get_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_GET_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_GET_TX_HANDLER);
    MPIDI_CH4I_am_request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_GET_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_reply_ssend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_reply_ssend(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIDI_CH4U_ssend_ack_msg_t ack_msg;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_REPLY_SSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_REPLY_SSEND);
    MPIR_cc_incr(rreq->cc_ptr, &c);
    ack_msg.sreq_ptr = MPIDI_CH4U_REQUEST(rreq, req->rreq.peer_req_ptr);

#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    if (MPIDI_SHM == MPIDI_CH4U_REQUEST(rreq, caller)) {
        mpi_errno = MPIDI_SHM_am_isend_reply(MPIDI_CH4U_get_context(MPIDI_CH4U_REQUEST(rreq, tag)),
                                             MPIDI_CH4U_REQUEST(rreq, src_rank),
                                             MPIDI_CH4U_SSEND_ACK, &ack_msg, sizeof(ack_msg),
                                             NULL, 0, MPI_DATATYPE_NULL, rreq);
    }
    else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
        mpi_errno = MPIDI_NM_am_isend_reply(MPIDI_CH4U_get_context(MPIDI_CH4U_REQUEST(rreq, tag)),
                                            MPIDI_CH4U_REQUEST(rreq, src_rank),
                                            MPIDI_CH4U_SSEND_ACK, &ack_msg, sizeof(ack_msg),
                                            NULL, 0, MPI_DATATYPE_NULL, rreq);
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_REPLY_SSEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_ack_put
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_ack_put(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH4U_put_ack_msg_t ack_msg;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACK_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACK_PUT);

    ack_msg.preq_ptr = MPIDI_CH4U_REQUEST(rreq, req->preq.preq_ptr);
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    if (MPIDI_SHM == MPIDI_CH4U_REQUEST(rreq, caller)) {
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(MPIDI_CH4U_win_to_context
                                        (MPIDI_CH4U_REQUEST(rreq, req->preq.win_ptr)),
                                        MPIDI_CH4U_REQUEST(rreq, src_rank), MPIDI_CH4U_PUT_ACK,
                                        &ack_msg, sizeof(ack_msg));
    }
    else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDI_CH4U_win_to_context
                                       (MPIDI_CH4U_REQUEST(rreq, req->preq.win_ptr)),
                                       MPIDI_CH4U_REQUEST(rreq, src_rank), MPIDI_CH4U_PUT_ACK,
                                       &ack_msg, sizeof(ack_msg));
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACK_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_ack_cswap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_ack_cswap(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIDI_CH4U_cswap_ack_msg_t ack_msg;
    void *result_addr;
    size_t data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACK_CSWAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACK_CSWAP);

    MPIDI_Datatype_check_size(MPIDI_CH4U_REQUEST(rreq, req->creq.datatype), 1, data_sz);
    result_addr = ((char *) MPIDI_CH4U_REQUEST(rreq, req->creq.data)) + data_sz;

    MPIR_cc_incr(rreq->cc_ptr, &c);
    ack_msg.req_ptr = MPIDI_CH4U_REQUEST(rreq, req->creq.creq_ptr);

#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    if (MPIDI_SHM == MPIDI_CH4U_REQUEST(rreq, caller)) {
        mpi_errno =
            MPIDI_SHM_am_isend_reply(MPIDI_CH4U_win_to_context
                                     (MPIDI_CH4U_REQUEST(rreq, req->creq.win_ptr)),
                                     MPIDI_CH4U_REQUEST(rreq, src_rank), MPIDI_CH4U_CSWAP_ACK,
                                     &ack_msg, sizeof(ack_msg), result_addr, 1,
                                     MPIDI_CH4U_REQUEST(rreq, req->creq.datatype), rreq);
    }
    else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
        mpi_errno =
            MPIDI_NM_am_isend_reply(MPIDI_CH4U_win_to_context
                                    (MPIDI_CH4U_REQUEST(rreq, req->creq.win_ptr)),
                                    MPIDI_CH4U_REQUEST(rreq, src_rank), MPIDI_CH4U_CSWAP_ACK,
                                    &ack_msg, sizeof(ack_msg), result_addr, 1,
                                    MPIDI_CH4U_REQUEST(rreq, req->creq.datatype), rreq);
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACK_CSWAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_ack_acc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_ack_acc(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH4U_acc_ack_msg_t ack_msg;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACK_ACC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACK_ACC);

    ack_msg.req_ptr = MPIDI_CH4U_REQUEST(rreq, req->areq.req_ptr);
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    if (MPIDI_SHM == MPIDI_CH4U_REQUEST(rreq, caller)) {
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(MPIDI_CH4U_win_to_context
                                        (MPIDI_CH4U_REQUEST(rreq, req->areq.win_ptr)),
                                        MPIDI_CH4U_REQUEST(rreq, src_rank), MPIDI_CH4U_ACC_ACK,
                                        &ack_msg, sizeof(ack_msg));
    }
    else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDI_CH4U_win_to_context
                                       (MPIDI_CH4U_REQUEST(rreq, req->areq.win_ptr)),
                                       MPIDI_CH4U_REQUEST(rreq, src_rank), MPIDI_CH4U_ACC_ACK,
                                       &ack_msg, sizeof(ack_msg));
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */

    win = MPIDI_CH4U_REQUEST(rreq, req->areq.win_ptr);
    /* MPIDI_CS_ENTER(); */
    OPA_decr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACK_ACC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_ack_get_acc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_ack_get_acc(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIDI_CH4U_acc_ack_msg_t ack_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACK_GET_ACC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACK_GET_ACC);

    MPIR_cc_incr(rreq->cc_ptr, &c);
    ack_msg.req_ptr = MPIDI_CH4U_REQUEST(rreq, req->areq.req_ptr);

#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    if (MPIDI_SHM == MPIDI_CH4U_REQUEST(rreq, caller)) {
        mpi_errno =
            MPIDI_SHM_am_isend_reply(MPIDI_CH4U_win_to_context
                                     (MPIDI_CH4U_REQUEST(rreq, req->areq.win_ptr)),
                                     MPIDI_CH4U_REQUEST(rreq, src_rank), MPIDI_CH4U_GET_ACC_ACK,
                                     &ack_msg, sizeof(ack_msg), MPIDI_CH4U_REQUEST(rreq,
                                                                                   req->areq.data),
                                     MPIDI_CH4U_REQUEST(rreq, req->areq.data_sz), MPI_BYTE, rreq);
    }
    else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
        mpi_errno =
            MPIDI_NM_am_isend_reply(MPIDI_CH4U_win_to_context
                                    (MPIDI_CH4U_REQUEST(rreq, req->areq.win_ptr)),
                                    MPIDI_CH4U_REQUEST(rreq, src_rank), MPIDI_CH4U_GET_ACC_ACK,
                                    &ack_msg, sizeof(ack_msg), MPIDI_CH4U_REQUEST(rreq,
                                                                                  req->areq.data),
                                    MPIDI_CH4U_REQUEST(rreq, req->areq.data_sz), MPI_BYTE, rreq);
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACK_GET_ACC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_unexp_mrecv_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_unexp_mrecv_cmpl_handler(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    uint64_t msg_tag;
    size_t message_sz;
    MPI_Aint last;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    size_t data_sz, dt_sz;
    MPID_Segment *segment_ptr;
    void *buf;
    int count;
    MPI_Datatype datatype;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_UNEXP_MRECV_CMPL_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_UNEXP_MRECV_CMPL_HANDLER);

    msg_tag = MPIDI_CH4U_REQUEST(rreq, tag);
    rreq->status.MPI_SOURCE = MPIDI_CH4U_REQUEST(rreq, src_rank);
    rreq->status.MPI_TAG = MPIDI_CH4U_get_tag(msg_tag);

    buf = MPIDI_CH4U_REQUEST(rreq, req->rreq.mrcv_buffer);
    count = MPIDI_CH4U_REQUEST(rreq, req->rreq.mrcv_count);
    datatype = MPIDI_CH4U_REQUEST(rreq, req->rreq.mrcv_datatype);

    message_sz = MPIDI_CH4U_REQUEST(rreq, count);
    MPID_Datatype_get_size_macro(datatype, dt_sz);

    if (message_sz > count * dt_sz) {
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
    }
    else {
        rreq->status.MPI_ERROR = MPI_SUCCESS;
        count = message_sz / dt_sz;
    }

    MPIR_STATUS_SET_COUNT(rreq->status, count * dt_sz);
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (!dt_contig) {
        segment_ptr = MPIDU_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1(segment_ptr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Recv MPIDU_Segment_alloc");
        MPIDU_Segment_init(buf, count, datatype, segment_ptr, 0);

        last = count * dt_sz;
        MPIDU_Segment_unpack(segment_ptr, 0, &last, MPIDI_CH4U_REQUEST(rreq, buffer));
        MPIDU_Segment_free(segment_ptr);
        if (last != (MPI_Aint) (count * dt_sz)) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                             __FUNCTION__, __LINE__,
                                             MPI_ERR_TYPE, "**dtypemismatch", 0);
            rreq->status.MPI_ERROR = mpi_errno;
        }
    }
    else {
        MPIR_Memcpy((char *) buf + dt_true_lb, MPIDI_CH4U_REQUEST(rreq, buffer), data_sz);
    }

    MPL_free(MPIDI_CH4U_REQUEST(rreq, buffer));
    rreq->kind = MPIR_REQUEST_KIND__RECV;

    if (MPIDI_CH4U_REQUEST(rreq, req->status) & MPIDI_CH4U_REQ_PEER_SSEND) {
        mpi_errno = MPIDI_CH4U_reply_ssend(rreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    MPIDI_CH4I_am_request_complete(rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_UNEXP_MRECV_CMPL_HANDLER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_unexp_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_unexp_cmpl_handler(MPIR_Request * rreq)
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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_UNEXP_CMPL_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_UNEXP_CMPL_HANDLER);

    /* MPIDI_CS_ENTER(); */
    if (MPIDI_CH4U_REQUEST(rreq, req->status) & MPIDI_CH4U_REQ_UNEXP_DQUED) {
        if (MPIDI_CH4U_REQUEST(rreq, req->status) & MPIDI_CH4U_REQ_UNEXP_CLAIMED) {
            MPIDI_CH4U_unexp_mrecv_cmpl_handler(rreq);
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
        mpi_errno = MPIDI_CH4U_reply_ssend(rreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    dtype_release_if_not_builtin(MPIDI_CH4U_REQUEST(match_req, datatype));
    MPL_free(MPIDI_CH4U_REQUEST(rreq, buffer));
    MPIR_Object_release_ref(rreq, &c);
    MPIDI_CH4I_am_request_complete(rreq);
    MPIDI_CH4I_am_request_complete(match_req);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_UNEXP_CMPL_HANDLER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_recv_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_recv_cmpl_handler(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_RECV_CMPL_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_RECV_CMPL_HANDLER);

    if (!MPIDI_CH4U_check_cmpl_order(rreq, MPIDI_CH4U_recv_cmpl_handler))
        return mpi_errno;

    if (MPIDI_CH4U_REQUEST(rreq, req->status) & MPIDI_CH4U_REQ_RCV_NON_CONTIG) {
        MPL_free(MPIDI_CH4U_REQUEST(rreq, req->iov));
    }

    if (MPIDI_CH4U_REQUEST(rreq, req->status) & MPIDI_CH4U_REQ_UNEXPECTED) {
        mpi_errno = MPIDI_CH4U_unexp_cmpl_handler(rreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    rreq->status.MPI_SOURCE = MPIDI_CH4U_REQUEST(rreq, src_rank);
    rreq->status.MPI_TAG = MPIDI_CH4U_get_tag(MPIDI_CH4U_REQUEST(rreq, tag));

    if (MPIDI_CH4U_REQUEST(rreq, req->status) & MPIDI_CH4U_REQ_PEER_SSEND) {
        mpi_errno = MPIDI_CH4U_reply_ssend(rreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

#ifdef MPIDI_BUILD_CH4_SHM
    if (MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(rreq)) {
        int continue_matching = 1;
        MPIDI_CH4R_anysource_matched(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(rreq),
                                     MPIDI_CH4I_REQUEST(rreq,
                                                        is_local) ? MPIDI_SHM :
                                     MPIDI_NM, &continue_matching);
        if (unlikely(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(rreq))) {
            MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(rreq)) = NULL;
            MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(rreq) = NULL;
        }
    }
#endif

    dtype_release_if_not_builtin(MPIDI_CH4U_REQUEST(rreq, datatype));
    MPIDI_CH4I_am_request_complete(rreq);
  fn_exit:
    MPIDI_CH4U_progress_cmpl_list();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_RECV_CMPL_HANDLER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_get_acc_ack_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_get_acc_ack_cmpl_handler(MPIR_Request * areq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_GET_ACC_ACK_CMPL_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_GET_ACC_ACK_CMPL_HANDLER);

    if (!MPIDI_CH4U_check_cmpl_order(areq, MPIDI_CH4U_get_acc_ack_cmpl_handler))
        return mpi_errno;

    if (MPIDI_CH4U_REQUEST(areq, req->status) & MPIDI_CH4U_REQ_RCV_NON_CONTIG) {
        MPL_free(MPIDI_CH4U_REQUEST(areq, req->iov));
    }

    win = MPIDI_CH4U_REQUEST(areq, req->areq.win_ptr);
    /* MPIDI_CS_ENTER(); */
    OPA_decr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */

    dtype_release_if_not_builtin(MPIDI_CH4U_REQUEST(areq, req->areq.result_datatype));
    MPIDI_CH4I_am_request_complete(areq);

    MPIDI_CH4U_progress_cmpl_list();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_GET_ACC_ACK_CMPL_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_put_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_put_cmpl_handler(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_PUT_CMPL_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_PUT_CMPL_HANDLER);

    if (!MPIDI_CH4U_check_cmpl_order(rreq, MPIDI_CH4U_put_cmpl_handler))
        return mpi_errno;

    if (MPIDI_CH4U_REQUEST(rreq, req->status) & MPIDI_CH4U_REQ_RCV_NON_CONTIG) {
        MPL_free(MPIDI_CH4U_REQUEST(rreq, req->iov));
    }

    if (MPIDI_CH4U_REQUEST(rreq, req->preq.dt_iov)) {
        MPL_free(MPIDI_CH4U_REQUEST(rreq, req->preq.dt_iov));
    }

    mpi_errno = MPIDI_CH4U_ack_put(rreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    win = (MPIR_Win *) MPIDI_CH4U_REQUEST(rreq, req->preq.win_ptr);

    /* MPIDI_CS_ENTER(); */
    OPA_decr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */

    MPIDI_CH4I_am_request_complete(rreq);
    MPIDI_CH4U_progress_cmpl_list();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_PUT_CMPL_HANDLER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_put_iov_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_put_iov_cmpl_handler(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH4U_put_iov_ack_msg_t ack_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_PUT_IOV_CMPL_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_PUT_IOV_CMPL_HANDLER);

    ack_msg.src_rank = MPIDI_CH4U_REQUEST(rreq, src_rank);
    ack_msg.origin_preq_ptr = (uint64_t) MPIDI_CH4U_REQUEST(rreq, req->preq.preq_ptr);
    ack_msg.target_preq_ptr = (uint64_t) rreq;

#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    if (MPIDI_SHM == MPIDI_CH4U_REQUEST(rreq, caller)) {
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(MPIDI_CH4U_win_to_context
                                        (MPIDI_CH4U_REQUEST(rreq, req->preq.win_ptr)),
                                        MPIDI_CH4U_REQUEST(rreq, src_rank), MPIDI_CH4U_PUT_IOV_ACK,
                                        &ack_msg, sizeof(ack_msg));
    }
    else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDI_CH4U_win_to_context
                                       (MPIDI_CH4U_REQUEST(rreq, req->preq.win_ptr)),
                                       MPIDI_CH4U_REQUEST(rreq, src_rank), MPIDI_CH4U_PUT_IOV_ACK,
                                       &ack_msg, sizeof(ack_msg));
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_PUT_IOV_CMPL_HANDLER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_acc_iov_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_acc_iov_cmpl_handler(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH4U_acc_iov_ack_msg_t ack_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACC_IOV_CMPL_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACC_IOV_CMPL_HANDLER);

    ack_msg.origin_preq_ptr = (uint64_t) MPIDI_CH4U_REQUEST(rreq, req->areq.req_ptr);
    ack_msg.target_preq_ptr = (uint64_t) rreq;

#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    if (MPIDI_SHM == MPIDI_CH4U_REQUEST(rreq, caller)) {
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(MPIDI_CH4U_win_to_context
                                        (MPIDI_CH4U_REQUEST(rreq, req->areq.win_ptr)),
                                        MPIDI_CH4U_REQUEST(rreq, src_rank), MPIDI_CH4U_ACC_IOV_ACK,
                                        &ack_msg, sizeof(ack_msg));
    }
    else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDI_CH4U_win_to_context
                                       (MPIDI_CH4U_REQUEST(rreq, req->areq.win_ptr)),
                                       MPIDI_CH4U_REQUEST(rreq, src_rank), MPIDI_CH4U_ACC_IOV_ACK,
                                       &ack_msg, sizeof(ack_msg));
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACC_IOV_CMPL_HANDLER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_cswap_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_cswap_cmpl_handler(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    void *compare_addr;
    void *origin_addr;
    size_t data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_CSWAP_CMPL_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_CSWAP_CMPL_HANDLER);

    if (!MPIDI_CH4U_check_cmpl_order(rreq, MPIDI_CH4U_cswap_cmpl_handler))
        return mpi_errno;

    MPIDI_Datatype_check_size(MPIDI_CH4U_REQUEST(rreq, req->creq.datatype), 1, data_sz);
    origin_addr = MPIDI_CH4U_REQUEST(rreq, req->creq.data);
    compare_addr = ((char *) MPIDI_CH4U_REQUEST(rreq, req->creq.data)) + data_sz;

    /* MPIDI_CS_ENTER(); */

    if (MPIR_Compare_equal((void *) MPIDI_CH4U_REQUEST(rreq, req->creq.addr), compare_addr,
                           MPIDI_CH4U_REQUEST(rreq, req->creq.datatype))) {
        MPIR_Memcpy(compare_addr, (void *) MPIDI_CH4U_REQUEST(rreq, req->creq.addr), data_sz);
        MPIR_Memcpy((void *) MPIDI_CH4U_REQUEST(rreq, req->creq.addr), origin_addr, data_sz);
    }
    else {
        MPIR_Memcpy(compare_addr, (void *) MPIDI_CH4U_REQUEST(rreq, req->creq.addr), data_sz);
    }

    /* MPIDI_CS_EXIT(); */

    mpi_errno = MPIDI_CH4U_ack_cswap(rreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPIDI_CH4I_am_request_complete(rreq);
    MPIDI_CH4U_progress_cmpl_list();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_CSWAP_CMPL_HANDLER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_do_accumulate_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_do_accumulate_op(void *source_buf, int source_count,
                                              MPI_Datatype source_dtp, void *target_buf,
                                              int target_count, MPI_Datatype target_dtp,
                                              MPI_Aint stream_offset, MPI_Op acc_op)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_User_function *uop = NULL;
    MPI_Aint source_dtp_size = 0, source_dtp_extent = 0;
    int is_empty_source = FALSE;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_DO_ACCUMULATE_OP);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_DO_ACCUMULATE_OP);

    /* first Judge if source buffer is empty */
    if (acc_op == MPI_NO_OP)
        is_empty_source = TRUE;

    if (is_empty_source == FALSE) {
        MPIR_Assert(MPIR_DATATYPE_IS_PREDEFINED(source_dtp));
        MPID_Datatype_get_size_macro(source_dtp, source_dtp_size);
        MPID_Datatype_get_extent_macro(source_dtp, source_dtp_extent);
    }

    if (HANDLE_GET_KIND(acc_op) == HANDLE_KIND_BUILTIN) {
        /* get the function by indexing into the op table */
        uop = MPIR_OP_HDL_TO_FN(acc_op);
    }
    else {
        /* --BEGIN ERROR HANDLING-- */
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         FCNAME, __LINE__, MPI_ERR_OP,
                                         "**opnotpredefined", "**opnotpredefined %d", acc_op);
        return mpi_errno;
        /* --END ERROR HANDLING-- */
    }


    if (is_empty_source == TRUE || MPIR_DATATYPE_IS_PREDEFINED(target_dtp)) {
        /* directly apply op if target dtp is predefined dtp OR source buffer is empty */
        MPI_Aint real_stream_offset;
        void *curr_target_buf;

        if (is_empty_source == FALSE) {
            MPIR_Assert(source_dtp == target_dtp);
            real_stream_offset = (stream_offset / source_dtp_size) * source_dtp_extent;
            curr_target_buf = (void *) ((char *) target_buf + real_stream_offset);
        }
        else {
            curr_target_buf = target_buf;
        }

        (*uop) (source_buf, curr_target_buf, &source_count, &source_dtp);
    }
    else {
        /* derived datatype */
        MPID_Segment *segp;
        DLOOP_VECTOR *dloop_vec;
        MPI_Aint first, last;
        int vec_len, i, count;
        MPI_Aint type_extent, type_size;
        MPI_Datatype type;
        MPIR_Datatype *dtp;
        MPI_Aint curr_len;
        void *curr_loc;
        int accumulated_count;

        segp = MPIDU_Segment_alloc();
        /* --BEGIN ERROR HANDLING-- */
        if (!segp) {
            mpi_errno =
                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                     MPI_ERR_OTHER, "**nomem", 0);
            MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DO_ACCUMULATE_OP);
            return mpi_errno;
        }
        /* --END ERROR HANDLING-- */
        MPIDU_Segment_init(NULL, target_count, target_dtp, segp, 0);
        first = stream_offset;
        last = first + source_count * source_dtp_size;

        MPID_Datatype_get_ptr(target_dtp, dtp);
        vec_len = dtp->max_contig_blocks * target_count + 1;
        /* +1 needed because Rob says so */
        dloop_vec = (DLOOP_VECTOR *)
            MPL_malloc(vec_len * sizeof(DLOOP_VECTOR));
        /* --BEGIN ERROR HANDLING-- */
        if (!dloop_vec) {
            mpi_errno =
                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                     MPI_ERR_OTHER, "**nomem", 0);
            MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DO_ACCUMULATE_OP);
            return mpi_errno;
        }
        /* --END ERROR HANDLING-- */

        MPIDU_Segment_pack_vector(segp, first, &last, dloop_vec, &vec_len);

        type = dtp->basic_type;
        MPIR_Assert(type != MPI_DATATYPE_NULL);

        MPIR_Assert(type == source_dtp);
        type_size = source_dtp_size;
        type_extent = source_dtp_extent;

        i = 0;
        curr_loc = dloop_vec[0].DLOOP_VECTOR_BUF;
        curr_len = dloop_vec[0].DLOOP_VECTOR_LEN;
        accumulated_count = 0;
        while (i != vec_len) {
            if (curr_len < type_size) {
                MPIR_Assert(i != vec_len);
                i++;
                curr_len += dloop_vec[i].DLOOP_VECTOR_LEN;
                continue;
            }

            MPIR_Assign_trunc(count, curr_len / type_size, int);

            (*uop) ((char *) source_buf + type_extent * accumulated_count,
                    (char *) target_buf + MPIR_Ptr_to_aint(curr_loc), &count, &type);

            if (curr_len % type_size == 0) {
                i++;
                if (i != vec_len) {
                    curr_loc = dloop_vec[i].DLOOP_VECTOR_BUF;
                    curr_len = dloop_vec[i].DLOOP_VECTOR_LEN;
                }
            }
            else {
                curr_loc = (void *) ((char *) curr_loc + type_extent * count);
                curr_len -= type_size * count;
            }

            accumulated_count += count;
        }

        MPIDU_Segment_free(segp);
        MPL_free(dloop_vec);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DO_ACCUMULATE_OP);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_handle_acc_cmpl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_handle_acc_cmpl(MPIR_Request * rreq, int do_get)
{
    int mpi_errno = MPI_SUCCESS, i;
    MPI_Aint basic_sz, count, offset = 0;
    struct iovec *iov;
    char *src_ptr, *original = NULL;
    size_t data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_HANDLE_ACC_CMPL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_HANDLE_ACC_CMPL);

    MPID_Datatype_get_size_macro(MPIDI_CH4U_REQUEST(rreq, req->areq.target_datatype), basic_sz);
    data_sz = MPIDI_CH4U_REQUEST(rreq, req->areq.data_sz);

    /* MPIDI_CS_ENTER(); */

    if (do_get) {
        original = (char *) MPL_malloc(data_sz);
        MPIR_Assert(original);
    }

    if (MPIDI_CH4U_REQUEST(rreq, req->areq.op) == MPI_NO_OP) {
        MPIDI_CH4U_REQUEST(rreq, req->areq.origin_count) =
            MPIDI_CH4U_REQUEST(rreq, req->areq.target_count);
        MPIDI_CH4U_REQUEST(rreq, req->areq.data_sz) = data_sz;
    }

    if (MPIDI_CH4U_REQUEST(rreq, req->areq.dt_iov) == NULL) {

        if (original) {
            MPIR_Memcpy(original, MPIDI_CH4U_REQUEST(rreq, req->areq.target_addr),
                        basic_sz * MPIDI_CH4U_REQUEST(rreq, req->areq.target_count));
        }

        mpi_errno = MPIDI_CH4U_do_accumulate_op(MPIDI_CH4U_REQUEST(rreq, req->areq.data),
                                                MPIDI_CH4U_REQUEST(rreq, req->areq.origin_count),
                                                MPIDI_CH4U_REQUEST(rreq, req->areq.origin_datatype),
                                                MPIDI_CH4U_REQUEST(rreq, req->areq.target_addr),
                                                MPIDI_CH4U_REQUEST(rreq, req->areq.target_count),
                                                MPIDI_CH4U_REQUEST(rreq, req->areq.target_datatype),
                                                0, MPIDI_CH4U_REQUEST(rreq, req->areq.op));
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    else {
        iov = (struct iovec *) MPIDI_CH4U_REQUEST(rreq, req->areq.dt_iov);
        src_ptr = (char *) MPIDI_CH4U_REQUEST(rreq, req->areq.data);
        for (i = 0; i < MPIDI_CH4U_REQUEST(rreq, req->areq.n_iov); i++) {
            count = iov[i].iov_len / basic_sz;
            MPIR_Assert(count > 0);

            if (original) {
                MPIR_Memcpy(original + offset, iov[i].iov_base, count * basic_sz);
                offset += count * basic_sz;
            }

            mpi_errno = MPIDI_CH4U_do_accumulate_op(src_ptr, count,
                                                    MPIDI_CH4U_REQUEST(rreq,
                                                                       req->areq.origin_datatype),
                                                    iov[i].iov_base, count, MPIDI_CH4U_REQUEST(rreq,
                                                                                               req->
                                                                                               areq.
                                                                                               target_datatype),
                                                    0, MPIDI_CH4U_REQUEST(rreq, req->areq.op));
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            src_ptr += count * basic_sz;
        }
        MPL_free(iov);
    }

    /* MPIDI_CS_EXIT(); */
    if (MPIDI_CH4U_REQUEST(rreq, req->areq.data))
        MPL_free(MPIDI_CH4U_REQUEST(rreq, req->areq.data));

    if (original) {
        MPIDI_CH4U_REQUEST(rreq, req->areq.data) = original;
        mpi_errno = MPIDI_CH4U_ack_get_acc(rreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    else {
        MPIDI_CH4U_REQUEST(rreq, req->areq.data) = NULL;
        mpi_errno = MPIDI_CH4U_ack_acc(rreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    MPIDI_CH4I_am_request_complete(rreq);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_HANDLE_ACC_CMPL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_acc_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_acc_cmpl_handler(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACC_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACC_HANDLER);

    if (!MPIDI_CH4U_check_cmpl_order(rreq, MPIDI_CH4U_acc_cmpl_handler))
        return mpi_errno;

    mpi_errno = MPIDI_CH4U_handle_acc_cmpl(rreq, 0);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIDI_CH4U_progress_cmpl_list();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACC_HANDLER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_get_acc_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_get_acc_cmpl_handler(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_GET_ACC_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_GET_ACC_HANDLER);

    if (!MPIDI_CH4U_check_cmpl_order(rreq, MPIDI_CH4U_get_acc_cmpl_handler))
        return mpi_errno;

    mpi_errno = MPIDI_CH4U_handle_acc_cmpl(rreq, 1);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIDI_CH4U_progress_cmpl_list();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_GET_ACC_HANDLER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_get_ack_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_get_ack_cmpl_handler(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *greq;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_GET_ACK_CMPL_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_GET_ACK_CMPL_HANDLER);

    if (!MPIDI_CH4U_check_cmpl_order(rreq, MPIDI_CH4U_get_ack_cmpl_handler))
        return mpi_errno;

    greq = (MPIR_Request *) MPIDI_CH4U_REQUEST(rreq, req->greq.greq_ptr);
    if (MPIDI_CH4U_REQUEST(greq, req->status) & MPIDI_CH4U_REQ_RCV_NON_CONTIG) {
        MPL_free(MPIDI_CH4U_REQUEST(greq, req->iov));
    }

    win = MPIDI_CH4U_REQUEST(greq, req->greq.win_ptr);
    /* MPIDI_CS_ENTER(); */
    OPA_decr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */

    MPIDI_CH4I_am_request_complete(greq);
    MPIDI_CH4I_am_request_complete(rreq);
    MPIDI_CH4U_progress_cmpl_list();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_GET_ACK_CMPL_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_cswap_ack_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_cswap_ack_cmpl_handler(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_CSWAP_ACK_CMPL_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_CSWAP_ACK_CMPL_HANDLER);

    if (!MPIDI_CH4U_check_cmpl_order(rreq, MPIDI_CH4U_cswap_ack_cmpl_handler))
        return mpi_errno;

    win = MPIDI_CH4U_REQUEST(rreq, req->creq.win_ptr);
    /* MPIDI_CS_ENTER(); */
    OPA_decr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */

    MPL_free(MPIDI_CH4U_REQUEST(rreq, req->creq.data));
    MPIDI_CH4I_am_request_complete(rreq);

    MPIDI_CH4U_progress_cmpl_list();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_CSWAP_ACK_CMPL_HANDLER);
    return mpi_errno;
}

/* Main body of the send target handler, commonly used by both eager send and rendezvous send */
static inline int MPIDI_CH4I_do_send_target_handler(void **data,
                                                    size_t * p_data_sz,
                                                    int *is_contig,
                                                    MPIDI_NM_am_completion_handler_fn *
                                                    cmpl_handler_fn,
                                                    MPIR_Request * rreq, MPIDI_call_context caller)
{
    int dt_contig, n_iov;
    MPI_Aint dt_true_lb, last, num_iov;
    MPIR_Datatype *dt_ptr;
    MPID_Segment *segment_ptr;
    size_t data_sz;

    *cmpl_handler_fn = MPIDI_CH4U_recv_cmpl_handler;
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

    return MPI_SUCCESS;
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
                                                 cmpl_handler_fn,
                                                 MPIR_Request ** req, MPIDI_call_context caller)
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
        rreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__RECV, caller);
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

    mpi_errno = MPIDI_CH4I_do_send_target_handler(data, p_data_sz, is_contig,
                                                  cmpl_handler_fn, rreq, caller);

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
                                                          cmpl_handler_fn,
                                                          MPIR_Request ** req,
                                                          MPIDI_call_context caller)
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
        rreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__RECV, caller);

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
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
        if (MPIDI_SHM == MPIDI_CH4U_REQUEST(rreq, caller)) {
            mpi_errno = MPIDI_SHM_am_recv(rreq);
        }
        else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
            mpi_errno = MPIDI_NM_am_recv(rreq);
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
        }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
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
                                                          cmpl_handler_fn,
                                                          MPIR_Request ** req,
                                                          MPIDI_call_context caller)
{
    int mpi_errno;
    MPIR_Request *rreq;
    MPIDI_CH4U_send_long_lmt_msg_t *lmt_hdr = (MPIDI_CH4U_send_long_lmt_msg_t *) am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_SEND_LONG_LMT_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_SEND_LONG_LMT_HANDLER);

    rreq = (MPIR_Request *) lmt_hdr->rreq_ptr;
    MPIR_Assert(rreq);
    mpi_errno = MPIDI_CH4I_do_send_target_handler(data, p_data_sz, is_contig,
                                                  cmpl_handler_fn, rreq, caller);
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
                                                  cmpl_handler_fn,
                                                  MPIR_Request ** req, MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_CH4U_ssend_req_msg_t *msg_hdr = (MPIDI_CH4U_ssend_req_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_SSEND_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_SSEND_HANDLER);

    mpi_errno = MPIDI_CH4U_send_target_handler(handler_id, am_hdr,
                                               data, p_data_sz, is_contig, cmpl_handler_fn,
                                               req, caller);
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
                                                      cmpl_handler_fn,
                                                      MPIR_Request ** req,
                                                      MPIDI_call_context caller)
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
                                                          cmpl_handler_fn,
                                                          MPIR_Request ** req,
                                                          MPIDI_call_context caller)
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
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    if (MPIDI_SHM == caller) {
        mpi_errno =
            MPIDI_SHM_am_isend_reply(MPIDI_CH4U_get_context
                                     (MPIDI_CH4U_REQUEST(sreq, req->lreq).msg_tag),
                                     MPIDI_CH4U_REQUEST(sreq, src_rank), MPIDI_CH4U_SEND_LONG_LMT,
                                     &send_hdr, sizeof(send_hdr), MPIDI_CH4U_REQUEST(sreq,
                                                                                     req->lreq).
                                     src_buf, MPIDI_CH4U_REQUEST(sreq, req->lreq).count,
                                     MPIDI_CH4U_REQUEST(sreq, req->lreq).datatype, sreq);
    }
    else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
        mpi_errno =
            MPIDI_NM_am_isend_reply(MPIDI_CH4U_get_context
                                    (MPIDI_CH4U_REQUEST(sreq, req->lreq).msg_tag),
                                    MPIDI_CH4U_REQUEST(sreq, src_rank), MPIDI_CH4U_SEND_LONG_LMT,
                                    &send_hdr, sizeof(send_hdr), MPIDI_CH4U_REQUEST(sreq,
                                                                                    req->lreq).
                                    src_buf, MPIDI_CH4U_REQUEST(sreq, req->lreq).count,
                                    MPIDI_CH4U_REQUEST(sreq, req->lreq).datatype, sreq);
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
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

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_put_ack_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_put_ack_target_handler(int handler_id, void *am_hdr,
                                                    void **data,
                                                    size_t * p_data_sz, int *is_contig,
                                                    MPIDI_NM_am_completion_handler_fn *
                                                    cmpl_handler_fn,
                                                    MPIR_Request ** req, MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH4U_put_ack_msg_t *msg_hdr = (MPIDI_CH4U_put_ack_msg_t *) am_hdr;
    MPIR_Win *win;
    MPIR_Request *preq;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_PUT_ACK_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_PUT_ACK_HANDLER);

    preq = (MPIR_Request *) msg_hdr->preq_ptr;
    win = MPIDI_CH4U_REQUEST(preq, req->preq.win_ptr);

    if (MPIDI_CH4U_REQUEST(preq, req->preq.dt_iov)) {
        MPL_free(MPIDI_CH4U_REQUEST(preq, req->preq.dt_iov));
    }

    MPIDI_CH4I_am_request_complete(preq);

    /* MPIDI_CS_ENTER(); */
    OPA_decr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */

    if (req)
        *req = NULL;
    if (cmpl_handler_fn)
        *cmpl_handler_fn = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_PUT_ACK_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_acc_ack_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_acc_ack_target_handler(int handler_id, void *am_hdr,
                                                    void **data,
                                                    size_t * p_data_sz, int *is_contig,
                                                    MPIDI_NM_am_completion_handler_fn *
                                                    cmpl_handler_fn,
                                                    MPIR_Request ** req, MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH4U_acc_ack_msg_t *msg_hdr = (MPIDI_CH4U_acc_ack_msg_t *) am_hdr;
    MPIR_Win *win;
    MPIR_Request *areq;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACC_ACK_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACC_ACK_HANDLER);

    areq = (MPIR_Request *) msg_hdr->req_ptr;
    win = MPIDI_CH4U_REQUEST(areq, req->areq.win_ptr);

    if (MPIDI_CH4U_REQUEST(areq, req->areq.dt_iov)) {
        MPL_free(MPIDI_CH4U_REQUEST(areq, req->areq.dt_iov));
    }

    MPIDI_CH4I_am_request_complete(areq);

    /* MPIDI_CS_ENTER(); */
    OPA_decr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */

    if (req)
        *req = NULL;
    if (cmpl_handler_fn)
        *cmpl_handler_fn = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACC_ACK_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_get_acc_ack_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_get_acc_ack_target_handler(int handler_id, void *am_hdr,
                                                        void **data,
                                                        size_t * p_data_sz, int *is_contig,
                                                        MPIDI_NM_am_completion_handler_fn *
                                                        cmpl_handler_fn,
                                                        MPIR_Request ** req,
                                                        MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH4U_acc_ack_msg_t *msg_hdr = (MPIDI_CH4U_acc_ack_msg_t *) am_hdr;
    MPIR_Request *areq;

    size_t data_sz;
    int dt_contig, n_iov;
    MPI_Aint dt_true_lb, last, num_iov;
    MPIR_Datatype *dt_ptr;
    MPID_Segment *segment_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACC_ACK_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACC_ACK_HANDLER);

    areq = (MPIR_Request *) msg_hdr->req_ptr;

    if (MPIDI_CH4U_REQUEST(areq, req->areq.dt_iov)) {
        MPL_free(MPIDI_CH4U_REQUEST(areq, req->areq.dt_iov));
    }

    MPIDI_Datatype_get_info(MPIDI_CH4U_REQUEST(areq, req->areq.result_count),
                            MPIDI_CH4U_REQUEST(areq, req->areq.result_datatype),
                            dt_contig, data_sz, dt_ptr, dt_true_lb);
    *is_contig = dt_contig;

    if (dt_contig) {
        *p_data_sz = data_sz;
        *data = (char *) MPIDI_CH4U_REQUEST(areq, req->areq.result_addr) + dt_true_lb;
    }
    else {
        segment_ptr = MPIDU_Segment_alloc();
        MPIR_Assert(segment_ptr);

        MPIDU_Segment_init(MPIDI_CH4U_REQUEST(areq, req->areq.result_addr),
                           MPIDI_CH4U_REQUEST(areq, req->areq.result_count),
                           MPIDI_CH4U_REQUEST(areq, req->areq.result_datatype), segment_ptr, 0);

        last = data_sz;
        MPIDU_Segment_count_contig_blocks(segment_ptr, 0, &last, &num_iov);
        n_iov = (int) num_iov;
        MPIR_Assert(n_iov > 0);
        MPIDI_CH4U_REQUEST(areq, req->iov) =
            (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec));
        MPIR_Assert(MPIDI_CH4U_REQUEST(areq, req->iov));

        last = data_sz;
        MPIDU_Segment_pack_vector(segment_ptr, 0, &last, MPIDI_CH4U_REQUEST(areq, req->iov),
                                  &n_iov);
        MPIR_Assert(last == (MPI_Aint) data_sz);
        *data = MPIDI_CH4U_REQUEST(areq, req->iov);
        *p_data_sz = n_iov;
        MPIDI_CH4U_REQUEST(areq, req->status) |= MPIDI_CH4U_REQ_RCV_NON_CONTIG;
        MPL_free(segment_ptr);
    }

    *req = areq;
    *cmpl_handler_fn = MPIDI_CH4U_get_acc_ack_cmpl_handler;
    MPIDI_CH4U_REQUEST(areq, req->seq_no) = OPA_fetch_and_add_int(&MPIDI_CH4_Global.nxt_seq_no, 1);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACC_ACK_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_cswap_ack_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_cswap_ack_target_handler(int handler_id, void *am_hdr,
                                                      void **data,
                                                      size_t * p_data_sz, int *is_contig,
                                                      MPIDI_NM_am_completion_handler_fn *
                                                      cmpl_handler_fn,
                                                      MPIR_Request ** req,
                                                      MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH4U_cswap_ack_msg_t *msg_hdr = (MPIDI_CH4U_cswap_ack_msg_t *) am_hdr;
    MPIR_Request *creq;
    uint64_t data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_CSWAP_ACK_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_CSWAP_ACK_HANDLER);

    creq = (MPIR_Request *) msg_hdr->req_ptr;
    MPIDI_Datatype_check_size(MPIDI_CH4U_REQUEST(creq, req->creq.datatype), 1, data_sz);
    *data = MPIDI_CH4U_REQUEST(creq, req->creq.result_addr);
    *p_data_sz = data_sz;
    *is_contig = 1;

    *req = creq;
    *cmpl_handler_fn = MPIDI_CH4U_cswap_ack_cmpl_handler;
    MPIDI_CH4U_REQUEST(creq, req->seq_no) = OPA_fetch_and_add_int(&MPIDI_CH4_Global.nxt_seq_no, 1);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_CSWAP_ACK_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_win_lock_advance
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_win_lock_advance(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDI_CH4U_win_sync_lock *slock = &MPIDI_CH4U_WIN(win, sync).lock;
    struct MPIDI_CH4U_win_queue *q = &slock->local.requested;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_WIN_LOCK_ADVANCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_WIN_LOCK_ADVANCE);

    if ((q->head != NULL) && ((slock->local.count == 0) ||
                              ((slock->local.type == MPI_LOCK_SHARED) &&
                               (q->head->type == MPI_LOCK_SHARED)))) {
        struct MPIDI_CH4U_win_lock *lock = q->head;
        q->head = lock->next;

        if (q->head == NULL)
            q->tail = NULL;

        ++slock->local.count;
        slock->local.type = lock->type;

        MPIDI_CH4U_win_cntrl_msg_t msg;
        int handler_id;
        msg.win_id = MPIDI_CH4U_WIN(win, win_id);
        msg.origin_rank = win->comm_ptr->rank;

        if (lock->mtype == MPIDI_CH4U_WIN_LOCK)
            handler_id = MPIDI_CH4U_WIN_LOCK_ACK;
        else if (lock->mtype == MPIDI_CH4U_WIN_LOCKALL)
            handler_id = MPIDI_CH4U_WIN_LOCKALL_ACK;
        else
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**rmasync");

#ifdef MPIDI_CH4_EXCLUSIVE_SHM
        int r;
        if ((r = MPIDI_CH4_rank_is_local(lock->rank, win->comm_ptr))) {
            mpi_errno = MPIDI_SHM_am_send_hdr_reply(MPIDI_CH4U_win_to_context(win),
                                                    lock->rank, handler_id, &msg, sizeof(msg));
        }
        else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
            mpi_errno = MPIDI_NM_am_send_hdr_reply(MPIDI_CH4U_win_to_context(win),
                                                   lock->rank, handler_id, &msg, sizeof(msg));
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
        }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPL_free(lock);

        mpi_errno = MPIDI_CH4U_win_lock_advance(win);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_WIN_LOCK_ADVANCE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_win_lock_req_proc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_CH4U_win_lock_req_proc(int handler_id,
                                                const MPIDI_CH4U_win_cntrl_msg_t * info,
                                                MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_WIN_LOCK_REQ_PROC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_WIN_LOCK_REQ_PROC);

    struct MPIDI_CH4U_win_lock *lock = (struct MPIDI_CH4U_win_lock *)
        MPL_calloc(1, sizeof(struct MPIDI_CH4U_win_lock));

    lock->mtype = handler_id;
    lock->rank = info->origin_rank;
    lock->type = info->lock_type;
    struct MPIDI_CH4U_win_queue *q = &MPIDI_CH4U_WIN(win, sync).lock.local.requested;
    MPIR_Assert((q->head != NULL) ^ (q->tail == NULL));

    if (q->tail == NULL)
        q->head = lock;
    else
        q->tail->next = lock;

    q->tail = lock;

    MPIDI_CH4U_win_lock_advance(win);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_WIN_LOCK_REQ_PROC);
    return;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_win_lock_ack_proc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_CH4U_win_lock_ack_proc(int handler_id,
                                                const MPIDI_CH4U_win_cntrl_msg_t * info,
                                                MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_WIN_LOCK_ACK_PROC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_WIN_LOCK_ACK_PROC);

    if (handler_id == MPIDI_CH4U_WIN_LOCK_ACK)
        MPIDI_CH4U_WIN(win, sync).lock.remote.locked += 1;
    else if (handler_id == MPIDI_CH4U_WIN_LOCKALL_ACK)
        MPIDI_CH4U_WIN(win, sync).lock.remote.allLocked += 1;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_WIN_LOCK_ACK_PROC);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_win_unlock_ack_proc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_CH4U_win_unlock_proc(const MPIDI_CH4U_win_cntrl_msg_t * info,
                                              MPIR_Win * win)
{

    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_WIN_UNLOCK_ACK_PROC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_WIN_UNLOCK_ACK_PROC);

    --MPIDI_CH4U_WIN(win, sync).lock.local.count;
    MPIR_Assert((int) MPIDI_CH4U_WIN(win, sync).lock.local.count >= 0);
    MPIDI_CH4U_win_lock_advance(win);

    MPIDI_CH4U_win_cntrl_msg_t msg;
    msg.win_id = MPIDI_CH4U_WIN(win, win_id);
    msg.origin_rank = win->comm_ptr->rank;

#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    int r;
    if ((r = MPIDI_CH4_rank_is_local(info->origin_rank, win->comm_ptr))) {
        mpi_errno = MPIDI_SHM_am_send_hdr_reply(MPIDI_CH4U_win_to_context(win),
                                                info->origin_rank,
                                                MPIDI_CH4U_WIN_UNLOCK_ACK, &msg, sizeof(msg));
    }
    else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
        mpi_errno = MPIDI_NM_am_send_hdr_reply(MPIDI_CH4U_win_to_context(win),
                                               info->origin_rank,
                                               MPIDI_CH4U_WIN_UNLOCK_ACK, &msg, sizeof(msg));
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_WIN_UNLOCK_ACK_PROC);
    return;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_win_complete_proc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_CH4U_win_complete_proc(const MPIDI_CH4U_win_cntrl_msg_t * info,
                                                MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_WIN_COMPLETE_PROC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_WIN_COMPLETE_PROC);

    ++MPIDI_CH4U_WIN(win, sync).sc.count;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_WIN_COMPLETE_PROC);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_win_post_proc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_CH4U_win_post_proc(const MPIDI_CH4U_win_cntrl_msg_t * info, MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_WIN_POST_PROC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_WIN_POST_PROC);

    ++MPIDI_CH4U_WIN(win, sync).pw.count;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_WIN_POST_PROC);
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_win_unlock_done_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_CH4U_win_unlock_done_cb(const MPIDI_CH4U_win_cntrl_msg_t * info,
                                                 MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_WIN_UNLOCK_DONE_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_WIN_UNLOCK_DONE_CB);

    if (MPIDI_CH4U_WIN(win, sync).origin_epoch_type == MPIDI_CH4U_EPOTYPE_LOCK) {
        MPIDI_CH4U_WIN(win, sync).lock.remote.locked--;
    }
    else if (MPIDI_CH4U_WIN(win, sync).origin_epoch_type == MPIDI_CH4U_EPOTYPE_LOCK_ALL) {
        MPIR_Assert((int) MPIDI_CH4U_WIN(win, sync).lock.remote.allLocked > 0);
        MPIDI_CH4U_WIN(win, sync).lock.remote.allLocked -= 1;
    }
    else {
        MPIR_Assert(0);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_WIN_UNLOCK_DONE_CB);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_win_ctrl_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_win_ctrl_target_handler(int handler_id, void *am_hdr,
                                                     void **data,
                                                     size_t * p_data_sz, int *is_contig,
                                                     MPIDI_NM_am_completion_handler_fn *
                                                     cmpl_handler_fn,
                                                     MPIR_Request ** req, MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH4U_win_cntrl_msg_t *msg_hdr = (MPIDI_CH4U_win_cntrl_msg_t *) am_hdr;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_WIN_CTRL_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_WIN_CTRL_HANDLER);

    MPL_HASH_FIND(dev.ch4u.hash_handle, MPIDI_CH4_Global.win_hash,
                  &msg_hdr->win_id, sizeof(uint64_t), win);
    /* TODO: check output win ptr */

    switch (handler_id) {
        char buff[32];

    case MPIDI_CH4U_WIN_LOCK:
    case MPIDI_CH4U_WIN_LOCKALL:
        MPIDI_CH4U_win_lock_req_proc(handler_id, msg_hdr, win);
        break;

    case MPIDI_CH4U_WIN_LOCK_ACK:
    case MPIDI_CH4U_WIN_LOCKALL_ACK:
        MPIDI_CH4U_win_lock_ack_proc(handler_id, msg_hdr, win);
        break;

    case MPIDI_CH4U_WIN_UNLOCK:
    case MPIDI_CH4U_WIN_UNLOCKALL:
        MPIDI_CH4U_win_unlock_proc(msg_hdr, win);
        break;

    case MPIDI_CH4U_WIN_UNLOCK_ACK:
    case MPIDI_CH4U_WIN_UNLOCKALL_ACK:
        MPIDI_CH4U_win_unlock_done_cb(msg_hdr, win);
        break;

    case MPIDI_CH4U_WIN_COMPLETE:
        MPIDI_CH4U_win_complete_proc(msg_hdr, win);
        break;

    case MPIDI_CH4U_WIN_POST:
        MPIDI_CH4U_win_post_proc(msg_hdr, win);
        break;

    default:
        MPL_snprintf(buff, sizeof(buff), "Invalid message type: %d\n", handler_id);
        MPID_Abort(NULL, MPI_ERR_INTERN, 1, buff);
    }

    if (req)
        *req = NULL;
    if (cmpl_handler_fn)
        *cmpl_handler_fn = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_WIN_CTRL_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_put_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_put_target_handler(int handler_id, void *am_hdr,
                                                void **data,
                                                size_t * p_data_sz,
                                                int *is_contig,
                                                MPIDI_NM_am_completion_handler_fn *
                                                cmpl_handler_fn,
                                                MPIR_Request ** req, MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    size_t data_sz;
    struct iovec *iov, *dt_iov;
    uintptr_t base;             /* Base address of the window */
    size_t offset;

    int dt_contig, n_iov;
    MPI_Aint dt_true_lb, last, num_iov;
    MPIR_Datatype *dt_ptr;
    MPID_Segment *segment_ptr;
    MPIR_Win *win;
    MPIDI_CH4U_put_msg_t *msg_hdr = (MPIDI_CH4U_put_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_PUT_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_PUT_HANDLER);

    rreq = MPIDI_CH4I_am_win_request_create();
    MPIR_Assert(rreq);
    rreq->kind = MPIR_REQUEST_KIND__RMA;
    *req = rreq;

    MPIDI_CH4U_REQUEST(*req, req->preq.preq_ptr) = msg_hdr->preq_ptr;
    MPIDI_CH4U_REQUEST(*req, src_rank) = msg_hdr->src_rank;

    MPL_HASH_FIND(dev.ch4u.hash_handle, MPIDI_CH4_Global.win_hash,
                  &msg_hdr->win_id, sizeof(uint64_t), win);
    MPIR_Assert(win);

    base = MPIDI_CH4I_win_base_at_target(win);

    /* MPIDI_CS_ENTER(); */
    OPA_incr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */
    MPIDI_CH4U_REQUEST(rreq, req->preq.win_ptr) = win;

    *cmpl_handler_fn = MPIDI_CH4U_put_cmpl_handler;
    MPIDI_CH4U_REQUEST(rreq, req->seq_no) = OPA_fetch_and_add_int(&MPIDI_CH4_Global.nxt_seq_no, 1);

    offset = win->disp_unit * msg_hdr->target_disp;
    if (msg_hdr->n_iov) {
        int i;
        dt_iov = (struct iovec *) MPL_malloc(sizeof(struct iovec) * msg_hdr->n_iov);
        MPIR_Assert(dt_iov);

        iov = (struct iovec *) ((char *) am_hdr + sizeof(*msg_hdr));
        for (i = 0; i < msg_hdr->n_iov; i++)
            iov[i].iov_base = (char *) iov[i].iov_base + base + offset;
        MPIR_Memcpy(dt_iov, iov, sizeof(struct iovec) * msg_hdr->n_iov);
        MPIDI_CH4U_REQUEST(rreq, req->preq.dt_iov) = dt_iov;
        MPIDI_CH4U_REQUEST(rreq, req->preq.n_iov) = msg_hdr->n_iov;
        *is_contig = 0;
        *data = iov;
        *p_data_sz = msg_hdr->n_iov;
        goto fn_exit;
    }

    MPIDI_CH4U_REQUEST(rreq, req->preq.dt_iov) = NULL;
    MPIDI_Datatype_get_info(msg_hdr->count, msg_hdr->datatype,
                            dt_contig, data_sz, dt_ptr, dt_true_lb);
    *is_contig = dt_contig;

    if (dt_contig) {
        *p_data_sz = data_sz;
        *data = (char *) (offset + base + dt_true_lb);
    }
    else {
        segment_ptr = MPIDU_Segment_alloc();
        MPIR_Assert(segment_ptr);

        MPIDU_Segment_init((void *) (offset + base), msg_hdr->count, msg_hdr->datatype,
                           segment_ptr, 0);
        last = data_sz;
        MPIDU_Segment_count_contig_blocks(segment_ptr, 0, &last, &num_iov);
        n_iov = (int) num_iov;
        MPIR_Assert(n_iov > 0);
        MPIDI_CH4U_REQUEST(rreq, req->iov) =
            (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec));
        MPIR_Assert(MPIDI_CH4U_REQUEST(rreq, req->iov));

        last = data_sz;
        MPIDU_Segment_pack_vector(segment_ptr, 0, &last, MPIDI_CH4U_REQUEST(rreq, req->iov),
                                  &n_iov);
        MPIR_Assert(last == (MPI_Aint) data_sz);
        *data = MPIDI_CH4U_REQUEST(rreq, req->iov);
        *p_data_sz = n_iov;
        MPIDI_CH4U_REQUEST(rreq, req->status) |= MPIDI_CH4U_REQ_RCV_NON_CONTIG;
        MPL_free(segment_ptr);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_PUT_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_put_iov_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_put_iov_target_handler(int handler_id, void *am_hdr,
                                                    void **data,
                                                    size_t * p_data_sz,
                                                    int *is_contig,
                                                    MPIDI_NM_am_completion_handler_fn *
                                                    cmpl_handler_fn,
                                                    MPIR_Request ** req, MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    struct iovec *dt_iov;

    MPIR_Win *win;
    MPIDI_CH4U_put_msg_t *msg_hdr = (MPIDI_CH4U_put_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_PUT_IOV_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_PUT_IOV_HANDLER);

    rreq = MPIDI_CH4I_am_win_request_create();
    MPIR_Assert(rreq);
    rreq->kind = MPIR_REQUEST_KIND__RMA;
    *req = rreq;

    MPIDI_CH4U_REQUEST(*req, req->preq.preq_ptr) = msg_hdr->preq_ptr;
    MPIDI_CH4U_REQUEST(*req, src_rank) = msg_hdr->src_rank;

    MPL_HASH_FIND(dev.ch4u.hash_handle, MPIDI_CH4_Global.win_hash,
                  &msg_hdr->win_id, sizeof(uint64_t), win);
    MPIR_Assert(win);

    /* MPIDI_CS_ENTER(); */
    OPA_incr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */
    MPIDI_CH4U_REQUEST(rreq, req->preq.win_ptr) = win;

    *cmpl_handler_fn = MPIDI_CH4U_put_iov_cmpl_handler;
    MPIDI_CH4U_REQUEST(rreq, req->seq_no) = OPA_fetch_and_add_int(&MPIDI_CH4_Global.nxt_seq_no, 1);

    /* Base adjustment for iov will be done after we get the entire iovs,
     * at MPIDI_CH4U_put_data_target_handler */
    MPIR_Assert(msg_hdr->n_iov);
    dt_iov = (struct iovec *) MPL_malloc(sizeof(struct iovec) * msg_hdr->n_iov);
    MPIR_Assert(dt_iov);

    MPIDI_CH4U_REQUEST(rreq, req->preq.dt_iov) = dt_iov;
    MPIDI_CH4U_REQUEST(rreq, req->preq.n_iov) = msg_hdr->n_iov;
    *is_contig = 1;
    *data = dt_iov;
    *p_data_sz = msg_hdr->n_iov * sizeof(struct iovec);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_PUT_IOV_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_put_iov_ack_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_put_iov_ack_target_handler(int handler_id, void *am_hdr,
                                                        void **data,
                                                        size_t * p_data_sz,
                                                        int *is_contig,
                                                        MPIDI_NM_am_completion_handler_fn *
                                                        cmpl_handler_fn,
                                                        MPIR_Request ** req,
                                                        MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq, *origin_req;
    MPIDI_CH4U_put_iov_ack_msg_t *msg_hdr = (MPIDI_CH4U_put_iov_ack_msg_t *) am_hdr;
    MPIDI_CH4U_put_dat_msg_t dat_msg;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_PUT_IOV_ACK_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_PUT_IOV_ACK_HANDLER);

    rreq = MPIDI_CH4I_am_win_request_create();
    MPIR_Assert(rreq);
    rreq->kind = MPIR_REQUEST_KIND__RMA;

    origin_req = (MPIR_Request *) msg_hdr->origin_preq_ptr;
    dat_msg.preq_ptr = msg_hdr->target_preq_ptr;
    win = MPIDI_CH4U_REQUEST(origin_req, req->preq.win_ptr);
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    if (MPIDI_SHM == caller) {
        mpi_errno = MPIDI_SHM_am_isend_reply(MPIDI_CH4U_win_to_context(win),
                                             MPIDI_CH4U_REQUEST(origin_req, src_rank),
                                             MPIDI_CH4U_PUT_DAT_REQ,
                                             &dat_msg, sizeof(dat_msg),
                                             MPIDI_CH4U_REQUEST(origin_req, req->preq.origin_addr),
                                             MPIDI_CH4U_REQUEST(origin_req, req->preq.origin_count),
                                             MPIDI_CH4U_REQUEST(origin_req,
                                                                req->preq.origin_datatype), rreq);
    }
    else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
        mpi_errno = MPIDI_NM_am_isend_reply(MPIDI_CH4U_win_to_context(win),
                                            MPIDI_CH4U_REQUEST(origin_req, src_rank),
                                            MPIDI_CH4U_PUT_DAT_REQ,
                                            &dat_msg, sizeof(dat_msg),
                                            MPIDI_CH4U_REQUEST(origin_req, req->preq.origin_addr),
                                            MPIDI_CH4U_REQUEST(origin_req, req->preq.origin_count),
                                            MPIDI_CH4U_REQUEST(origin_req,
                                                               req->preq.origin_datatype), rreq);
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    dtype_release_if_not_builtin(MPIDI_CH4U_REQUEST(origin_req, req->preq.origin_datatype));

    *cmpl_handler_fn = NULL;
    *req = NULL;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_PUT_IOV_ACK_HANDLER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_acc_iov_ack_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_acc_iov_ack_target_handler(int handler_id, void *am_hdr,
                                                        void **data,
                                                        size_t * p_data_sz,
                                                        int *is_contig,
                                                        MPIDI_NM_am_completion_handler_fn *
                                                        cmpl_handler_fn,
                                                        MPIR_Request ** req,
                                                        MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq, *origin_req;
    MPIDI_CH4U_acc_iov_ack_msg_t *msg_hdr = (MPIDI_CH4U_acc_iov_ack_msg_t *) am_hdr;
    MPIDI_CH4U_acc_dat_msg_t dat_msg;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACC_IOV_ACK_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACC_IOV_ACK_HANDLER);

    rreq = MPIDI_CH4I_am_win_request_create();
    MPIR_Assert(rreq);
    rreq->kind = MPIR_REQUEST_KIND__RMA;

    origin_req = (MPIR_Request *) msg_hdr->origin_preq_ptr;
    dat_msg.preq_ptr = msg_hdr->target_preq_ptr;
    win = MPIDI_CH4U_REQUEST(origin_req, req->areq.win_ptr);
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    if (MPIDI_SHM == caller) {
        mpi_errno = MPIDI_SHM_am_isend_reply(MPIDI_CH4U_win_to_context(win),
                                             MPIDI_CH4U_REQUEST(origin_req, src_rank),
                                             MPIDI_CH4U_ACC_DAT_REQ,
                                             &dat_msg, sizeof(dat_msg),
                                             MPIDI_CH4U_REQUEST(origin_req, req->areq.origin_addr),
                                             MPIDI_CH4U_REQUEST(origin_req, req->areq.origin_count),
                                             MPIDI_CH4U_REQUEST(origin_req,
                                                                req->areq.origin_datatype), rreq);
    }
    else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
        mpi_errno = MPIDI_NM_am_isend_reply(MPIDI_CH4U_win_to_context(win),
                                            MPIDI_CH4U_REQUEST(origin_req, src_rank),
                                            MPIDI_CH4U_ACC_DAT_REQ,
                                            &dat_msg, sizeof(dat_msg),
                                            MPIDI_CH4U_REQUEST(origin_req, req->areq.origin_addr),
                                            MPIDI_CH4U_REQUEST(origin_req, req->areq.origin_count),
                                            MPIDI_CH4U_REQUEST(origin_req,
                                                               req->areq.origin_datatype), rreq);
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    dtype_release_if_not_builtin(MPIDI_CH4U_REQUEST(origin_req, req->areq.origin_datatype));

    *cmpl_handler_fn = NULL;
    *req = NULL;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACC_IOV_ACK_HANDLER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_put_data_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_put_data_target_handler(int handler_id, void *am_hdr,
                                                     void **data,
                                                     size_t * p_data_sz,
                                                     int *is_contig,
                                                     MPIDI_NM_am_completion_handler_fn *
                                                     cmpl_handler_fn,
                                                     MPIR_Request ** req, MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDI_CH4U_put_dat_msg_t *msg_hdr = (MPIDI_CH4U_put_dat_msg_t *) am_hdr;
    MPIR_Win *win;
    struct iovec *iov;
    uintptr_t base;
    int i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_PUT_DATA_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_PUT_DATA_HANDLER);

    rreq = (MPIR_Request *) msg_hdr->preq_ptr;
    win = MPIDI_CH4U_REQUEST(rreq, req->preq.win_ptr);
    base = MPIDI_CH4I_win_base_at_target(win);

    /* Adjust the target addresses using the window base address */
    iov = (struct iovec *) MPIDI_CH4U_REQUEST(rreq, req->preq.dt_iov);
    for (i = 0; i < MPIDI_CH4U_REQUEST(rreq, req->preq.n_iov); i++)
        iov[i].iov_base = (char *) iov[i].iov_base + base;

    *data = MPIDI_CH4U_REQUEST(rreq, req->preq.dt_iov);
    *is_contig = 0;
    *p_data_sz = MPIDI_CH4U_REQUEST(rreq, req->preq.n_iov);
    *req = rreq;
    *cmpl_handler_fn = MPIDI_CH4U_put_cmpl_handler;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_PUT_DATA_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_acc_data_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_acc_data_target_handler(int handler_id, void *am_hdr,
                                                     void **data,
                                                     size_t * p_data_sz,
                                                     int *is_contig,
                                                     MPIDI_NM_am_completion_handler_fn *
                                                     cmpl_handler_fn,
                                                     MPIR_Request ** req, MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    size_t data_sz;
    void *p_data = NULL;
    MPIDI_CH4U_acc_dat_msg_t *msg_hdr = (MPIDI_CH4U_acc_dat_msg_t *) am_hdr;
    uintptr_t base;
    int i;
    struct iovec *iov;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACC_DATA_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACC_DATA_HANDLER);

    rreq = (MPIR_Request *) msg_hdr->preq_ptr;
    base = (uintptr_t) MPIDI_CH4U_REQUEST(rreq, req->areq.target_addr);

    MPIDI_Datatype_check_size(MPIDI_CH4U_REQUEST(rreq, req->areq.origin_datatype),
                              MPIDI_CH4U_REQUEST(rreq, req->areq.origin_count), data_sz);
    if (data_sz) {
        p_data = MPL_malloc(data_sz);
        MPIR_Assert(p_data);
    }

    MPIDI_CH4U_REQUEST(rreq, req->areq.data) = p_data;

    /* Adjust the target addresses using the window base address */
    iov = (struct iovec *) MPIDI_CH4U_REQUEST(rreq, req->areq.dt_iov);
    for (i = 0; i < MPIDI_CH4U_REQUEST(rreq, req->areq.n_iov); i++)
        iov[i].iov_base = (char *) iov[i].iov_base + base;

    *data = p_data;
    *is_contig = 1;
    *p_data_sz = data_sz;
    *req = rreq;
    *cmpl_handler_fn = MPIDI_CH4U_REQUEST(rreq, req->areq.do_get) ?
        MPIDI_CH4U_get_acc_cmpl_handler : MPIDI_CH4U_acc_cmpl_handler;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACC_DATA_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_cswap_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_cswap_target_handler(int handler_id, void *am_hdr,
                                                  void **data,
                                                  size_t * p_data_sz,
                                                  int *is_contig,
                                                  MPIDI_NM_am_completion_handler_fn *
                                                  cmpl_handler_fn,
                                                  MPIR_Request ** req, MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    size_t data_sz;
    MPIR_Win *win;
    uintptr_t base;
    size_t offset;

    int dt_contig;
    void *p_data;

    MPIDI_CH4U_cswap_req_msg_t *msg_hdr = (MPIDI_CH4U_cswap_req_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_CSWAP_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_CSWAP_HANDLER);

    rreq = MPIDI_CH4I_am_win_request_create();
    MPIR_Assert(rreq);
    rreq->kind = MPIR_REQUEST_KIND__RMA;
    *req = rreq;

    *cmpl_handler_fn = MPIDI_CH4U_cswap_cmpl_handler;
    MPIDI_CH4U_REQUEST(rreq, req->seq_no) = OPA_fetch_and_add_int(&MPIDI_CH4_Global.nxt_seq_no, 1);

    MPIDI_Datatype_check_contig_size(msg_hdr->datatype, 1, dt_contig, data_sz);
    *is_contig = dt_contig;

    MPL_HASH_FIND(dev.ch4u.hash_handle, MPIDI_CH4_Global.win_hash,
                  &msg_hdr->win_id, sizeof(uint64_t), win);
    MPIR_Assert(win);

    base = MPIDI_CH4I_win_base_at_target(win);

    /* MPIDI_CS_ENTER(); */
    OPA_incr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */
    offset = win->disp_unit * msg_hdr->target_disp;

    MPIDI_CH4U_REQUEST(*req, req->creq.win_ptr) = win;
    MPIDI_CH4U_REQUEST(*req, req->creq.creq_ptr) = msg_hdr->req_ptr;
    MPIDI_CH4U_REQUEST(*req, req->creq.datatype) = msg_hdr->datatype;
    MPIDI_CH4U_REQUEST(*req, req->creq.addr) = offset + base;
    MPIDI_CH4U_REQUEST(*req, src_rank) = msg_hdr->src_rank;

    MPIR_Assert(dt_contig == 1);
    p_data = MPL_malloc(data_sz * 2);
    MPIR_Assert(p_data);

    *p_data_sz = data_sz * 2;
    *data = p_data;
    MPIDI_CH4U_REQUEST(*req, req->creq.data) = p_data;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_CSWAP_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_handle_acc_request
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_handle_acc_request(int handler_id, void *am_hdr,
                                                void **data,
                                                size_t * p_data_sz,
                                                int *is_contig,
                                                MPIDI_NM_am_completion_handler_fn *
                                                cmpl_handler_fn,
                                                MPIR_Request ** req, MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    size_t data_sz;
    void *p_data = NULL;
    struct iovec *iov, *dt_iov;
    MPIR_Win *win;
    uintptr_t base;
    size_t offset;
    int i;

    MPIDI_CH4U_acc_req_msg_t *msg_hdr = (MPIDI_CH4U_acc_req_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_HANDLE_ACC_REQ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_HANDLE_ACC_REQ);

    rreq = MPIDI_CH4I_am_win_request_create();
    MPIR_Assert(rreq);
    rreq->kind = MPIR_REQUEST_KIND__RMA;
    *req = rreq;

    MPIDI_Datatype_check_size(msg_hdr->origin_datatype, msg_hdr->origin_count, data_sz);
    if (data_sz) {
        p_data = MPL_malloc(data_sz);
        MPIR_Assert(p_data);
    }

    *cmpl_handler_fn = (msg_hdr->do_get) ? MPIDI_CH4U_get_acc_cmpl_handler :
        MPIDI_CH4U_acc_cmpl_handler;
    MPIDI_CH4U_REQUEST(rreq, req->seq_no) = OPA_fetch_and_add_int(&MPIDI_CH4_Global.nxt_seq_no, 1);

    if (is_contig) {
        *is_contig = 1;
        *p_data_sz = data_sz;
        *data = p_data;
    }

    MPL_HASH_FIND(dev.ch4u.hash_handle, MPIDI_CH4_Global.win_hash,
                  &msg_hdr->win_id, sizeof(uint64_t), win);
    MPIR_Assert(win);

    base = MPIDI_CH4I_win_base_at_target(win);
    offset = win->disp_unit * msg_hdr->target_disp;
    /* MPIDI_CS_ENTER(); */
    OPA_incr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */

    MPIDI_CH4U_REQUEST(*req, req->areq.win_ptr) = win;
    MPIDI_CH4U_REQUEST(*req, req->areq.req_ptr) = msg_hdr->req_ptr;
    MPIDI_CH4U_REQUEST(*req, req->areq.origin_datatype) = msg_hdr->origin_datatype;
    MPIDI_CH4U_REQUEST(*req, req->areq.target_datatype) = msg_hdr->target_datatype;
    MPIDI_CH4U_REQUEST(*req, req->areq.origin_count) = msg_hdr->origin_count;
    MPIDI_CH4U_REQUEST(*req, req->areq.target_count) = msg_hdr->target_count;
    MPIDI_CH4U_REQUEST(*req, req->areq.target_addr) = (void *) (offset + base);
    MPIDI_CH4U_REQUEST(*req, req->areq.op) = msg_hdr->op;
    MPIDI_CH4U_REQUEST(*req, req->areq.data) = p_data;
    MPIDI_CH4U_REQUEST(*req, req->areq.n_iov) = msg_hdr->n_iov;
    MPIDI_CH4U_REQUEST(*req, req->areq.data_sz) = msg_hdr->result_data_sz;
    MPIDI_CH4U_REQUEST(*req, src_rank) = msg_hdr->src_rank;

    if (!msg_hdr->n_iov) {
        MPIDI_CH4U_REQUEST(rreq, req->areq.dt_iov) = NULL;
        goto fn_exit;
    }

    dt_iov = (struct iovec *) MPL_malloc(sizeof(struct iovec) * msg_hdr->n_iov);
    MPIR_Assert(dt_iov);

    iov = (struct iovec *) ((char *) msg_hdr + sizeof(*msg_hdr));
    for (i = 0; i < msg_hdr->n_iov; i++) {
        dt_iov[i].iov_base = (char *) iov[i].iov_base + base + offset;
        dt_iov[i].iov_len = iov[i].iov_len;
    }
    MPIDI_CH4U_REQUEST(rreq, req->areq.dt_iov) = dt_iov;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_HANDLE_ACC_REQ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_acc_iov_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_acc_iov_target_handler(int handler_id, void *am_hdr,
                                                    void **data,
                                                    size_t * p_data_sz,
                                                    int *is_contig,
                                                    MPIDI_NM_am_completion_handler_fn *
                                                    cmpl_handler_fn,
                                                    MPIR_Request ** req, MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    struct iovec *dt_iov;
    MPIR_Win *win;
    uintptr_t base;
    size_t offset;

    MPIDI_CH4U_acc_req_msg_t *msg_hdr = (MPIDI_CH4U_acc_req_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_HANDLE_ACC_IOV_REQ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_HANDLE_ACC_IOV_REQ);

    rreq = MPIDI_CH4I_am_win_request_create();
    MPIR_Assert(rreq);
    rreq->kind = MPIR_REQUEST_KIND__RMA;
    *req = rreq;

    MPL_HASH_FIND(dev.ch4u.hash_handle, MPIDI_CH4_Global.win_hash,
                  &msg_hdr->win_id, sizeof(uint64_t), win);
    MPIR_Assert(win);

    base = MPIDI_CH4I_win_base_at_target(win);

    offset = win->disp_unit * msg_hdr->target_disp;
    /* MPIDI_CS_ENTER(); */
    OPA_incr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */

    MPIDI_CH4U_REQUEST(*req, req->areq.win_ptr) = win;
    MPIDI_CH4U_REQUEST(*req, req->areq.req_ptr) = msg_hdr->req_ptr;
    MPIDI_CH4U_REQUEST(*req, req->areq.origin_datatype) = msg_hdr->origin_datatype;
    MPIDI_CH4U_REQUEST(*req, req->areq.target_datatype) = msg_hdr->target_datatype;
    MPIDI_CH4U_REQUEST(*req, req->areq.origin_count) = msg_hdr->origin_count;
    MPIDI_CH4U_REQUEST(*req, req->areq.target_count) = msg_hdr->target_count;
    MPIDI_CH4U_REQUEST(*req, req->areq.target_addr) = (void *) (offset + base);
    MPIDI_CH4U_REQUEST(*req, req->areq.op) = msg_hdr->op;
    MPIDI_CH4U_REQUEST(*req, req->areq.n_iov) = msg_hdr->n_iov;
    MPIDI_CH4U_REQUEST(*req, req->areq.data_sz) = msg_hdr->result_data_sz;
    MPIDI_CH4U_REQUEST(*req, req->areq.do_get) = msg_hdr->do_get;
    MPIDI_CH4U_REQUEST(*req, src_rank) = msg_hdr->src_rank;

    dt_iov = (struct iovec *) MPL_malloc(sizeof(struct iovec) * msg_hdr->n_iov);
    MPIDI_CH4U_REQUEST(rreq, req->areq.dt_iov) = dt_iov;
    MPIR_Assert(dt_iov);

    /* Base adjustment for iov will be done after we get the entire iovs,
     * at MPIDI_CH4U_acc_data_target_handler */
    *is_contig = 1;
    *p_data_sz = sizeof(struct iovec) * msg_hdr->n_iov;
    *data = (void *) dt_iov;

    *cmpl_handler_fn = MPIDI_CH4U_acc_iov_cmpl_handler;
    MPIDI_CH4U_REQUEST(rreq, req->seq_no) = OPA_fetch_and_add_int(&MPIDI_CH4_Global.nxt_seq_no, 1);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_HANDLE_ACC_IOV_REQ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_get_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_get_target_handler(int handler_id, void *am_hdr,
                                                void **data,
                                                size_t * p_data_sz,
                                                int *is_contig,
                                                MPIDI_NM_am_completion_handler_fn *
                                                cmpl_handler_fn,
                                                MPIR_Request ** req, MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    MPIDI_CH4U_get_req_msg_t *msg_hdr = (MPIDI_CH4U_get_req_msg_t *) am_hdr;
    struct iovec *iov;
    MPIR_Win *win;
    uintptr_t base;
    size_t offset;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_GET_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_GET_HANDLER);

    rreq = MPIDI_CH4I_am_win_request_create();
    MPIR_Assert(rreq);
    rreq->kind = MPIR_REQUEST_KIND__RMA;

    *req = rreq;
    *cmpl_handler_fn = MPIDI_CH4U_get_cmpl_handler;
    MPIDI_CH4U_REQUEST(rreq, req->seq_no) = OPA_fetch_and_add_int(&MPIDI_CH4_Global.nxt_seq_no, 1);

    MPL_HASH_FIND(dev.ch4u.hash_handle, MPIDI_CH4_Global.win_hash,
                  &msg_hdr->win_id, sizeof(uint64_t), win);
    MPIR_Assert(win);

    base = MPIDI_CH4I_win_base_at_target(win);

    /* MPIDI_CS_ENTER(); */
    OPA_incr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */

    offset = win->disp_unit * msg_hdr->target_disp;
    MPIDI_CH4U_REQUEST(rreq, req->greq.win_ptr) = win;
    MPIDI_CH4U_REQUEST(rreq, req->greq.n_iov) = msg_hdr->n_iov;
    MPIDI_CH4U_REQUEST(rreq, req->greq.addr) = offset + base;
    MPIDI_CH4U_REQUEST(rreq, req->greq.count) = msg_hdr->count;
    MPIDI_CH4U_REQUEST(rreq, req->greq.datatype) = msg_hdr->datatype;
    MPIDI_CH4U_REQUEST(rreq, req->greq.dt_iov) = NULL;
    MPIDI_CH4U_REQUEST(rreq, req->greq.greq_ptr) = msg_hdr->greq_ptr;
    MPIDI_CH4U_REQUEST(rreq, src_rank) = msg_hdr->src_rank;

    if (msg_hdr->n_iov) {
        iov = (struct iovec *) MPL_malloc(msg_hdr->n_iov * sizeof(*iov));
        MPIR_Assert(iov);

        *data = (void *) iov;
        *is_contig = 1;
        *p_data_sz = msg_hdr->n_iov * sizeof(*iov);
        MPIDI_CH4U_REQUEST(rreq, req->greq.dt_iov) = iov;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_GET_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_get_ack_target_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_get_ack_target_handler(int handler_id, void *am_hdr,
                                                    void **data,
                                                    size_t * p_data_sz,
                                                    int *is_contig,
                                                    MPIDI_NM_am_completion_handler_fn *
                                                    cmpl_handler_fn,
                                                    MPIR_Request ** req, MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL, *greq;
    size_t data_sz;

    int dt_contig, n_iov;
    MPI_Aint dt_true_lb, last, num_iov;
    MPIR_Datatype *dt_ptr;
    MPID_Segment *segment_ptr;

    MPIDI_CH4U_get_ack_msg_t *msg_hdr = (MPIDI_CH4U_get_ack_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_GET_ACK_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_GET_ACK_HANDLER);

    greq = MPIDI_CH4I_am_win_request_create();
    MPIR_Assert(greq);
    greq->kind = MPIR_REQUEST_KIND__RMA;
    *req = greq;

    rreq = (MPIR_Request *) msg_hdr->greq_ptr;
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__RMA);
    MPIDI_CH4U_REQUEST(greq, req->greq.greq_ptr) = (uint64_t) rreq;

    if (MPIDI_CH4U_REQUEST(rreq, req->greq.dt_iov)) {
        MPL_free(MPIDI_CH4U_REQUEST(rreq, req->greq.dt_iov));
    }

    *cmpl_handler_fn = MPIDI_CH4U_get_ack_cmpl_handler;
    MPIDI_CH4U_REQUEST(greq, req->seq_no) = OPA_fetch_and_add_int(&MPIDI_CH4_Global.nxt_seq_no, 1);

    MPIDI_Datatype_get_info(MPIDI_CH4U_REQUEST(rreq, req->greq.count),
                            MPIDI_CH4U_REQUEST(rreq, req->greq.datatype),
                            dt_contig, data_sz, dt_ptr, dt_true_lb);

    *is_contig = dt_contig;

    if (dt_contig) {
        *p_data_sz = data_sz;
        *data = (char *) (MPIDI_CH4U_REQUEST(rreq, req->greq.addr) + dt_true_lb);
    }
    else {
        segment_ptr = MPIDU_Segment_alloc();
        MPIR_Assert(segment_ptr);

        MPIDU_Segment_init((void *) MPIDI_CH4U_REQUEST(rreq, req->greq.addr),
                           MPIDI_CH4U_REQUEST(rreq, req->greq.count),
                           MPIDI_CH4U_REQUEST(rreq, req->greq.datatype), segment_ptr, 0);
        last = data_sz;
        MPIDU_Segment_count_contig_blocks(segment_ptr, 0, &last, &num_iov);
        n_iov = (int) num_iov;
        MPIR_Assert(n_iov > 0);
        MPIDI_CH4U_REQUEST(rreq, req->iov) =
            (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec));
        MPIR_Assert(MPIDI_CH4U_REQUEST(rreq, req->iov));

        last = data_sz;
        MPIDU_Segment_pack_vector(segment_ptr, 0, &last, MPIDI_CH4U_REQUEST(rreq, req->iov),
                                  &n_iov);
        MPIR_Assert(last == (MPI_Aint) data_sz);
        *data = MPIDI_CH4U_REQUEST(rreq, req->iov);
        *p_data_sz = n_iov;
        MPIDI_CH4U_REQUEST(rreq, req->status) |= MPIDI_CH4U_REQ_RCV_NON_CONTIG;
        MPL_free(segment_ptr);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_GET_ACK_HANDLER);
    return mpi_errno;
}

#endif /* CH4R_CALLBACKS_H_INCLUDED */
