/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "ch4r_rma_target_callbacks.h"

static int ack_put(MPIR_Request * rreq);
static int ack_cswap(MPIR_Request * rreq);
static int ack_acc(MPIR_Request * rreq);
static int ack_get_acc(MPIR_Request * rreq);
static int win_lock_advance(MPIR_Win * win);
static void win_lock_req_proc(int handler_id, const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win);
static void win_lock_ack_proc(int handler_id, const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win);
static void win_unlock_proc(const MPIDIG_win_cntrl_msg_t * info, int is_local, MPIR_Win * win);
static void win_complete_proc(const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win);
static void win_post_proc(const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win);
static void win_unlock_done(const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win);
static int handle_acc_cmpl(MPIR_Request * rreq);
static int handle_get_acc_cmpl(MPIR_Request * rreq);
static void handle_acc_data(void **data, size_t * p_data_sz, int *is_contig, MPIR_Request * rreq);
static int get_target_cmpl_cb(MPIR_Request * req);
static int put_target_cmpl_cb(MPIR_Request * rreq);
static int put_iov_target_cmpl_cb(MPIR_Request * rreq);
static int acc_iov_target_cmpl_cb(MPIR_Request * rreq);
static int get_acc_iov_target_cmpl_cb(MPIR_Request * rreq);
static int cswap_target_cmpl_cb(MPIR_Request * rreq);
static int acc_target_cmpl_cb(MPIR_Request * rreq);
static int get_acc_target_cmpl_cb(MPIR_Request * rreq);
static int get_ack_target_cmpl_cb(MPIR_Request * rreq);
static int get_acc_ack_target_cmpl_cb(MPIR_Request * areq);
static int cswap_ack_target_cmpl_cb(MPIR_Request * rreq);

int MPIDIG_RMA_Init_targetcb_pvars(void)
{
    int mpi_errno = MPI_SUCCESS;
    /* rma_targetcb_put */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_put,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Put (in seconds)");

    /* rma_targetcb_put_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_put_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Put ACK (in seconds)");

    /* rma_targetcb_get */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_get,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Get (in seconds)");

    /* rma_targetcb_get_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_get_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Get ACK (in seconds)");

    /* rma_targetcb_cas */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_cas,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Compare-and-swap (in seconds)");

    /* rma_targetcb_cas_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_cas_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Compare-and-swap ACK (in seconds)");

    /* rma_targetcb_acc */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_acc,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Accumulate (in seconds)");

    /* rma_targetcb_get_acc */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_get_acc,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Get-Accumulate (in seconds)");

    /* rma_targetcb_acc_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_acc_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Accumulate ACK (in seconds)");

    /* rma_targetcb_get_acc_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_get_acc_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Get-Accumulate ACK (in seconds)");

    /* rma_targetcb_win_ctrl */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_win_ctrl,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for WIN CTRL (in seconds)");

    /* rma_targetcb_put_iov */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_put_iov,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for PUT IOV (in seconds)");

    /* rma_targetcb_put_iov_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_put_iov_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for PUT IOV ACK (in seconds)");

    /* rma_targetcb_put_data */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_put_data,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for PUT DATA (in seconds)");

    /* rma_targetcb_acc_iov */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_acc_iov,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for ACC IOV (in seconds)");

    /* rma_targetcb_get_acc_iov */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_get_acc_iov,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for GET ACC IOV (in seconds)");

    /* rma_targetcb_acc_iov_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_acc_iov_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for ACC IOV ACK (in seconds)");

    /* rma_targetcb_get_acc_iov_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_get_acc_iov_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for GET ACC IOV ACK (in seconds)");

    /* rma_targetcb_acc_data */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_acc_data,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for ACC DATA (in seconds)");

    /* rma_targetcb_get_acc_data */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_get_acc_data,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for GET ACC DATA (in seconds)");

    return mpi_errno;
}

static int ack_put(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_put_ack_msg_t ack_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACK_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACK_PUT);

    ack_msg.preq_ptr = MPIDIG_REQUEST(rreq, req->preq.preq_ptr);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(MPIDIG_win_to_context
                                        (MPIDIG_REQUEST(rreq, req->preq.win_ptr)),
                                        MPIDIG_REQUEST(rreq, rank), MPIDIG_PUT_ACK,
                                        &ack_msg, sizeof(ack_msg));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDIG_win_to_context
                                       (MPIDIG_REQUEST(rreq, req->preq.win_ptr)),
                                       MPIDIG_REQUEST(rreq, rank), MPIDIG_PUT_ACK,
                                       &ack_msg, sizeof(ack_msg));
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACK_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ack_cswap(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIDIG_cswap_ack_msg_t ack_msg;
    void *result_addr;
    size_t data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACK_CSWAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACK_CSWAP);

    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, req->creq.datatype), 1, data_sz);
    result_addr = ((char *) MPIDIG_REQUEST(rreq, req->creq.data)) + data_sz;

    MPIR_cc_incr(rreq->cc_ptr, &c);
    ack_msg.req_ptr = MPIDIG_REQUEST(rreq, req->creq.creq_ptr);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_isend_reply(MPIDIG_win_to_context
                                     (MPIDIG_REQUEST(rreq, req->creq.win_ptr)),
                                     MPIDIG_REQUEST(rreq, rank), MPIDIG_CSWAP_ACK, &ack_msg,
                                     sizeof(ack_msg), result_addr, 1,
                                     MPIDIG_REQUEST(rreq, req->creq.datatype), rreq);
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_isend_reply(MPIDIG_win_to_context
                                    (MPIDIG_REQUEST(rreq, req->creq.win_ptr)),
                                    MPIDIG_REQUEST(rreq, rank), MPIDIG_CSWAP_ACK, &ack_msg,
                                    sizeof(ack_msg), result_addr, 1,
                                    MPIDIG_REQUEST(rreq, req->creq.datatype), rreq);
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACK_CSWAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ack_acc(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_acc_ack_msg_t ack_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACK_ACC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACK_ACC);

    ack_msg.req_ptr = MPIDIG_REQUEST(rreq, req->areq.req_ptr);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(MPIDIG_win_to_context
                                        (MPIDIG_REQUEST(rreq, req->areq.win_ptr)),
                                        MPIDIG_REQUEST(rreq, rank), MPIDIG_ACC_ACK,
                                        &ack_msg, sizeof(ack_msg));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDIG_win_to_context
                                       (MPIDIG_REQUEST(rreq, req->areq.win_ptr)),
                                       MPIDIG_REQUEST(rreq, rank), MPIDIG_ACC_ACK,
                                       &ack_msg, sizeof(ack_msg));
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACK_ACC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ack_get_acc(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIDIG_acc_ack_msg_t ack_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACK_GET_ACC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACK_GET_ACC);

    MPIR_cc_incr(rreq->cc_ptr, &c);
    ack_msg.req_ptr = MPIDIG_REQUEST(rreq, req->areq.req_ptr);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_isend_reply(MPIDIG_win_to_context
                                     (MPIDIG_REQUEST(rreq, req->areq.win_ptr)),
                                     MPIDIG_REQUEST(rreq, rank), MPIDIG_GET_ACC_ACK,
                                     &ack_msg, sizeof(ack_msg),
                                     MPIDIG_REQUEST(rreq, req->areq.data),
                                     MPIDIG_REQUEST(rreq, req->areq.data_sz), MPI_BYTE, rreq);
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_isend_reply(MPIDIG_win_to_context
                                    (MPIDIG_REQUEST(rreq, req->areq.win_ptr)),
                                    MPIDIG_REQUEST(rreq, rank), MPIDIG_GET_ACC_ACK,
                                    &ack_msg, sizeof(ack_msg),
                                    MPIDIG_REQUEST(rreq, req->areq.data),
                                    MPIDIG_REQUEST(rreq, req->areq.data_sz), MPI_BYTE, rreq);
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACK_GET_ACC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static int win_lock_advance(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_win_lock_recvd_t *lock_recvd_q = &MPIDIG_WIN(win, sync).lock_recvd;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_LOCK_ADVANCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_LOCK_ADVANCE);

    if ((lock_recvd_q->head != NULL) && ((lock_recvd_q->count == 0) ||
                                         ((lock_recvd_q->type == MPI_LOCK_SHARED) &&
                                          (lock_recvd_q->head->type == MPI_LOCK_SHARED)))) {
        struct MPIDIG_win_lock *lock = lock_recvd_q->head;
        lock_recvd_q->head = lock->next;

        if (lock_recvd_q->head == NULL)
            lock_recvd_q->tail = NULL;

        ++lock_recvd_q->count;
        lock_recvd_q->type = lock->type;

        MPIDIG_win_cntrl_msg_t msg;
        int handler_id;
        msg.win_id = MPIDIG_WIN(win, win_id);
        msg.origin_rank = win->comm_ptr->rank;

        if (lock->mtype == MPIDIG_WIN_LOCK)
            handler_id = MPIDIG_WIN_LOCK_ACK;
        else if (lock->mtype == MPIDIG_WIN_LOCKALL)
            handler_id = MPIDIG_WIN_LOCKALL_ACK;
        else
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**rmasync");

#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (MPIDI_rank_is_local(lock->rank, win->comm_ptr))
            mpi_errno = MPIDI_SHM_am_send_hdr_reply(MPIDIG_win_to_context(win),
                                                    lock->rank, handler_id, &msg, sizeof(msg));
        else
#endif
        {
            mpi_errno = MPIDI_NM_am_send_hdr_reply(MPIDIG_win_to_context(win),
                                                   lock->rank, handler_id, &msg, sizeof(msg));
        }

        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPL_free(lock);

        mpi_errno = win_lock_advance(win);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_LOCK_ADVANCE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void win_lock_req_proc(int handler_id, const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_LOCK_REQ_PROC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_LOCK_REQ_PROC);

    MPIR_T_PVAR_TIMER_START(RMA, rma_winlock_getlocallock);
    struct MPIDIG_win_lock *lock = (struct MPIDIG_win_lock *)
        MPL_calloc(1, sizeof(struct MPIDIG_win_lock), MPL_MEM_RMA);

    lock->mtype = handler_id;
    lock->rank = info->origin_rank;
    lock->type = info->lock_type;
    MPIDIG_win_lock_recvd_t *lock_recvd_q = &MPIDIG_WIN(win, sync).lock_recvd;
    MPIR_Assert((lock_recvd_q->head != NULL) ^ (lock_recvd_q->tail == NULL));

    if (lock_recvd_q->tail == NULL)
        lock_recvd_q->head = lock;
    else
        lock_recvd_q->tail->next = lock;

    lock_recvd_q->tail = lock;

    win_lock_advance(win);
    MPIR_T_PVAR_TIMER_END(RMA, rma_winlock_getlocallock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_LOCK_REQ_PROC);
    return;
}

static void win_lock_ack_proc(int handler_id, const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_LOCK_ACK_PROC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_LOCK_ACK_PROC);

    if (handler_id == MPIDIG_WIN_LOCK_ACK) {
        MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_find(win, info->origin_rank);
        MPIR_Assert(target_ptr);

        MPIR_Assert((int) target_ptr->sync.lock.locked == 0);
        target_ptr->sync.lock.locked = 1;
    } else if (handler_id == MPIDIG_WIN_LOCKALL_ACK) {
        MPIDIG_WIN(win, sync).lockall.allLocked += 1;
    } else {
        MPIR_Assert(0);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_LOCK_ACK_PROC);
}

static void win_unlock_proc(const MPIDIG_win_cntrl_msg_t * info, int is_local, MPIR_Win * win)
{

    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_UNLOCK_PROC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_UNLOCK_PROC);

    /* NOTE: origin blocking waits in lock or lockall call till lock granted. */
    --MPIDIG_WIN(win, sync).lock_recvd.count;
    MPIR_Assert((int) MPIDIG_WIN(win, sync).lock_recvd.count >= 0);
    win_lock_advance(win);

    MPIDIG_win_cntrl_msg_t msg;
    msg.win_id = MPIDIG_WIN(win, win_id);
    msg.origin_rank = win->comm_ptr->rank;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (is_local)
        mpi_errno = MPIDI_SHM_am_send_hdr_reply(MPIDIG_win_to_context(win),
                                                info->origin_rank,
                                                MPIDIG_WIN_UNLOCK_ACK, &msg, sizeof(msg));
    else
#endif
    {
        mpi_errno = MPIDI_NM_am_send_hdr_reply(MPIDIG_win_to_context(win),
                                               info->origin_rank,
                                               MPIDIG_WIN_UNLOCK_ACK, &msg, sizeof(msg));
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_UNLOCK_PROC);
    return;
  fn_fail:
    goto fn_exit;
}

static void win_complete_proc(const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_COMPLETE_PROC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_COMPLETE_PROC);

    ++MPIDIG_WIN(win, sync).sc.count;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_COMPLETE_PROC);
}

static void win_post_proc(const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_POST_PROC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_POST_PROC);

    ++MPIDIG_WIN(win, sync).pw.count;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_POST_PROC);
}


static void win_unlock_done(const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_UNLOCK_DONE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_UNLOCK_DONE);

    if (MPIDIG_WIN(win, sync).access_epoch_type == MPIDIG_EPOTYPE_LOCK) {
        MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_find(win, info->origin_rank);
        MPIR_Assert(target_ptr);

        MPIR_Assert((int) target_ptr->sync.lock.locked == 1);
        target_ptr->sync.lock.locked = 0;
    } else if (MPIDIG_WIN(win, sync).access_epoch_type == MPIDIG_EPOTYPE_LOCK_ALL) {
        MPIR_Assert((int) MPIDIG_WIN(win, sync).lockall.allLocked > 0);
        MPIDIG_WIN(win, sync).lockall.allLocked -= 1;
    } else {
        MPIR_Assert(0);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_UNLOCK_DONE);
}

static int handle_acc_cmpl(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, i;
    MPI_Aint basic_sz, count;
    struct iovec *iov;
    char *src_ptr;
    size_t data_sz;
    int shm_locked ATTRIBUTE((unused)) = 0;
    MPIR_Win *win ATTRIBUTE((unused)) = MPIDIG_REQUEST(rreq, req->areq.win_ptr);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_HANDLE_ACC_CMPL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_HANDLE_ACC_CMPL);

    MPIR_Datatype_get_size_macro(MPIDIG_REQUEST(rreq, req->areq.target_datatype), basic_sz);
    MPIR_ERR_CHKANDJUMP(basic_sz == 0, mpi_errno, MPI_ERR_OTHER, "**dtype");
    data_sz = MPIDIG_REQUEST(rreq, req->areq.data_sz);

    /* MPIDI_CS_ENTER(); */

    if (MPIDIG_REQUEST(rreq, req->areq.op) == MPI_NO_OP) {
        MPIDIG_REQUEST(rreq, req->areq.origin_count) = MPIDIG_REQUEST(rreq, req->areq.target_count);
        MPIDIG_REQUEST(rreq, req->areq.data_sz) = data_sz;
    }
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDIG_WIN(win, shm_allocated)) {
        mpi_errno = MPIDI_SHM_rma_op_cs_enter_hook(win);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        shm_locked = 1;
    }
#endif

    if (MPIDIG_REQUEST(rreq, req->areq.dt_iov) == NULL) {
        mpi_errno = MPIDIG_compute_acc_op(MPIDIG_REQUEST(rreq, req->areq.data),
                                          MPIDIG_REQUEST(rreq, req->areq.origin_count),
                                          MPIDIG_REQUEST(rreq, req->areq.origin_datatype),
                                          MPIDIG_REQUEST(rreq, req->areq.target_addr),
                                          MPIDIG_REQUEST(rreq, req->areq.target_count),
                                          MPIDIG_REQUEST(rreq, req->areq.target_datatype),
                                          MPIDIG_REQUEST(rreq, req->areq.op),
                                          MPIDIG_ACC_SRCBUF_DEFAULT);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        iov = (struct iovec *) MPIDIG_REQUEST(rreq, req->areq.dt_iov);
        src_ptr = (char *) MPIDIG_REQUEST(rreq, req->areq.data);
        for (i = 0; i < MPIDIG_REQUEST(rreq, req->areq.n_iov); i++) {
            count = iov[i].iov_len / basic_sz;
            MPIR_Assert(count > 0);

            mpi_errno = MPIDIG_compute_acc_op(src_ptr, count,
                                              MPIDIG_REQUEST(rreq, req->areq.origin_datatype),
                                              iov[i].iov_base, count,
                                              MPIDIG_REQUEST(rreq, req->areq.target_datatype),
                                              MPIDIG_REQUEST(rreq, req->areq.op),
                                              MPIDIG_ACC_SRCBUF_DEFAULT);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            src_ptr += count * basic_sz;
        }
        MPL_free(iov);
    }

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDIG_WIN(win, shm_allocated)) {
        mpi_errno = MPIDI_SHM_rma_op_cs_exit_hook(win);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
#endif

    /* MPIDI_CS_EXIT(); */
    MPL_free(MPIDIG_REQUEST(rreq, req->areq.data));

    MPIDIG_REQUEST(rreq, req->areq.data) = NULL;
    mpi_errno = ack_acc(rreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPID_Request_complete(rreq);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_HANDLE_ACC_CMPL);
    return mpi_errno;
  fn_fail:
#ifndef MPIDI_CH4_DIRECT_NETMOD
    /* release lock if error is reported inside CS. */
    if (shm_locked)
        MPIDI_SHM_rma_op_cs_exit_hook(win);
#endif
    goto fn_exit;
}

static int handle_get_acc_cmpl(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, i;
    MPI_Aint basic_sz, count, offset = 0;
    struct iovec *iov;
    char *src_ptr, *original = NULL;
    size_t data_sz;
    int shm_locked ATTRIBUTE((unused)) = 0;
    MPIR_Win *win ATTRIBUTE((unused)) = MPIDIG_REQUEST(rreq, req->areq.win_ptr);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_HANDLE_GET_ACC_CMPL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_HANDLE_GET_ACC_CMPL);

    MPIR_Datatype_get_size_macro(MPIDIG_REQUEST(rreq, req->areq.target_datatype), basic_sz);
    MPIR_Assert(basic_sz);
    data_sz = MPIDIG_REQUEST(rreq, req->areq.data_sz);

    /* MPIDI_CS_ENTER(); */

    original = (char *) MPL_malloc(data_sz, MPL_MEM_RMA);
    MPIR_Assert(original);

    if (MPIDIG_REQUEST(rreq, req->areq.op) == MPI_NO_OP) {
        MPIDIG_REQUEST(rreq, req->areq.origin_count) = MPIDIG_REQUEST(rreq, req->areq.target_count);
        MPIDIG_REQUEST(rreq, req->areq.data_sz) = data_sz;
    }
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDIG_WIN(win, shm_allocated)) {
        mpi_errno = MPIDI_SHM_rma_op_cs_enter_hook(win);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        shm_locked = 1;
    }
#endif

    if (MPIDIG_REQUEST(rreq, req->areq.dt_iov) == NULL) {

        MPIR_Memcpy(original, MPIDIG_REQUEST(rreq, req->areq.target_addr),
                    basic_sz * MPIDIG_REQUEST(rreq, req->areq.target_count));

        mpi_errno = MPIDIG_compute_acc_op(MPIDIG_REQUEST(rreq, req->areq.data),
                                          MPIDIG_REQUEST(rreq, req->areq.origin_count),
                                          MPIDIG_REQUEST(rreq, req->areq.origin_datatype),
                                          MPIDIG_REQUEST(rreq, req->areq.target_addr),
                                          MPIDIG_REQUEST(rreq, req->areq.target_count),
                                          MPIDIG_REQUEST(rreq, req->areq.target_datatype),
                                          MPIDIG_REQUEST(rreq, req->areq.op),
                                          MPIDIG_ACC_SRCBUF_DEFAULT);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        iov = (struct iovec *) MPIDIG_REQUEST(rreq, req->areq.dt_iov);
        src_ptr = (char *) MPIDIG_REQUEST(rreq, req->areq.data);
        for (i = 0; i < MPIDIG_REQUEST(rreq, req->areq.n_iov); i++) {
            count = iov[i].iov_len / basic_sz;
            MPIR_Assert(count > 0);

            MPIR_Memcpy(original + offset, iov[i].iov_base, count * basic_sz);
            offset += count * basic_sz;

            mpi_errno = MPIDIG_compute_acc_op(src_ptr, count,
                                              MPIDIG_REQUEST(rreq, req->areq.origin_datatype),
                                              iov[i].iov_base, count,
                                              MPIDIG_REQUEST(rreq, req->areq.target_datatype),
                                              MPIDIG_REQUEST(rreq, req->areq.op),
                                              MPIDIG_ACC_SRCBUF_DEFAULT);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            src_ptr += count * basic_sz;
        }
        MPL_free(iov);
    }

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDIG_WIN(win, shm_allocated)) {
        mpi_errno = MPIDI_SHM_rma_op_cs_exit_hook(win);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
#endif

    /* MPIDI_CS_EXIT(); */
    MPL_free(MPIDIG_REQUEST(rreq, req->areq.data));

    MPIDIG_REQUEST(rreq, req->areq.data) = original;
    mpi_errno = ack_get_acc(rreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPID_Request_complete(rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_HANDLE_GET_ACC_CMPL);
    return mpi_errno;
  fn_fail:
#ifndef MPIDI_CH4_DIRECT_NETMOD
    /* release lock if error is reported inside CS. */
    if (shm_locked)
        MPIDI_SHM_rma_op_cs_exit_hook(win);
#endif
    goto fn_exit;
}

static void handle_acc_data(void **data, size_t * p_data_sz, int *is_contig, MPIR_Request * rreq)
{
    void *p_data = NULL;
    size_t data_sz;
    uintptr_t base;
    int i;
    struct iovec *iov;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_HANDLE_ACC_DATA);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_HANDLE_ACC_DATA);

    base = (uintptr_t) MPIDIG_REQUEST(rreq, req->areq.target_addr);
    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, req->areq.origin_datatype),
                              MPIDIG_REQUEST(rreq, req->areq.origin_count), data_sz);
    if (data_sz) {
        p_data = MPL_malloc(data_sz, MPL_MEM_RMA);
        MPIR_Assert(p_data);
    }

    MPIDIG_REQUEST(rreq, req->areq.data) = p_data;

    /* Adjust the target iov addresses using the base address
     * (window base + target_disp) */
    iov = (struct iovec *) MPIDIG_REQUEST(rreq, req->areq.dt_iov);
    for (i = 0; i < MPIDIG_REQUEST(rreq, req->areq.n_iov); i++)
        iov[i].iov_base = (char *) iov[i].iov_base + base;

    *data = p_data;
    *is_contig = 1;
    *p_data_sz = data_sz;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_HANDLE_ACC_DATA);
}

static int get_target_cmpl_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS, i, c;
    size_t data_sz, offset;
    MPIDIG_get_ack_msg_t get_ack;
    struct iovec *iov;
    char *p_data;
    uintptr_t base;
    MPIR_Win *win;
    MPIR_Context_id_t context_id;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_TARGET_CMPL_CB);

    base = MPIDIG_REQUEST(req, req->greq.addr);

    MPIR_cc_incr(req->cc_ptr, &c);
    get_ack.greq_ptr = MPIDIG_REQUEST(req, req->greq.greq_ptr);
    win = MPIDIG_REQUEST(req, req->greq.win_ptr);
    context_id = MPIDIG_win_to_context(win);

    if (MPIDIG_REQUEST(req, req->greq.n_iov) == 0) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (MPIDI_REQUEST(req, is_local))
            mpi_errno = MPIDI_SHM_am_isend_reply(context_id, MPIDIG_REQUEST(req, rank),
                                                 MPIDIG_GET_ACK, &get_ack, sizeof(get_ack),
                                                 (void *) MPIDIG_REQUEST(req, req->greq.addr),
                                                 MPIDIG_REQUEST(req, req->greq.count),
                                                 MPIDIG_REQUEST(req, req->greq.datatype), req);
        else
#endif
        {
            mpi_errno = MPIDI_NM_am_isend_reply(context_id, MPIDIG_REQUEST(req, rank),
                                                MPIDIG_GET_ACK, &get_ack, sizeof(get_ack),
                                                (void *) MPIDIG_REQUEST(req, req->greq.addr),
                                                MPIDIG_REQUEST(req, req->greq.count),
                                                MPIDIG_REQUEST(req, req->greq.datatype), req);
        }

        MPID_Request_complete(req);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    iov = (struct iovec *) MPIDIG_REQUEST(req, req->greq.dt_iov);

    data_sz = 0;
    for (i = 0; i < MPIDIG_REQUEST(req, req->greq.n_iov); i++) {
        data_sz += iov[i].iov_len;
    }

    p_data = (char *) MPL_malloc(data_sz, MPL_MEM_RMA);
    MPIR_Assert(p_data);

    offset = 0;
    for (i = 0; i < MPIDIG_REQUEST(req, req->greq.n_iov); i++) {
        /* Adjust a window base address */
        iov[i].iov_base = (char *) iov[i].iov_base + base;
        MPIR_Memcpy(p_data + offset, iov[i].iov_base, iov[i].iov_len);
        offset += iov[i].iov_len;
    }

    MPL_free(MPIDIG_REQUEST(req, req->greq.dt_iov));
    MPIDIG_REQUEST(req, req->greq.dt_iov) = (void *) p_data;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(req, is_local))
        mpi_errno = MPIDI_SHM_am_isend_reply(context_id, MPIDIG_REQUEST(req, rank),
                                             MPIDIG_GET_ACK, &get_ack, sizeof(get_ack), p_data,
                                             data_sz, MPI_BYTE, req);
    else
#endif
    {
        mpi_errno = MPIDI_NM_am_isend_reply(context_id, MPIDIG_REQUEST(req, rank),
                                            MPIDIG_GET_ACK, &get_ack, sizeof(get_ack), p_data,
                                            data_sz, MPI_BYTE, req);
    }

    MPID_Request_complete(req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int put_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_TARGET_CMPL_CB);

    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_RCV_NON_CONTIG) {
        MPL_free(MPIDIG_REQUEST(rreq, req->iov));
    }

    MPL_free(MPIDIG_REQUEST(rreq, req->preq.dt_iov));

    mpi_errno = ack_put(rreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPID_Request_complete(rreq);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int put_iov_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_put_iov_ack_msg_t ack_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_IOV_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_IOV_TARGET_CMPL_CB);

    ack_msg.src_rank = MPIDIG_REQUEST(rreq, rank);
    ack_msg.origin_preq_ptr = (uint64_t) MPIDIG_REQUEST(rreq, req->preq.preq_ptr);
    ack_msg.target_preq_ptr = (uint64_t) rreq;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(MPIDIG_win_to_context
                                        (MPIDIG_REQUEST(rreq, req->preq.win_ptr)),
                                        MPIDIG_REQUEST(rreq, rank), MPIDIG_PUT_IOV_ACK,
                                        &ack_msg, sizeof(ack_msg));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDIG_win_to_context
                                       (MPIDIG_REQUEST(rreq, req->preq.win_ptr)),
                                       MPIDIG_REQUEST(rreq, rank), MPIDIG_PUT_IOV_ACK,
                                       &ack_msg, sizeof(ack_msg));
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_IOV_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int acc_iov_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_acc_iov_ack_msg_t ack_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACC_IOV_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACC_IOV_TARGET_CMPL_CB);

    ack_msg.origin_preq_ptr = (uint64_t) MPIDIG_REQUEST(rreq, req->areq.req_ptr);
    ack_msg.target_preq_ptr = (uint64_t) rreq;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(MPIDIG_win_to_context
                                        (MPIDIG_REQUEST(rreq, req->areq.win_ptr)),
                                        MPIDIG_REQUEST(rreq, rank), MPIDIG_ACC_IOV_ACK,
                                        &ack_msg, sizeof(ack_msg));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDIG_win_to_context
                                       (MPIDIG_REQUEST(rreq, req->areq.win_ptr)),
                                       MPIDIG_REQUEST(rreq, rank), MPIDIG_ACC_IOV_ACK,
                                       &ack_msg, sizeof(ack_msg));
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACC_IOV_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_acc_iov_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_get_acc_iov_ack_msg_t ack_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_IOV_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_IOV_TARGET_CMPL_CB);

    ack_msg.origin_preq_ptr = (uint64_t) MPIDIG_REQUEST(rreq, req->areq.req_ptr);
    ack_msg.target_preq_ptr = (uint64_t) rreq;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(MPIDIG_win_to_context
                                        (MPIDIG_REQUEST(rreq, req->areq.win_ptr)),
                                        MPIDIG_REQUEST(rreq, rank), MPIDIG_GET_ACC_IOV_ACK,
                                        &ack_msg, sizeof(ack_msg));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDIG_win_to_context
                                       (MPIDIG_REQUEST(rreq, req->areq.win_ptr)),
                                       MPIDIG_REQUEST(rreq, rank), MPIDIG_GET_ACC_IOV_ACK,
                                       &ack_msg, sizeof(ack_msg));
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_IOV_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int cswap_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    void *compare_addr;
    void *origin_addr;
    size_t data_sz;
    MPIR_Win *win ATTRIBUTE((unused)) = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_CSWAP_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_CSWAP_TARGET_CMPL_CB);

    if (!MPIDIG_check_cmpl_order(rreq, cswap_target_cmpl_cb))
        return mpi_errno;

    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, req->creq.datatype), 1, data_sz);
    origin_addr = MPIDIG_REQUEST(rreq, req->creq.data);
    compare_addr = ((char *) MPIDIG_REQUEST(rreq, req->creq.data)) + data_sz;

    /* MPIDI_CS_ENTER(); */
#ifndef MPIDI_CH4_DIRECT_NETMOD
    win = MPIDIG_REQUEST(rreq, req->creq.win_ptr);
    if (MPIDIG_WIN(win, shm_allocated)) {
        mpi_errno = MPIDI_SHM_rma_op_cs_enter_hook(win);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
#endif

    if (MPIR_Compare_equal((void *) MPIDIG_REQUEST(rreq, req->creq.addr), compare_addr,
                           MPIDIG_REQUEST(rreq, req->creq.datatype))) {
        MPIR_Memcpy(compare_addr, (void *) MPIDIG_REQUEST(rreq, req->creq.addr), data_sz);
        MPIR_Memcpy((void *) MPIDIG_REQUEST(rreq, req->creq.addr), origin_addr, data_sz);
    } else {
        MPIR_Memcpy(compare_addr, (void *) MPIDIG_REQUEST(rreq, req->creq.addr), data_sz);
    }

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDIG_WIN(win, shm_allocated)) {
        mpi_errno = MPIDI_SHM_rma_op_cs_exit_hook(win);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
#endif
    /* MPIDI_CS_EXIT(); */

    mpi_errno = ack_cswap(rreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPID_Request_complete(rreq);
    MPIDIG_progress_compl_list();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_CSWAP_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}



static int acc_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACC_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACC_TARGET_CMPL_CB);

    if (!MPIDIG_check_cmpl_order(rreq, acc_target_cmpl_cb))
        return mpi_errno;

    mpi_errno = handle_acc_cmpl(rreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIDIG_progress_compl_list();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACC_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_acc_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_TARGET_CMPL_CB);

    if (!MPIDIG_check_cmpl_order(rreq, get_acc_target_cmpl_cb))
        return mpi_errno;

    mpi_errno = handle_get_acc_cmpl(rreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIDIG_progress_compl_list();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_ack_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *greq;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACK_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACK_TARGET_CMPL_CB);

    greq = (MPIR_Request *) MPIDIG_REQUEST(rreq, req->greq.greq_ptr);
    if (MPIDIG_REQUEST(greq, req->status) & MPIDIG_REQ_RCV_NON_CONTIG) {
        MPL_free(MPIDIG_REQUEST(greq, req->iov));
    }

    win = MPIDIG_REQUEST(greq, req->greq.win_ptr);
    MPIDIG_win_remote_cmpl_cnt_decr(win, MPIDIG_REQUEST(greq, rank));

    MPID_Request_complete(greq);
    MPID_Request_complete(rreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACK_TARGET_CMPL_CB);
    return mpi_errno;
}


static int get_acc_ack_target_cmpl_cb(MPIR_Request * areq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_ACK_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_ACK_TARGET_CMPL_CB);

    if (MPIDIG_REQUEST(areq, req->status) & MPIDIG_REQ_RCV_NON_CONTIG) {
        MPL_free(MPIDIG_REQUEST(areq, req->iov));
    }

    win = MPIDIG_REQUEST(areq, req->areq.win_ptr);
    MPIDIG_win_remote_cmpl_cnt_decr(win, MPIDIG_REQUEST(areq, rank));
    MPIDIG_win_remote_acc_cmpl_cnt_decr(win, MPIDIG_REQUEST(areq, rank));

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(areq, req->areq.result_datatype));
    MPID_Request_complete(areq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_ACK_TARGET_CMPL_CB);
    return mpi_errno;
}

static int cswap_ack_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_CSWAP_ACK_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_CSWAP_ACK_TARGET_CMPL_CB);

    win = MPIDIG_REQUEST(rreq, req->creq.win_ptr);
    MPIDIG_win_remote_cmpl_cnt_decr(win, MPIDIG_REQUEST(rreq, rank));
    MPIDIG_win_remote_acc_cmpl_cnt_decr(win, MPIDIG_REQUEST(rreq, rank));

    MPL_free(MPIDIG_REQUEST(rreq, req->creq.data));
    MPID_Request_complete(rreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_CSWAP_ACK_TARGET_CMPL_CB);
    return mpi_errno;
}

int MPIDIG_put_ack_taget_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                int is_local, int *is_contig,
                                MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_put_ack_msg_t *msg_hdr = (MPIDIG_put_ack_msg_t *) am_hdr;
    MPIR_Win *win;
    MPIR_Request *preq;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_put_ack);

    preq = (MPIR_Request *) msg_hdr->preq_ptr;
    win = MPIDIG_REQUEST(preq, req->preq.win_ptr);

    MPL_free(MPIDIG_REQUEST(preq, req->preq.dt_iov));

    MPIDIG_win_remote_cmpl_cnt_decr(win, MPIDIG_REQUEST(preq, rank));

    MPID_Request_complete(preq);

    if (req)
        *req = NULL;
    if (target_cmpl_cb)
        *target_cmpl_cb = NULL;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_put_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_ACK_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_acc_ack_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                 int is_local, int *is_contig,
                                 MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_acc_ack_msg_t *msg_hdr = (MPIDIG_acc_ack_msg_t *) am_hdr;
    MPIR_Win *win;
    MPIR_Request *areq;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACC_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACC_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_acc_ack);

    areq = (MPIR_Request *) msg_hdr->req_ptr;
    win = MPIDIG_REQUEST(areq, req->areq.win_ptr);

    MPL_free(MPIDIG_REQUEST(areq, req->areq.dt_iov));

    MPIDIG_win_remote_cmpl_cnt_decr(win, MPIDIG_REQUEST(areq, rank));
    MPIDIG_win_remote_acc_cmpl_cnt_decr(win, MPIDIG_REQUEST(areq, rank));

    MPID_Request_complete(areq);

    if (req)
        *req = NULL;
    if (target_cmpl_cb)
        *target_cmpl_cb = NULL;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_acc_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACC_ACK_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_get_acc_ack_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                     int is_local, int *is_contig,
                                     MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_get_acc_ack_msg_t *msg_hdr = (MPIDIG_get_acc_ack_msg_t *) am_hdr;
    MPIR_Request *areq;

    size_t data_sz;
    int dt_contig, n_iov;
    MPI_Aint dt_true_lb, last, num_iov;
    MPIR_Datatype *dt_ptr;
    MPIR_Segment *segment_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_acc_ack);

    areq = (MPIR_Request *) msg_hdr->req_ptr;

    MPL_free(MPIDIG_REQUEST(areq, req->areq.dt_iov));

    MPIDI_Datatype_get_info(MPIDIG_REQUEST(areq, req->areq.result_count),
                            MPIDIG_REQUEST(areq, req->areq.result_datatype),
                            dt_contig, data_sz, dt_ptr, dt_true_lb);
    *is_contig = dt_contig;

    if (dt_contig) {
        *p_data_sz = data_sz;
        *data = (char *) MPIDIG_REQUEST(areq, req->areq.result_addr) + dt_true_lb;
    } else {
        segment_ptr = MPIR_Segment_alloc(MPIDIG_REQUEST(areq, req->areq.result_addr),
                                         MPIDIG_REQUEST(areq, req->areq.result_count),
                                         MPIDIG_REQUEST(areq, req->areq.result_datatype));
        MPIR_Assert(segment_ptr);

        last = data_sz;
        MPIR_Segment_count_contig_blocks(segment_ptr, 0, &last, &num_iov);
        n_iov = (int) num_iov;
        MPIR_Assert(n_iov > 0);
        MPIDIG_REQUEST(areq, req->iov) =
            (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec), MPL_MEM_RMA);
        MPIR_Assert(MPIDIG_REQUEST(areq, req->iov));

        last = data_sz;
        MPIR_Segment_to_iov(segment_ptr, 0, &last, MPIDIG_REQUEST(areq, req->iov), &n_iov);
        MPIR_Assert(last == (MPI_Aint) data_sz);
        *data = MPIDIG_REQUEST(areq, req->iov);
        *p_data_sz = n_iov;
        MPIDIG_REQUEST(areq, req->status) |= MPIDIG_REQ_RCV_NON_CONTIG;
        MPL_free(segment_ptr);
    }

    *req = areq;
    *target_cmpl_cb = get_acc_ack_target_cmpl_cb;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_acc_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_ACK_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_cswap_ack_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                   int is_local, int *is_contig,
                                   MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_cswap_ack_msg_t *msg_hdr = (MPIDIG_cswap_ack_msg_t *) am_hdr;
    MPIR_Request *creq;
    uint64_t data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_CSWAP_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_CSWAP_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_cas_ack);

    creq = (MPIR_Request *) msg_hdr->req_ptr;
    MPIDI_Datatype_check_size(MPIDIG_REQUEST(creq, req->creq.datatype), 1, data_sz);
    *data = MPIDIG_REQUEST(creq, req->creq.result_addr);
    *p_data_sz = data_sz;
    *is_contig = 1;

    *req = creq;
    *target_cmpl_cb = cswap_ack_target_cmpl_cb;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_cas_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_CSWAP_ACK_TARGET_MSG_CB);
    return mpi_errno;
}


int MPIDIG_win_ctrl_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                  int is_local, int *is_contig,
                                  MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_win_cntrl_msg_t *msg_hdr = (MPIDIG_win_cntrl_msg_t *) am_hdr;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_CTRL_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_CTRL_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_win_ctrl);

    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, msg_hdr->win_id);
    /* TODO: check output win ptr */

    switch (handler_id) {
            char buff[32];

        case MPIDIG_WIN_LOCK:
        case MPIDIG_WIN_LOCKALL:
            win_lock_req_proc(handler_id, msg_hdr, win);
            break;

        case MPIDIG_WIN_LOCK_ACK:
        case MPIDIG_WIN_LOCKALL_ACK:
            win_lock_ack_proc(handler_id, msg_hdr, win);
            break;

        case MPIDIG_WIN_UNLOCK:
        case MPIDIG_WIN_UNLOCKALL:
            win_unlock_proc(msg_hdr, is_local, win);
            break;

        case MPIDIG_WIN_UNLOCK_ACK:
        case MPIDIG_WIN_UNLOCKALL_ACK:
            win_unlock_done(msg_hdr, win);
            break;

        case MPIDIG_WIN_COMPLETE:
            win_complete_proc(msg_hdr, win);
            break;

        case MPIDIG_WIN_POST:
            win_post_proc(msg_hdr, win);
            break;

        default:
            MPL_snprintf(buff, sizeof(buff), "Invalid message type: %d\n", handler_id);
            MPID_Abort(NULL, MPI_ERR_INTERN, 1, buff);
    }

    if (req)
        *req = NULL;
    if (target_cmpl_cb)
        *target_cmpl_cb = NULL;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_win_ctrl);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_CTRL_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_put_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                             int is_local, int *is_contig,
                             MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
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
    MPIR_Segment *segment_ptr;
    MPIR_Win *win;
    MPIDIG_put_msg_t *msg_hdr = (MPIDIG_put_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_put);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *req = rreq;

    MPIDIG_REQUEST(*req, req->preq.preq_ptr) = msg_hdr->preq_ptr;
    MPIDIG_REQUEST(*req, rank) = msg_hdr->src_rank;

    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, msg_hdr->win_id);
    MPIR_Assert(win);

    base = MPIDIG_win_base_at_target(win);

    MPIDIG_REQUEST(rreq, req->preq.win_ptr) = win;

    *target_cmpl_cb = put_target_cmpl_cb;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    offset = win->disp_unit * msg_hdr->target_disp;
    if (msg_hdr->n_iov) {
        int i;
        dt_iov = (struct iovec *) MPL_malloc(sizeof(struct iovec) * msg_hdr->n_iov, MPL_MEM_RMA);
        MPIR_Assert(dt_iov);

        iov = (struct iovec *) ((char *) am_hdr + sizeof(*msg_hdr));
        for (i = 0; i < msg_hdr->n_iov; i++) {
            dt_iov[i].iov_base = (char *) iov[i].iov_base + base + offset;
            dt_iov[i].iov_len = iov[i].iov_len;
        }

        MPIDIG_REQUEST(rreq, req->preq.dt_iov) = dt_iov;
        MPIDIG_REQUEST(rreq, req->preq.n_iov) = msg_hdr->n_iov;
        *is_contig = 0;
        *data = dt_iov;
        *p_data_sz = msg_hdr->n_iov;
        goto fn_exit;
    }

    MPIDIG_REQUEST(rreq, req->preq.dt_iov) = NULL;
    MPIDI_Datatype_get_info(msg_hdr->count, msg_hdr->datatype,
                            dt_contig, data_sz, dt_ptr, dt_true_lb);
    *is_contig = dt_contig;

    if (dt_contig) {
        *p_data_sz = data_sz;
        *data = (char *) (offset + base + dt_true_lb);
    } else {
        segment_ptr =
            MPIR_Segment_alloc((void *) (offset + base), msg_hdr->count, msg_hdr->datatype);
        MPIR_Assert(segment_ptr);

        last = data_sz;
        MPIR_Segment_count_contig_blocks(segment_ptr, 0, &last, &num_iov);
        n_iov = (int) num_iov;
        MPIR_Assert(n_iov > 0);
        MPIDIG_REQUEST(rreq, req->iov) =
            (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec), MPL_MEM_RMA);
        MPIR_Assert(MPIDIG_REQUEST(rreq, req->iov));

        last = data_sz;
        MPIR_Segment_to_iov(segment_ptr, 0, &last, MPIDIG_REQUEST(rreq, req->iov), &n_iov);
        MPIR_Assert(last == (MPI_Aint) data_sz);
        *data = MPIDIG_REQUEST(rreq, req->iov);
        *p_data_sz = n_iov;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_RCV_NON_CONTIG;
        MPL_free(segment_ptr);
    }

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_put);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_put_iov_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                 int is_local, int *is_contig,
                                 MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    struct iovec *dt_iov;

    MPIR_Win *win;
    MPIDIG_put_msg_t *msg_hdr = (MPIDIG_put_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_IOV_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_IOV_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_put_iov);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *req = rreq;

    MPIDIG_REQUEST(*req, req->preq.preq_ptr) = msg_hdr->preq_ptr;
    MPIDIG_REQUEST(*req, rank) = msg_hdr->src_rank;

    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, msg_hdr->win_id);
    MPIR_Assert(win);

    MPIDIG_REQUEST(rreq, req->preq.win_ptr) = win;

    *target_cmpl_cb = put_iov_target_cmpl_cb;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    /* Base adjustment for iov will be done after we get the entire iovs,
     * at MPIDIG_put_data_target_msg_cb */
    MPIR_Assert(msg_hdr->n_iov);
    dt_iov = (struct iovec *) MPL_malloc(sizeof(struct iovec) * msg_hdr->n_iov, MPL_MEM_RMA);
    MPIR_Assert(dt_iov);

    MPIDIG_REQUEST(rreq, req->preq.dt_iov) = dt_iov;
    MPIDIG_REQUEST(rreq, req->preq.n_iov) = msg_hdr->n_iov;
    *is_contig = 1;
    *data = dt_iov;
    *p_data_sz = msg_hdr->n_iov * sizeof(struct iovec);

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_put_iov);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_IOV_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_put_iov_ack_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                     int is_local, int *is_contig,
                                     MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq, *origin_req;
    MPIDIG_put_iov_ack_msg_t *msg_hdr = (MPIDIG_put_iov_ack_msg_t *) am_hdr;
    MPIDIG_put_dat_msg_t dat_msg;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_IOV_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_IOV_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_put_iov_ack);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    origin_req = (MPIR_Request *) msg_hdr->origin_preq_ptr;
    dat_msg.preq_ptr = msg_hdr->target_preq_ptr;
    win = MPIDIG_REQUEST(origin_req, req->preq.win_ptr);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (is_local)
        mpi_errno = MPIDI_SHM_am_isend_reply(MPIDIG_win_to_context(win),
                                             MPIDIG_REQUEST(origin_req, rank),
                                             MPIDIG_PUT_DAT_REQ, &dat_msg, sizeof(dat_msg),
                                             MPIDIG_REQUEST(origin_req, req->preq.origin_addr),
                                             MPIDIG_REQUEST(origin_req, req->preq.origin_count),
                                             MPIDIG_REQUEST(origin_req, req->preq.origin_datatype),
                                             rreq);
    else
#endif
    {
        mpi_errno = MPIDI_NM_am_isend_reply(MPIDIG_win_to_context(win),
                                            MPIDIG_REQUEST(origin_req, rank),
                                            MPIDIG_PUT_DAT_REQ, &dat_msg, sizeof(dat_msg),
                                            MPIDIG_REQUEST(origin_req, req->preq.origin_addr),
                                            MPIDIG_REQUEST(origin_req, req->preq.origin_count),
                                            MPIDIG_REQUEST(origin_req, req->preq.origin_datatype),
                                            rreq);
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(origin_req, req->preq.origin_datatype));

    *target_cmpl_cb = NULL;
    *req = NULL;

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_put_iov_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_IOV_ACK_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_acc_iov_ack_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                     int is_local, int *is_contig,
                                     MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq, *origin_req;
    MPIDIG_acc_iov_ack_msg_t *msg_hdr = (MPIDIG_acc_iov_ack_msg_t *) am_hdr;
    MPIDIG_acc_dat_msg_t dat_msg;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACC_IOV_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACC_IOV_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_acc_iov_ack);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    origin_req = (MPIR_Request *) msg_hdr->origin_preq_ptr;
    dat_msg.preq_ptr = msg_hdr->target_preq_ptr;
    win = MPIDIG_REQUEST(origin_req, req->areq.win_ptr);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (is_local)
        mpi_errno = MPIDI_SHM_am_isend_reply(MPIDIG_win_to_context(win),
                                             MPIDIG_REQUEST(origin_req, rank),
                                             MPIDIG_ACC_DAT_REQ,
                                             &dat_msg, sizeof(dat_msg),
                                             MPIDIG_REQUEST(origin_req, req->areq.origin_addr),
                                             MPIDIG_REQUEST(origin_req, req->areq.origin_count),
                                             MPIDIG_REQUEST(origin_req, req->areq.origin_datatype),
                                             rreq);
    else
#endif
    {
        mpi_errno = MPIDI_NM_am_isend_reply(MPIDIG_win_to_context(win),
                                            MPIDIG_REQUEST(origin_req, rank),
                                            MPIDIG_ACC_DAT_REQ,
                                            &dat_msg, sizeof(dat_msg),
                                            MPIDIG_REQUEST(origin_req, req->areq.origin_addr),
                                            MPIDIG_REQUEST(origin_req, req->areq.origin_count),
                                            MPIDIG_REQUEST(origin_req, req->areq.origin_datatype),
                                            rreq);
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(origin_req, req->areq.origin_datatype));

    *target_cmpl_cb = NULL;
    *req = NULL;

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_acc_iov_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACC_IOV_ACK_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_get_acc_iov_ack_target_msg_cb(int handler_id, void *am_hdr, void **data,
                                         size_t * p_data_sz, int is_local, int *is_contig,
                                         MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                         MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq, *origin_req;
    MPIDIG_get_acc_iov_ack_msg_t *msg_hdr = (MPIDIG_get_acc_iov_ack_msg_t *) am_hdr;
    MPIDIG_get_acc_dat_msg_t dat_msg;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_IOV_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_IOV_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_acc_iov_ack);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    origin_req = (MPIR_Request *) msg_hdr->origin_preq_ptr;
    dat_msg.preq_ptr = msg_hdr->target_preq_ptr;
    win = MPIDIG_REQUEST(origin_req, req->areq.win_ptr);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (is_local)
        mpi_errno = MPIDI_SHM_am_isend_reply(MPIDIG_win_to_context(win),
                                             MPIDIG_REQUEST(origin_req, rank),
                                             MPIDIG_GET_ACC_DAT_REQ,
                                             &dat_msg, sizeof(dat_msg),
                                             MPIDIG_REQUEST(origin_req, req->areq.origin_addr),
                                             MPIDIG_REQUEST(origin_req, req->areq.origin_count),
                                             MPIDIG_REQUEST(origin_req, req->areq.origin_datatype),
                                             rreq);
    else
#endif
    {
        mpi_errno = MPIDI_NM_am_isend_reply(MPIDIG_win_to_context(win),
                                            MPIDIG_REQUEST(origin_req, rank),
                                            MPIDIG_GET_ACC_DAT_REQ,
                                            &dat_msg, sizeof(dat_msg),
                                            MPIDIG_REQUEST(origin_req, req->areq.origin_addr),
                                            MPIDIG_REQUEST(origin_req, req->areq.origin_count),
                                            MPIDIG_REQUEST(origin_req, req->areq.origin_datatype),
                                            rreq);
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(origin_req, req->areq.origin_datatype));

    *target_cmpl_cb = NULL;
    *req = NULL;

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_acc_iov_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_IOV_ACK_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_put_data_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                  int is_local, int *is_contig,
                                  MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDIG_put_dat_msg_t *msg_hdr = (MPIDIG_put_dat_msg_t *) am_hdr;
    MPIR_Win *win;
    struct iovec *iov;
    uintptr_t base;
    int i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_DATA_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_DATA_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_put_data);

    rreq = (MPIR_Request *) msg_hdr->preq_ptr;
    win = MPIDIG_REQUEST(rreq, req->preq.win_ptr);
    base = MPIDIG_win_base_at_target(win);

    /* Adjust the target addresses using the window base address */
    iov = (struct iovec *) MPIDIG_REQUEST(rreq, req->preq.dt_iov);
    for (i = 0; i < MPIDIG_REQUEST(rreq, req->preq.n_iov); i++)
        iov[i].iov_base = (char *) iov[i].iov_base + base;

    *data = MPIDIG_REQUEST(rreq, req->preq.dt_iov);
    *is_contig = 0;
    *p_data_sz = MPIDIG_REQUEST(rreq, req->preq.n_iov);
    *req = rreq;
    *target_cmpl_cb = put_target_cmpl_cb;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_put_data);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_DATA_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_acc_data_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                  int is_local, int *is_contig,
                                  MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDIG_acc_dat_msg_t *msg_hdr = (MPIDIG_acc_dat_msg_t *) am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACC_DATA_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACC_DATA_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_acc_data);

    rreq = (MPIR_Request *) msg_hdr->preq_ptr;
    handle_acc_data(data, p_data_sz, is_contig, rreq);

    *req = rreq;
    *target_cmpl_cb = acc_target_cmpl_cb;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_acc_data);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACC_DATA_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_get_acc_data_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                      int is_local, int *is_contig,
                                      MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                      MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDIG_get_acc_dat_msg_t *msg_hdr = (MPIDIG_get_acc_dat_msg_t *) am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_DATA_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_DATA_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_acc_data);

    rreq = (MPIR_Request *) msg_hdr->preq_ptr;
    handle_acc_data(data, p_data_sz, is_contig, rreq);

    *req = rreq;
    *target_cmpl_cb = get_acc_target_cmpl_cb;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_acc_data);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_DATA_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_cswap_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                               int is_local, int *is_contig,
                               MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    size_t data_sz;
    MPIR_Win *win;
    uintptr_t base;
    size_t offset;

    int dt_contig;
    void *p_data;

    MPIDIG_cswap_req_msg_t *msg_hdr = (MPIDIG_cswap_req_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_CSWAP_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_CSWAP_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_cas);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *req = rreq;

    *target_cmpl_cb = cswap_target_cmpl_cb;
    MPIDIG_REQUEST(rreq, req->seq_no) = OPA_fetch_and_add_int(&MPIDI_global.nxt_seq_no, 1);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    MPIDI_Datatype_check_contig_size(msg_hdr->datatype, 1, dt_contig, data_sz);
    *is_contig = dt_contig;

    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, msg_hdr->win_id);
    MPIR_Assert(win);

    base = MPIDIG_win_base_at_target(win);
    offset = win->disp_unit * msg_hdr->target_disp;

    MPIDIG_REQUEST(*req, req->creq.win_ptr) = win;
    MPIDIG_REQUEST(*req, req->creq.creq_ptr) = msg_hdr->req_ptr;
    MPIDIG_REQUEST(*req, req->creq.datatype) = msg_hdr->datatype;
    MPIDIG_REQUEST(*req, req->creq.addr) = offset + base;
    MPIDIG_REQUEST(*req, rank) = msg_hdr->src_rank;

    MPIR_Assert(dt_contig == 1);
    p_data = MPL_malloc(data_sz * 2, MPL_MEM_RMA);
    MPIR_Assert(p_data);

    *p_data_sz = data_sz * 2;
    *data = p_data;
    MPIDIG_REQUEST(*req, req->creq.data) = p_data;

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_cas);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_CSWAP_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_acc_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                             int is_local, int *is_contig,
                             MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
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

    MPIDIG_acc_req_msg_t *msg_hdr = (MPIDIG_acc_req_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACC_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACC_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_acc);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *req = rreq;

    MPIDI_Datatype_check_size(msg_hdr->origin_datatype, msg_hdr->origin_count, data_sz);
    if (data_sz) {
        p_data = MPL_malloc(data_sz, MPL_MEM_RMA);
        MPIR_Assert(p_data);
    }

    *target_cmpl_cb = acc_target_cmpl_cb;
    MPIDIG_REQUEST(rreq, req->seq_no) = OPA_fetch_and_add_int(&MPIDI_global.nxt_seq_no, 1);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    if (is_contig) {
        *is_contig = 1;
        *p_data_sz = data_sz;
        *data = p_data;
    }

    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, msg_hdr->win_id);
    MPIR_Assert(win);

    base = MPIDIG_win_base_at_target(win);
    offset = win->disp_unit * msg_hdr->target_disp;

    MPIDIG_REQUEST(*req, req->areq.win_ptr) = win;
    MPIDIG_REQUEST(*req, req->areq.req_ptr) = msg_hdr->req_ptr;
    MPIDIG_REQUEST(*req, req->areq.origin_datatype) = msg_hdr->origin_datatype;
    MPIDIG_REQUEST(*req, req->areq.target_datatype) = msg_hdr->target_datatype;
    MPIDIG_REQUEST(*req, req->areq.origin_count) = msg_hdr->origin_count;
    MPIDIG_REQUEST(*req, req->areq.target_count) = msg_hdr->target_count;
    MPIDIG_REQUEST(*req, req->areq.target_addr) = (void *) (offset + base);
    MPIDIG_REQUEST(*req, req->areq.op) = msg_hdr->op;
    MPIDIG_REQUEST(*req, req->areq.data) = p_data;
    MPIDIG_REQUEST(*req, req->areq.n_iov) = msg_hdr->n_iov;
    MPIDIG_REQUEST(*req, req->areq.data_sz) = msg_hdr->result_data_sz;
    MPIDIG_REQUEST(*req, rank) = msg_hdr->src_rank;

    if (!msg_hdr->n_iov) {
        MPIDIG_REQUEST(rreq, req->areq.dt_iov) = NULL;
        goto fn_exit;
    }

    dt_iov = (struct iovec *) MPL_malloc(sizeof(struct iovec) * msg_hdr->n_iov, MPL_MEM_RMA);
    MPIR_Assert(dt_iov);

    iov = (struct iovec *) ((char *) msg_hdr + sizeof(*msg_hdr));
    for (i = 0; i < msg_hdr->n_iov; i++) {
        dt_iov[i].iov_base = (char *) iov[i].iov_base + base + offset;
        dt_iov[i].iov_len = iov[i].iov_len;
    }
    MPIDIG_REQUEST(rreq, req->areq.dt_iov) = dt_iov;

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_acc);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACC_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


int MPIDIG_get_acc_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                 int is_local, int *is_contig,
                                 MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_acc);

    /* the same handling processing as ACC except the completion handler function. */
    mpi_errno =
        MPIDIG_acc_target_msg_cb(handler_id, am_hdr, data, p_data_sz, is_local, is_contig,
                                 target_cmpl_cb, req);

    *target_cmpl_cb = get_acc_target_cmpl_cb;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_acc);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_acc_iov_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                 int is_local, int *is_contig,
                                 MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    struct iovec *dt_iov;
    MPIR_Win *win;
    uintptr_t base;
    size_t offset;

    MPIDIG_acc_req_msg_t *msg_hdr = (MPIDIG_acc_req_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACC_IOV_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACC_IOV_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_acc_iov);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *req = rreq;

    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, msg_hdr->win_id);
    MPIR_Assert(win);

    base = MPIDIG_win_base_at_target(win);
    offset = win->disp_unit * msg_hdr->target_disp;

    MPIDIG_REQUEST(*req, req->areq.win_ptr) = win;
    MPIDIG_REQUEST(*req, req->areq.req_ptr) = msg_hdr->req_ptr;
    MPIDIG_REQUEST(*req, req->areq.origin_datatype) = msg_hdr->origin_datatype;
    MPIDIG_REQUEST(*req, req->areq.target_datatype) = msg_hdr->target_datatype;
    MPIDIG_REQUEST(*req, req->areq.origin_count) = msg_hdr->origin_count;
    MPIDIG_REQUEST(*req, req->areq.target_count) = msg_hdr->target_count;
    MPIDIG_REQUEST(*req, req->areq.target_addr) = (void *) (offset + base);
    MPIDIG_REQUEST(*req, req->areq.op) = msg_hdr->op;
    MPIDIG_REQUEST(*req, req->areq.n_iov) = msg_hdr->n_iov;
    MPIDIG_REQUEST(*req, req->areq.data_sz) = msg_hdr->result_data_sz;
    MPIDIG_REQUEST(*req, rank) = msg_hdr->src_rank;

    dt_iov = (struct iovec *) MPL_malloc(sizeof(struct iovec) * msg_hdr->n_iov, MPL_MEM_RMA);
    MPIDIG_REQUEST(rreq, req->areq.dt_iov) = dt_iov;
    MPIR_Assert(dt_iov);

    /* Base adjustment for iov will be done after we get the entire iovs,
     * at MPIDIG_acc_data_target_msg_cb */
    *is_contig = 1;
    *p_data_sz = sizeof(struct iovec) * msg_hdr->n_iov;
    *data = (void *) dt_iov;

    *target_cmpl_cb = acc_iov_target_cmpl_cb;
    MPIDIG_REQUEST(rreq, req->seq_no) = OPA_fetch_and_add_int(&MPIDI_global.nxt_seq_no, 1);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_acc_iov);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACC_IOV_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_get_acc_iov_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                     int is_local, int *is_contig,
                                     MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_IOV_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_IOV_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_acc_iov);

    /* the same handling processing as ACC except the completion handler function. */
    mpi_errno = MPIDIG_acc_iov_target_msg_cb(handler_id, am_hdr, data,
                                             p_data_sz, is_local, is_contig, target_cmpl_cb, req);

    *target_cmpl_cb = get_acc_iov_target_cmpl_cb;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_acc_iov);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_IOV_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_get_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                             int is_local, int *is_contig,
                             MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    MPIDIG_get_msg_t *msg_hdr = (MPIDIG_get_msg_t *) am_hdr;
    struct iovec *iov;
    MPIR_Win *win;
    uintptr_t base;
    size_t offset;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    *req = rreq;
    *target_cmpl_cb = get_target_cmpl_cb;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, msg_hdr->win_id);
    MPIR_Assert(win);

    base = MPIDIG_win_base_at_target(win);

    offset = win->disp_unit * msg_hdr->target_disp;
    MPIDIG_REQUEST(rreq, req->greq.win_ptr) = win;
    MPIDIG_REQUEST(rreq, req->greq.n_iov) = msg_hdr->n_iov;
    MPIDIG_REQUEST(rreq, req->greq.addr) = offset + base;
    MPIDIG_REQUEST(rreq, req->greq.count) = msg_hdr->count;
    MPIDIG_REQUEST(rreq, req->greq.datatype) = msg_hdr->datatype;
    MPIDIG_REQUEST(rreq, req->greq.dt_iov) = NULL;
    MPIDIG_REQUEST(rreq, req->greq.greq_ptr) = msg_hdr->greq_ptr;
    MPIDIG_REQUEST(rreq, rank) = msg_hdr->src_rank;

    if (msg_hdr->n_iov) {
        iov = (struct iovec *) MPL_malloc(msg_hdr->n_iov * sizeof(*iov), MPL_MEM_RMA);
        MPIR_Assert(iov);

        *data = (void *) iov;
        *is_contig = 1;
        *p_data_sz = msg_hdr->n_iov * sizeof(*iov);
        MPIDIG_REQUEST(rreq, req->greq.dt_iov) = iov;
    }

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_get_ack_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                 int is_local, int *is_contig,
                                 MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL, *greq;
    size_t data_sz;

    int dt_contig, n_iov;
    MPI_Aint dt_true_lb, last, num_iov;
    MPIR_Datatype *dt_ptr;
    MPIR_Segment *segment_ptr;

    MPIDIG_get_ack_msg_t *msg_hdr = (MPIDIG_get_ack_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_ack);

    greq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(greq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *req = greq;

    rreq = (MPIR_Request *) msg_hdr->greq_ptr;
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__RMA);
    MPIDIG_REQUEST(greq, req->greq.greq_ptr) = (uint64_t) rreq;

    MPL_free(MPIDIG_REQUEST(rreq, req->greq.dt_iov));

    *target_cmpl_cb = get_ack_target_cmpl_cb;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(greq, is_local) = is_local;
#endif

    MPIDI_Datatype_get_info(MPIDIG_REQUEST(rreq, req->greq.count),
                            MPIDIG_REQUEST(rreq, req->greq.datatype),
                            dt_contig, data_sz, dt_ptr, dt_true_lb);

    *is_contig = dt_contig;

    if (dt_contig) {
        *p_data_sz = data_sz;
        *data = (char *) (MPIDIG_REQUEST(rreq, req->greq.addr) + dt_true_lb);
    } else {
        segment_ptr = MPIR_Segment_alloc((void *) MPIDIG_REQUEST(rreq, req->greq.addr),
                                         MPIDIG_REQUEST(rreq, req->greq.count),
                                         MPIDIG_REQUEST(rreq, req->greq.datatype));
        MPIR_Assert(segment_ptr);

        last = data_sz;
        MPIR_Segment_count_contig_blocks(segment_ptr, 0, &last, &num_iov);
        n_iov = (int) num_iov;
        MPIR_Assert(n_iov > 0);
        MPIDIG_REQUEST(rreq, req->iov) =
            (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec), MPL_MEM_RMA);
        MPIR_Assert(MPIDIG_REQUEST(rreq, req->iov));

        last = data_sz;
        MPIR_Segment_to_iov(segment_ptr, 0, &last, MPIDIG_REQUEST(rreq, req->iov), &n_iov);
        MPIR_Assert(last == (MPI_Aint) data_sz);
        *data = MPIDIG_REQUEST(rreq, req->iov);
        *p_data_sz = n_iov;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_RCV_NON_CONTIG;
        MPL_free(segment_ptr);
    }

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACK_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
