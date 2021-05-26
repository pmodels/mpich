/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
static void handle_acc_data(MPI_Aint in_data_sz, MPIR_Request * rreq);
static int get_target_cmpl_cb(MPIR_Request * req);
static int put_target_cmpl_cb(MPIR_Request * rreq);
static int put_dt_target_cmpl_cb(MPIR_Request * rreq);
static int acc_dt_target_cmpl_cb(MPIR_Request * rreq);
static int get_acc_dt_target_cmpl_cb(MPIR_Request * rreq);
static int cswap_target_cmpl_cb(MPIR_Request * rreq);
static int acc_target_cmpl_cb(MPIR_Request * rreq);
static int get_acc_target_cmpl_cb(MPIR_Request * rreq);
static int get_ack_target_cmpl_cb(MPIR_Request * rreq);
static int get_acc_ack_target_cmpl_cb(MPIR_Request * rreq);
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

    /* rma_targetcb_put_dt */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_put_dt,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for PUT IOV (in seconds)");

    /* rma_targetcb_put_dt_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_put_dt_ack,
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

    /* rma_targetcb_acc_dt */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_acc_dt,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for ACC IOV (in seconds)");

    /* rma_targetcb_get_acc_dt */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_get_acc_dt,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for GET ACC IOV (in seconds)");

    /* rma_targetcb_acc_dt_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_acc_dt_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for ACC IOV ACK (in seconds)");

    /* rma_targetcb_get_acc_dt_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_get_acc_dt_ack,
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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ACK_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_ACK_PUT);

    ack_msg.preq_ptr = MPIDIG_REQUEST(rreq, req->preq.preq_ptr);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(MPIDIG_win_to_context
                                        (MPIDIG_REQUEST(rreq, req->preq.win_ptr)),
                                        MPIDIG_REQUEST(rreq, rank), MPIDIG_PUT_ACK,
                                        &ack_msg, (MPI_Aint) sizeof(ack_msg));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDIG_win_to_context
                                       (MPIDIG_REQUEST(rreq, req->preq.win_ptr)),
                                       MPIDIG_REQUEST(rreq, rank), MPIDIG_PUT_ACK,
                                       &ack_msg, (MPI_Aint) sizeof(ack_msg));
    }

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_ACK_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ack_cswap(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_cswap_ack_msg_t ack_msg;
    void *result_addr;
    MPI_Aint data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ACK_CSWAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_ACK_CSWAP);

    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, req->creq.datatype), 1, data_sz);
    result_addr = ((char *) MPIDIG_REQUEST(rreq, req->creq.data)) + data_sz;

    MPIR_cc_inc(rreq->cc_ptr);
    ack_msg.req_ptr = MPIDIG_REQUEST(rreq, req->creq.creq_ptr);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_isend_reply(MPIDIG_win_to_context
                                     (MPIDIG_REQUEST(rreq, req->creq.win_ptr)),
                                     MPIDIG_REQUEST(rreq, rank), MPIDIG_CSWAP_ACK, &ack_msg,
                                     (MPI_Aint) sizeof(ack_msg), result_addr, 1,
                                     MPIDIG_REQUEST(rreq, req->creq.datatype), rreq);
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_isend_reply(MPIDIG_win_to_context
                                    (MPIDIG_REQUEST(rreq, req->creq.win_ptr)),
                                    MPIDIG_REQUEST(rreq, rank), MPIDIG_CSWAP_ACK, &ack_msg,
                                    (MPI_Aint) sizeof(ack_msg), result_addr, 1,
                                    MPIDIG_REQUEST(rreq, req->creq.datatype), rreq);
    }

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_ACK_CSWAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ack_acc(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_acc_ack_msg_t ack_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ACK_ACC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_ACK_ACC);

    ack_msg.req_ptr = MPIDIG_REQUEST(rreq, req->areq.req_ptr);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(MPIDIG_win_to_context
                                        (MPIDIG_REQUEST(rreq, req->areq.win_ptr)),
                                        MPIDIG_REQUEST(rreq, rank), MPIDIG_ACC_ACK,
                                        &ack_msg, (MPI_Aint) sizeof(ack_msg));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDIG_win_to_context
                                       (MPIDIG_REQUEST(rreq, req->areq.win_ptr)),
                                       MPIDIG_REQUEST(rreq, rank), MPIDIG_ACC_ACK,
                                       &ack_msg, (MPI_Aint) sizeof(ack_msg));
    }

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_ACK_ACC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ack_get_acc(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_acc_ack_msg_t ack_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ACK_GET_ACC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_ACK_GET_ACC);

    MPIR_cc_inc(rreq->cc_ptr);
    ack_msg.req_ptr = MPIDIG_REQUEST(rreq, req->areq.req_ptr);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_isend_reply(MPIDIG_win_to_context
                                     (MPIDIG_REQUEST(rreq, req->areq.win_ptr)),
                                     MPIDIG_REQUEST(rreq, rank), MPIDIG_GET_ACC_ACK,
                                     &ack_msg, (MPI_Aint) sizeof(ack_msg),
                                     MPIDIG_REQUEST(rreq, req->areq.data),
                                     MPIDIG_REQUEST(rreq, req->areq.result_data_sz), MPI_BYTE,
                                     rreq);
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_isend_reply(MPIDIG_win_to_context
                                    (MPIDIG_REQUEST(rreq, req->areq.win_ptr)),
                                    MPIDIG_REQUEST(rreq, rank), MPIDIG_GET_ACC_ACK,
                                    &ack_msg, (MPI_Aint) sizeof(ack_msg),
                                    MPIDIG_REQUEST(rreq, req->areq.data),
                                    MPIDIG_REQUEST(rreq, req->areq.result_data_sz), MPI_BYTE, rreq);
    }

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_ACK_GET_ACC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static int win_lock_advance(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_win_lock_recvd_t *lock_recvd_q = &MPIDIG_WIN(win, sync).lock_recvd;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_WIN_LOCK_ADVANCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_WIN_LOCK_ADVANCE);

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
                                                    lock->rank, handler_id,
                                                    &msg, (MPI_Aint) sizeof(msg));
        else
#endif
        {
            mpi_errno = MPIDI_NM_am_send_hdr_reply(MPIDIG_win_to_context(win),
                                                   lock->rank, handler_id,
                                                   &msg, (MPI_Aint) sizeof(msg));
        }

        MPIR_ERR_CHECK(mpi_errno);
        MPL_free(lock);

        mpi_errno = win_lock_advance(win);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_WIN_LOCK_ADVANCE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void win_lock_req_proc(int handler_id, const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_WIN_LOCK_REQ_PROC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_WIN_LOCK_REQ_PROC);

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_WIN_LOCK_REQ_PROC);
    return;
}

static void win_lock_ack_proc(int handler_id, const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_WIN_LOCK_ACK_PROC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_WIN_LOCK_ACK_PROC);

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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_WIN_LOCK_ACK_PROC);
}

static void win_unlock_proc(const MPIDIG_win_cntrl_msg_t * info, int is_local, MPIR_Win * win)
{

    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_WIN_UNLOCK_PROC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_WIN_UNLOCK_PROC);

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
                                                MPIDIG_WIN_UNLOCK_ACK,
                                                &msg, (MPI_Aint) sizeof(msg));
    else
#endif
    {
        mpi_errno = MPIDI_NM_am_send_hdr_reply(MPIDIG_win_to_context(win),
                                               info->origin_rank,
                                               MPIDIG_WIN_UNLOCK_ACK, &msg, (MPI_Aint) sizeof(msg));
    }

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_WIN_UNLOCK_PROC);
    return;
  fn_fail:
    goto fn_exit;
}

static void win_complete_proc(const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_WIN_COMPLETE_PROC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_WIN_COMPLETE_PROC);

    ++MPIDIG_WIN(win, sync).sc.count;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_WIN_COMPLETE_PROC);
}

static void win_post_proc(const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_WIN_POST_PROC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_WIN_POST_PROC);

    ++MPIDIG_WIN(win, sync).pw.count;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_WIN_POST_PROC);
}


static void win_unlock_done(const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_WIN_UNLOCK_DONE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_WIN_UNLOCK_DONE);

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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_WIN_UNLOCK_DONE);
}

static int handle_acc_cmpl(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    int shm_locked ATTRIBUTE((unused)) = 0;
    MPIR_Win *win ATTRIBUTE((unused)) = MPIDIG_REQUEST(rreq, req->areq.win_ptr);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_HANDLE_ACC_CMPL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_HANDLE_ACC_CMPL);

    /* MPIDI_CS_ENTER(); */

    if (MPIDIG_REQUEST(rreq, req->areq.op) == MPI_NO_OP) {
        MPIDIG_REQUEST(rreq, req->areq.origin_count) = MPIDIG_REQUEST(rreq, req->areq.target_count);
    }
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_WIN(win, winattr) & MPIDI_WINATTR_SHM_ALLOCATED) {
        mpi_errno = MPIDI_SHM_rma_op_cs_enter_hook(win);
        MPIR_ERR_CHECK(mpi_errno);

        shm_locked = 1;
    }
#endif

    mpi_errno = MPIDIG_compute_acc_op(MPIDIG_REQUEST(rreq, req->areq.data),
                                      MPIDIG_REQUEST(rreq, req->areq.origin_count),
                                      MPIDIG_REQUEST(rreq, req->areq.origin_datatype),
                                      MPIDIG_REQUEST(rreq, req->areq.target_addr),
                                      MPIDIG_REQUEST(rreq, req->areq.target_count),
                                      MPIDIG_REQUEST(rreq, req->areq.target_datatype),
                                      MPIDIG_REQUEST(rreq, req->areq.op), MPIDIG_ACC_SRCBUF_PACKED);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, req->areq.target_datatype));
    MPL_free(MPIDIG_REQUEST(rreq, req->areq.flattened_dt));

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_WIN(win, winattr) & MPIDI_WINATTR_SHM_ALLOCATED) {
        mpi_errno = MPIDI_SHM_rma_op_cs_exit_hook(win);
        MPIR_ERR_CHECK(mpi_errno);
    }
#endif

    /* MPIDI_CS_EXIT(); */
    MPL_free(MPIDIG_REQUEST(rreq, req->areq.data));

    MPIDIG_REQUEST(rreq, req->areq.data) = NULL;
    mpi_errno = ack_acc(rreq);
    MPIR_ERR_CHECK(mpi_errno);

    MPID_Request_complete(rreq);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_HANDLE_ACC_CMPL);
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
    int mpi_errno = MPI_SUCCESS;
    char *original = NULL;
    MPI_Aint result_data_sz;
    int shm_locked ATTRIBUTE((unused)) = 0;
    MPIR_Win *win ATTRIBUTE((unused)) = MPIDIG_REQUEST(rreq, req->areq.win_ptr);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_HANDLE_GET_ACC_CMPL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_HANDLE_GET_ACC_CMPL);

    result_data_sz = MPIDIG_REQUEST(rreq, req->areq.result_data_sz);

    /* MPIDI_CS_ENTER(); */

    original = (char *) MPL_malloc(result_data_sz, MPL_MEM_RMA);
    MPIR_Assert(original);

    if (MPIDIG_REQUEST(rreq, req->areq.op) == MPI_NO_OP) {
        MPIDIG_REQUEST(rreq, req->areq.origin_count) = MPIDIG_REQUEST(rreq, req->areq.target_count);
    }
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_WIN(win, winattr) & MPIDI_WINATTR_SHM_ALLOCATED) {
        mpi_errno = MPIDI_SHM_rma_op_cs_enter_hook(win);
        MPIR_ERR_CHECK(mpi_errno);
        shm_locked = 1;
    }
#endif

    MPI_Aint actual_pack_bytes;
    MPIR_Typerep_pack(MPIDIG_REQUEST(rreq, req->areq.target_addr),
                      MPIDIG_REQUEST(rreq, req->areq.target_count), MPIDIG_REQUEST(rreq,
                                                                                   req->
                                                                                   areq.target_datatype),
                      0, original, result_data_sz, &actual_pack_bytes);

    mpi_errno = MPIDIG_compute_acc_op(MPIDIG_REQUEST(rreq, req->areq.data),
                                      MPIDIG_REQUEST(rreq, req->areq.origin_count),
                                      MPIDIG_REQUEST(rreq, req->areq.origin_datatype),
                                      MPIDIG_REQUEST(rreq, req->areq.target_addr),
                                      MPIDIG_REQUEST(rreq, req->areq.target_count),
                                      MPIDIG_REQUEST(rreq, req->areq.target_datatype),
                                      MPIDIG_REQUEST(rreq, req->areq.op), MPIDIG_ACC_SRCBUF_PACKED);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, req->areq.target_datatype));
    MPL_free(MPIDIG_REQUEST(rreq, req->areq.flattened_dt));

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_WIN(win, winattr) & MPIDI_WINATTR_SHM_ALLOCATED) {
        mpi_errno = MPIDI_SHM_rma_op_cs_exit_hook(win);
        MPIR_ERR_CHECK(mpi_errno);
    }
#endif

    /* MPIDI_CS_EXIT(); */
    MPL_free(MPIDIG_REQUEST(rreq, req->areq.data));

    MPIDIG_REQUEST(rreq, req->areq.data) = original;
    mpi_errno = ack_get_acc(rreq);
    MPIR_ERR_CHECK(mpi_errno);

    MPID_Request_complete(rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_HANDLE_GET_ACC_CMPL);
    return mpi_errno;
  fn_fail:
#ifndef MPIDI_CH4_DIRECT_NETMOD
    /* release lock if error is reported inside CS. */
    if (shm_locked)
        MPIDI_SHM_rma_op_cs_exit_hook(win);
#endif
    goto fn_exit;
}

static void handle_acc_data(MPI_Aint in_data_sz, MPIR_Request * rreq)
{
    void *p_data = NULL;
    MPI_Aint origin_data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_HANDLE_ACC_DATA);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_HANDLE_ACC_DATA);

    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, req->areq.origin_datatype),
                              MPIDIG_REQUEST(rreq, req->areq.origin_count), origin_data_sz);

    /* The origin_data can be NULL only with no-op.
     * TODO: when no-op is set, we do not need send origin_data at all. */
    if (origin_data_sz) {
        p_data = MPL_malloc(origin_data_sz, MPL_MEM_RMA);
        MPIR_Assert(p_data);
    } else {
        MPIR_Assert(MPIDIG_REQUEST(rreq, req->areq.op) == MPI_NO_OP);
    }

    MPIDIG_REQUEST(rreq, req->areq.data) = p_data;

    if (MPIDIG_REQUEST(rreq, req->areq.flattened_dt)) {
        /* FIXME: MPIR_Typerep_unflatten should allocate the new object */
        MPIR_Datatype *dt = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
        MPIR_Assert(dt);
        MPIR_Object_set_ref(dt, 1);
        MPIR_Typerep_unflatten(dt, MPIDIG_REQUEST(rreq, req->areq.flattened_dt));
        MPIDIG_REQUEST(rreq, req->areq.target_datatype) = dt->handle;
    }

    MPIDIG_recv_init(1, origin_data_sz, p_data, origin_data_sz, rreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_HANDLE_ACC_DATA);
}

static int get_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_get_ack_msg_t get_ack;
    MPIR_Win *win;
    MPIR_Context_id_t context_id;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_GET_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_GET_TARGET_CMPL_CB);

    MPIR_cc_inc(rreq->cc_ptr);
    get_ack.greq_ptr = MPIDIG_REQUEST(rreq, req->greq.greq_ptr);
    win = MPIDIG_REQUEST(rreq, req->greq.win_ptr);
    context_id = MPIDIG_win_to_context(win);

    if (MPIDIG_REQUEST(rreq, req->greq.flattened_dt) == NULL) {
        MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, req->greq.datatype),
                                  MPIDIG_REQUEST(rreq, req->greq.count), get_ack.target_data_sz);
#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (MPIDI_REQUEST(rreq, is_local))
            mpi_errno = MPIDI_SHM_am_isend_reply(context_id, MPIDIG_REQUEST(rreq, rank),
                                                 MPIDIG_GET_ACK,
                                                 &get_ack, (MPI_Aint) sizeof(get_ack),
                                                 (void *) MPIDIG_REQUEST(rreq, req->greq.addr),
                                                 MPIDIG_REQUEST(rreq, req->greq.count),
                                                 MPIDIG_REQUEST(rreq, req->greq.datatype), rreq);
        else
#endif
        {
            mpi_errno = MPIDI_NM_am_isend_reply(context_id, MPIDIG_REQUEST(rreq, rank),
                                                MPIDIG_GET_ACK,
                                                &get_ack, (MPI_Aint) sizeof(get_ack),
                                                (void *) MPIDIG_REQUEST(rreq, req->greq.addr),
                                                MPIDIG_REQUEST(rreq, req->greq.count),
                                                MPIDIG_REQUEST(rreq, req->greq.datatype), rreq);
        }

        MPID_Request_complete(rreq);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    /* FIXME: MPIR_Typerep_unflatten should allocate the new object */
    MPIR_Datatype *dt = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
    if (!dt) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "MPIR_Datatype_mem");
    }
    MPIR_Object_set_ref(dt, 1);
    MPIR_Typerep_unflatten(dt, MPIDIG_REQUEST(rreq, req->greq.flattened_dt));
    MPIDIG_REQUEST(rreq, req->greq.dt) = dt;
    /* count is still target_data_sz now, use it for reply */
    get_ack.target_data_sz = MPIDIG_REQUEST(rreq, req->greq.count);
    MPIDIG_REQUEST(rreq, req->greq.count) /= dt->size;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno = MPIDI_SHM_am_isend_reply(context_id, MPIDIG_REQUEST(rreq, rank),
                                             MPIDIG_GET_ACK, &get_ack, (MPI_Aint) sizeof(get_ack),
                                             MPIDIG_REQUEST(rreq, req->greq.addr),
                                             MPIDIG_REQUEST(rreq, req->greq.count), dt->handle,
                                             rreq);
    else
#endif
    {
        mpi_errno = MPIDI_NM_am_isend_reply(context_id, MPIDIG_REQUEST(rreq, rank),
                                            MPIDIG_GET_ACK, &get_ack, (MPI_Aint) sizeof(get_ack),
                                            MPIDIG_REQUEST(rreq, req->greq.addr),
                                            MPIDIG_REQUEST(rreq, req->greq.count), dt->handle,
                                            rreq);
    }

    MPID_Request_complete(rreq);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_GET_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int put_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PUT_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PUT_TARGET_CMPL_CB);

    MPIDIG_recv_finish(rreq);

    MPL_free(MPIDIG_REQUEST(rreq, req->preq.flattened_dt));
    if (MPIDIG_REQUEST(rreq, req->preq.dt))
        MPIR_Datatype_ptr_release(MPIDIG_REQUEST(rreq, req->preq.dt));

    mpi_errno = ack_put(rreq);
    MPIR_ERR_CHECK(mpi_errno);

    MPID_Request_complete(rreq);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PUT_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int put_dt_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_put_dt_ack_msg_t ack_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PUT_DT_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PUT_DT_TARGET_CMPL_CB);

    ack_msg.src_rank = MPIDIG_REQUEST(rreq, rank);
    ack_msg.origin_preq_ptr = MPIDIG_REQUEST(rreq, req->preq.preq_ptr);
    ack_msg.target_preq_ptr = rreq;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(MPIDIG_win_to_context
                                        (MPIDIG_REQUEST(rreq, req->preq.win_ptr)),
                                        MPIDIG_REQUEST(rreq, rank), MPIDIG_PUT_DT_ACK,
                                        &ack_msg, (MPI_Aint) sizeof(ack_msg));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDIG_win_to_context
                                       (MPIDIG_REQUEST(rreq, req->preq.win_ptr)),
                                       MPIDIG_REQUEST(rreq, rank), MPIDIG_PUT_DT_ACK,
                                       &ack_msg, (MPI_Aint) sizeof(ack_msg));
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PUT_DT_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int acc_dt_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_acc_dt_ack_msg_t ack_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ACC_DT_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_ACC_DT_TARGET_CMPL_CB);

    ack_msg.origin_preq_ptr = MPIDIG_REQUEST(rreq, req->areq.req_ptr);
    ack_msg.target_preq_ptr = rreq;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(MPIDIG_win_to_context
                                        (MPIDIG_REQUEST(rreq, req->areq.win_ptr)),
                                        MPIDIG_REQUEST(rreq, rank), MPIDIG_ACC_DT_ACK,
                                        &ack_msg, (MPI_Aint) sizeof(ack_msg));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDIG_win_to_context
                                       (MPIDIG_REQUEST(rreq, req->areq.win_ptr)),
                                       MPIDIG_REQUEST(rreq, rank), MPIDIG_ACC_DT_ACK,
                                       &ack_msg, (MPI_Aint) sizeof(ack_msg));
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_ACC_DT_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_acc_dt_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_get_acc_dt_ack_msg_t ack_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_GET_ACC_DT_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_GET_ACC_DT_TARGET_CMPL_CB);

    ack_msg.origin_preq_ptr = MPIDIG_REQUEST(rreq, req->areq.req_ptr);
    ack_msg.target_preq_ptr = rreq;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(MPIDIG_win_to_context
                                        (MPIDIG_REQUEST(rreq, req->areq.win_ptr)),
                                        MPIDIG_REQUEST(rreq, rank), MPIDIG_GET_ACC_DT_ACK,
                                        &ack_msg, (MPI_Aint) sizeof(ack_msg));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDIG_win_to_context
                                       (MPIDIG_REQUEST(rreq, req->areq.win_ptr)),
                                       MPIDIG_REQUEST(rreq, rank), MPIDIG_GET_ACC_DT_ACK,
                                       &ack_msg, (MPI_Aint) sizeof(ack_msg));
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_GET_ACC_DT_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int cswap_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    void *compare_addr;
    void *origin_addr;
    MPI_Aint data_sz;
    MPIR_Win *win ATTRIBUTE((unused)) = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CSWAP_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CSWAP_TARGET_CMPL_CB);

    if (!MPIDIG_check_cmpl_order(rreq))
        return mpi_errno;

    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, req->creq.datatype), 1, data_sz);
    origin_addr = MPIDIG_REQUEST(rreq, req->creq.data);
    compare_addr = ((char *) MPIDIG_REQUEST(rreq, req->creq.data)) + data_sz;

    /* MPIDI_CS_ENTER(); */
#ifndef MPIDI_CH4_DIRECT_NETMOD
    win = MPIDIG_REQUEST(rreq, req->creq.win_ptr);
    if (MPIDI_WIN(win, winattr) & MPIDI_WINATTR_SHM_ALLOCATED) {
        mpi_errno = MPIDI_SHM_rma_op_cs_enter_hook(win);
        MPIR_ERR_CHECK(mpi_errno);
    }
#endif

    if (MPIR_Compare_equal((void *) MPIDIG_REQUEST(rreq, req->creq.addr), compare_addr,
                           MPIDIG_REQUEST(rreq, req->creq.datatype))) {
        MPIR_Typerep_copy(compare_addr, (void *) MPIDIG_REQUEST(rreq, req->creq.addr), data_sz);
        MPIR_Typerep_copy((void *) MPIDIG_REQUEST(rreq, req->creq.addr), origin_addr, data_sz);
    } else {
        MPIR_Typerep_copy(compare_addr, (void *) MPIDIG_REQUEST(rreq, req->creq.addr), data_sz);
    }

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_WIN(win, winattr) & MPIDI_WINATTR_SHM_ALLOCATED) {
        mpi_errno = MPIDI_SHM_rma_op_cs_exit_hook(win);
        MPIR_ERR_CHECK(mpi_errno);
    }
#endif
    /* MPIDI_CS_EXIT(); */

    mpi_errno = ack_cswap(rreq);
    MPIR_ERR_CHECK(mpi_errno);
    MPID_Request_complete(rreq);
    MPIDIG_progress_compl_list();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CSWAP_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}



static int acc_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ACC_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_ACC_TARGET_CMPL_CB);

    if (!MPIDIG_check_cmpl_order(rreq))
        return mpi_errno;

    mpi_errno = handle_acc_cmpl(rreq);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDIG_progress_compl_list();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_ACC_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_acc_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_GET_ACC_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_GET_ACC_TARGET_CMPL_CB);

    if (!MPIDIG_check_cmpl_order(rreq))
        return mpi_errno;

    mpi_errno = handle_get_acc_cmpl(rreq);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDIG_progress_compl_list();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_GET_ACC_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_ack_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_GET_ACK_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_GET_ACK_TARGET_CMPL_CB);

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, req->greq.target_datatype));

    win = MPIDIG_REQUEST(rreq, req->greq.win_ptr);
    MPIDIG_win_remote_cmpl_cnt_decr(win, MPIDIG_REQUEST(rreq, rank));

    MPIDIG_recv_finish(rreq);

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, req->greq.datatype));
    MPID_Request_complete(rreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_GET_ACK_TARGET_CMPL_CB);
    return mpi_errno;
}


static int get_acc_ack_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_GET_ACC_ACK_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_GET_ACC_ACK_TARGET_CMPL_CB);

    MPIDIG_recv_finish(rreq);

    win = MPIDIG_REQUEST(rreq, req->areq.win_ptr);
    MPIDIG_win_remote_cmpl_cnt_decr(win, MPIDIG_REQUEST(rreq, rank));
    MPIDIG_win_remote_acc_cmpl_cnt_decr(win, MPIDIG_REQUEST(rreq, rank));

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, req->areq.result_datatype));
    MPID_Request_complete(rreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_GET_ACC_ACK_TARGET_CMPL_CB);
    return mpi_errno;
}

static int cswap_ack_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CSWAP_ACK_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CSWAP_ACK_TARGET_CMPL_CB);

    win = MPIDIG_REQUEST(rreq, req->creq.win_ptr);
    MPIDIG_win_remote_cmpl_cnt_decr(win, MPIDIG_REQUEST(rreq, rank));
    MPIDIG_win_remote_acc_cmpl_cnt_decr(win, MPIDIG_REQUEST(rreq, rank));

    MPL_free(MPIDIG_REQUEST(rreq, req->creq.data));
    MPID_Request_complete(rreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CSWAP_ACK_TARGET_CMPL_CB);
    return mpi_errno;
}

int MPIDIG_put_ack_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                 int is_local, int is_async, MPIR_Request ** req)
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

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(preq, req->preq.target_datatype));

    MPIDIG_win_remote_cmpl_cnt_decr(win, MPIDIG_REQUEST(preq, rank));

    MPID_Request_complete(preq);

    if (is_async)
        *req = NULL;

    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_put_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_ACK_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_acc_ack_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                 int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_acc_ack_msg_t *msg_hdr = (MPIDIG_acc_ack_msg_t *) am_hdr;
    MPIR_Win *win;
    MPIR_Request *rreq;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACC_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACC_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_acc_ack);

    rreq = (MPIR_Request *) msg_hdr->req_ptr;
    win = MPIDIG_REQUEST(rreq, req->areq.win_ptr);

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, req->areq.target_datatype));

    MPIDIG_win_remote_cmpl_cnt_decr(win, MPIDIG_REQUEST(rreq, rank));
    MPIDIG_win_remote_acc_cmpl_cnt_decr(win, MPIDIG_REQUEST(rreq, rank));

    MPID_Request_complete(rreq);

    if (is_async)
        *req = NULL;

    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_acc_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACC_ACK_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_get_acc_ack_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                     int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_get_acc_ack_msg_t *msg_hdr = (MPIDIG_get_acc_ack_msg_t *) am_hdr;
    MPIR_Request *rreq;
    MPI_Aint result_data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_acc_ack);

    rreq = (MPIR_Request *) msg_hdr->req_ptr;

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, req->areq.target_datatype));

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = get_acc_ack_target_cmpl_cb;

    MPIDIG_REQUEST(rreq, buffer) = MPIDIG_REQUEST(rreq, req->areq.result_addr);
    MPIDIG_REQUEST(rreq, count) = MPIDIG_REQUEST(rreq, req->areq.result_count);
    MPIDIG_REQUEST(rreq, datatype) = MPIDIG_REQUEST(rreq, req->areq.result_datatype);
    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, req->areq.result_datatype),
                              MPIDIG_REQUEST(rreq, req->areq.result_count), result_data_sz);

    MPIDIG_recv_type_init(result_data_sz, rreq);

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_acc_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_ACK_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_cswap_ack_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                   int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_cswap_ack_msg_t *msg_hdr = (MPIDIG_cswap_ack_msg_t *) am_hdr;
    MPIR_Request *rreq;
    MPI_Aint data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_CSWAP_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_CSWAP_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_cas_ack);

    rreq = (MPIR_Request *) msg_hdr->req_ptr;
    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, req->creq.datatype), 1, data_sz);
    void *result_addr = MPIDIG_REQUEST(rreq, req->creq.result_addr);

    MPIDIG_recv_init(1, data_sz, result_addr, data_sz, rreq);

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = cswap_ack_target_cmpl_cb;

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_cas_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_CSWAP_ACK_TARGET_MSG_CB);
    return mpi_errno;
}


int MPIDIG_win_ctrl_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                  int is_local, int is_async, MPIR_Request ** req)
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

    if (is_async)
        *req = NULL;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_win_ctrl);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_CTRL_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_put_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                             int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    uintptr_t base;             /* Base address of the window */
    size_t offset;

    MPIR_Win *win;
    MPIDIG_put_msg_t *msg_hdr = (MPIDIG_put_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_put);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    MPIDIG_REQUEST(rreq, req->preq.preq_ptr) = msg_hdr->preq_ptr;
    MPIDIG_REQUEST(rreq, rank) = msg_hdr->src_rank;

    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, msg_hdr->win_id);
    MPIR_Assert(win);

    base = MPIDIG_win_base_at_target(win);

    MPIDIG_REQUEST(rreq, req->preq.win_ptr) = win;

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = put_target_cmpl_cb;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    offset = win->disp_unit * msg_hdr->target_disp;
    if (msg_hdr->flattened_sz) {
        /* FIXME: MPIR_Typerep_unflatten should allocate the new object */
        MPIR_Datatype *dt = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
        if (!dt) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                                 "MPIR_Datatype_mem");
        }
        MPIR_Object_set_ref(dt, 1);
        MPIR_Typerep_unflatten(dt, (char *) am_hdr + sizeof(*msg_hdr));
        MPIDIG_REQUEST(rreq, req->preq.flattened_dt) = NULL;
        MPIDIG_REQUEST(rreq, req->preq.dt) = dt;

        MPIDIG_REQUEST(rreq, buffer) = (void *) (base + offset);
        MPIDIG_REQUEST(rreq, datatype) = dt->handle;
        MPIDIG_REQUEST(rreq, count) = msg_hdr->target_count;
        MPIDIG_recv_type_init(msg_hdr->origin_data_sz, rreq);
    } else {
        MPIDIG_REQUEST(rreq, req->preq.flattened_dt) = NULL;
        MPIDIG_REQUEST(rreq, req->preq.dt) = NULL;

        MPIDIG_REQUEST(rreq, buffer) = (void *) (base + offset + msg_hdr->target_true_lb);
        MPIDIG_REQUEST(rreq, count) = msg_hdr->target_count;
        MPIDIG_REQUEST(rreq, datatype) = msg_hdr->target_datatype;
        MPIDIG_recv_type_init(msg_hdr->origin_data_sz, rreq);
    }

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

  fn_exit:
    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_put);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_put_dt_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    uintptr_t base;
    size_t offset;

    MPIR_Win *win;
    MPIDIG_put_msg_t *msg_hdr = (MPIDIG_put_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_DT_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_DT_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_put_dt);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    MPIDIG_REQUEST(rreq, req->preq.preq_ptr) = msg_hdr->preq_ptr;
    MPIDIG_REQUEST(rreq, rank) = msg_hdr->src_rank;

    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, msg_hdr->win_id);
    MPIR_Assert(win);

    MPIDIG_REQUEST(rreq, req->preq.win_ptr) = win;

    offset = win->disp_unit * msg_hdr->target_disp;
    base = MPIDIG_win_base_at_target(win);
    MPIDIG_REQUEST(rreq, buffer) = (void *) (offset + base);
    MPIDIG_REQUEST(rreq, count) = msg_hdr->target_count;

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = put_dt_target_cmpl_cb;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    MPIR_Assert(msg_hdr->flattened_sz);
    void *flattened_dt = MPL_malloc(msg_hdr->flattened_sz, MPL_MEM_BUFFER);
    MPIDIG_recv_init(1, msg_hdr->flattened_sz, flattened_dt, msg_hdr->flattened_sz, rreq);
    MPIDIG_REQUEST(rreq, req->preq.flattened_dt) = flattened_dt;
    MPIDIG_REQUEST(rreq, req->preq.origin_data_sz) = msg_hdr->origin_data_sz;

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

  fn_exit:
    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_put_dt);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_DT_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_put_dt_ack_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                    int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq, *origin_req;
    MPIDIG_put_dt_ack_msg_t *msg_hdr = (MPIDIG_put_dt_ack_msg_t *) am_hdr;
    MPIDIG_put_dat_msg_t dat_msg;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_DT_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_DT_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_put_dt_ack);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    origin_req = (MPIR_Request *) msg_hdr->origin_preq_ptr;
    dat_msg.preq_ptr = msg_hdr->target_preq_ptr;
    win = MPIDIG_REQUEST(origin_req, req->preq.win_ptr);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (is_local)
        mpi_errno = MPIDI_SHM_am_isend_reply(MPIDIG_win_to_context(win),
                                             MPIDIG_REQUEST(origin_req, rank),
                                             MPIDIG_PUT_DAT_REQ,
                                             &dat_msg, (MPI_Aint) sizeof(dat_msg),
                                             MPIDIG_REQUEST(origin_req, req->preq.origin_addr),
                                             MPIDIG_REQUEST(origin_req, req->preq.origin_count),
                                             MPIDIG_REQUEST(origin_req, req->preq.origin_datatype),
                                             rreq);
    else
#endif
    {
        mpi_errno = MPIDI_NM_am_isend_reply(MPIDIG_win_to_context(win),
                                            MPIDIG_REQUEST(origin_req, rank),
                                            MPIDIG_PUT_DAT_REQ,
                                            &dat_msg, (MPI_Aint) sizeof(dat_msg),
                                            MPIDIG_REQUEST(origin_req, req->preq.origin_addr),
                                            MPIDIG_REQUEST(origin_req, req->preq.origin_count),
                                            MPIDIG_REQUEST(origin_req, req->preq.origin_datatype),
                                            rreq);
    }

    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(origin_req, req->preq.origin_datatype));

    if (is_async)
        *req = NULL;

  fn_exit:
    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_put_dt_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_DT_ACK_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_acc_dt_ack_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                    int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq, *origin_req;
    MPIDIG_acc_dt_ack_msg_t *msg_hdr = (MPIDIG_acc_dt_ack_msg_t *) am_hdr;
    MPIDIG_acc_dat_msg_t dat_msg;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACC_DT_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACC_DT_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_acc_dt_ack);

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
                                             &dat_msg, (MPI_Aint) sizeof(dat_msg),
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
                                            &dat_msg, (MPI_Aint) sizeof(dat_msg),
                                            MPIDIG_REQUEST(origin_req, req->areq.origin_addr),
                                            MPIDIG_REQUEST(origin_req, req->areq.origin_count),
                                            MPIDIG_REQUEST(origin_req, req->areq.origin_datatype),
                                            rreq);
    }

    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(origin_req, req->areq.origin_datatype));

    if (is_async)
        *req = NULL;

  fn_exit:
    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_acc_dt_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACC_DT_ACK_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_get_acc_dt_ack_target_msg_cb(int handler_id, void *am_hdr, void *data,
                                        MPI_Aint in_data_sz, int is_local, int is_async,
                                        MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq, *origin_req;
    MPIDIG_get_acc_dt_ack_msg_t *msg_hdr = (MPIDIG_get_acc_dt_ack_msg_t *) am_hdr;
    MPIDIG_get_acc_dat_msg_t dat_msg;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_DT_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_DT_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_acc_dt_ack);

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
                                             &dat_msg, (MPI_Aint) sizeof(dat_msg),
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
                                            &dat_msg, (MPI_Aint) sizeof(dat_msg),
                                            MPIDIG_REQUEST(origin_req, req->areq.origin_addr),
                                            MPIDIG_REQUEST(origin_req, req->areq.origin_count),
                                            MPIDIG_REQUEST(origin_req, req->areq.origin_datatype),
                                            rreq);
    }

    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(origin_req, req->areq.origin_datatype));

    if (is_async)
        *req = NULL;

    MPIDIG_rma_set_am_flag();

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_acc_dt_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_DT_ACK_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_put_data_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                  int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDIG_put_dat_msg_t *msg_hdr = (MPIDIG_put_dat_msg_t *) am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_DATA_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_DATA_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_put_data);

    rreq = (MPIR_Request *) msg_hdr->preq_ptr;

    /* FIXME: MPIR_Typerep_unflatten should allocate the new object */
    MPIR_Datatype *dt = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
    if (!dt) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "MPIR_Datatype_mem");
    }
    /* Note: handle is filled in by MPIR_Handle_obj_alloc() */
    MPIR_Object_set_ref(dt, 1);
    MPIR_Typerep_unflatten(dt, MPIDIG_REQUEST(rreq, req->preq.flattened_dt));
    MPIDIG_REQUEST(rreq, req->preq.dt) = dt;
    MPIDIG_REQUEST(rreq, datatype) = dt->handle;
    MPIDIG_REQUEST(rreq, count) /= dt->size;

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = put_target_cmpl_cb;
    MPIDIG_recv_type_init(MPIDIG_REQUEST(rreq, req->preq.origin_data_sz), rreq);

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

  fn_exit:
    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_put_data);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_DATA_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_acc_data_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                  int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDIG_acc_dat_msg_t *msg_hdr = (MPIDIG_acc_dat_msg_t *) am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACC_DATA_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACC_DATA_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_acc_data);

    rreq = (MPIR_Request *) msg_hdr->preq_ptr;
    handle_acc_data(in_data_sz, rreq);

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = acc_target_cmpl_cb;

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

    MPIDIG_rma_set_am_flag();

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_acc_data);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACC_DATA_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_get_acc_data_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                      int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDIG_get_acc_dat_msg_t *msg_hdr = (MPIDIG_get_acc_dat_msg_t *) am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_DATA_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_DATA_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_acc_data);

    rreq = (MPIR_Request *) msg_hdr->preq_ptr;
    handle_acc_data(in_data_sz, rreq);

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = get_acc_target_cmpl_cb;

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_acc_data);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_DATA_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_cswap_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                               int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    MPI_Aint data_sz;
    MPIR_Win *win;

    int dt_contig;
    void *p_data;

    MPIDIG_cswap_req_msg_t *msg_hdr = (MPIDIG_cswap_req_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_CSWAP_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_CSWAP_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_cas);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = cswap_target_cmpl_cb;
    MPIDIG_REQUEST(rreq, req->seq_no) = MPL_atomic_fetch_add_uint64(&MPIDI_global.nxt_seq_no, 1);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    MPIDI_Datatype_check_contig_size(msg_hdr->datatype, 1, dt_contig, data_sz);

    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, msg_hdr->win_id);
    MPIR_Assert(win);

    uintptr_t base = MPIDIG_win_base_at_target(win);
    size_t offset = win->disp_unit * msg_hdr->target_disp;

    MPIDIG_REQUEST(rreq, req->creq.win_ptr) = win;
    MPIDIG_REQUEST(rreq, req->creq.creq_ptr) = msg_hdr->req_ptr;
    MPIDIG_REQUEST(rreq, req->creq.datatype) = msg_hdr->datatype;
    MPIDIG_REQUEST(rreq, req->creq.addr) = (char *) base + offset;
    MPIDIG_REQUEST(rreq, rank) = msg_hdr->src_rank;

    MPIR_Assert(dt_contig == 1);
    p_data = MPL_malloc(data_sz * 2, MPL_MEM_RMA);
    MPIR_Assert(p_data);

    MPIDIG_REQUEST(rreq, req->creq.data) = p_data;

    MPIDIG_recv_init(1, data_sz * 2, p_data, data_sz * 2, rreq);

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

  fn_exit:
    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_cas);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_CSWAP_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_acc_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                             int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    MPI_Aint origin_data_sz;
    void *p_data = NULL;
    MPIR_Win *win;

    MPIDIG_acc_req_msg_t *msg_hdr = (MPIDIG_acc_req_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACC_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACC_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_acc);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    MPIDI_Datatype_check_size(msg_hdr->origin_datatype, msg_hdr->origin_count, origin_data_sz);
    if (origin_data_sz) {
        p_data = MPL_malloc(origin_data_sz, MPL_MEM_RMA);
        MPIR_Assert(p_data);
    }

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = acc_target_cmpl_cb;
    MPIDIG_REQUEST(rreq, req->seq_no) = MPL_atomic_fetch_add_uint64(&MPIDI_global.nxt_seq_no, 1);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    /* Note we always init the receive to be the data_sz calculated from the datatype and count.
     * If the network somehow send less data, something is very wrong */
    MPIDIG_recv_init(1, origin_data_sz, p_data, origin_data_sz, rreq);

    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, msg_hdr->win_id);
    MPIR_Assert(win);

    uintptr_t base = MPIDIG_win_base_at_target(win);
    size_t offset = win->disp_unit * msg_hdr->target_disp;

    MPIDIG_REQUEST(rreq, req->areq.win_ptr) = win;
    MPIDIG_REQUEST(rreq, req->areq.req_ptr) = msg_hdr->req_ptr;
    MPIDIG_REQUEST(rreq, req->areq.origin_datatype) = msg_hdr->origin_datatype;
    MPIDIG_REQUEST(rreq, req->areq.target_datatype) = msg_hdr->target_datatype;
    MPIDIG_REQUEST(rreq, req->areq.origin_count) = msg_hdr->origin_count;
    MPIDIG_REQUEST(rreq, req->areq.target_count) = msg_hdr->target_count;
    MPIDIG_REQUEST(rreq, req->areq.target_addr) = (char *) base + offset;
    MPIDIG_REQUEST(rreq, req->areq.op) = msg_hdr->op;
    MPIDIG_REQUEST(rreq, req->areq.data) = p_data;
    MPIDIG_REQUEST(rreq, req->areq.flattened_dt) = NULL;
    MPIDIG_REQUEST(rreq, req->areq.result_data_sz) = msg_hdr->result_data_sz;
    MPIDIG_REQUEST(rreq, rank) = msg_hdr->src_rank;

    if (msg_hdr->flattened_sz) {
        /* FIXME: MPIR_Typerep_unflatten should allocate the new object */
        MPIR_Datatype *dt = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
        if (!dt) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                                 "MPIR_Datatype_mem");
        }
        MPIR_Object_set_ref(dt, 1);
        MPIR_Typerep_unflatten(dt, (char *) am_hdr + sizeof(*msg_hdr));
        MPIDIG_REQUEST(rreq, req->areq.target_datatype) = dt->handle;
    }

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

  fn_exit:
    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_acc);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACC_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


int MPIDIG_get_acc_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                 int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_acc);

    /* the same handling processing as ACC except the completion handler function. */
    /* set is_async to 1 so we can get rreq back */
    MPIR_Request *rreq;
    mpi_errno = MPIDIG_acc_target_msg_cb(handler_id, am_hdr, data, in_data_sz, is_local, 1, &rreq);

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = get_acc_target_cmpl_cb;

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_acc);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_acc_dt_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    MPIR_Win *win;
    uintptr_t base;
    size_t offset;

    MPIDIG_acc_req_msg_t *msg_hdr = (MPIDIG_acc_req_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACC_DT_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACC_DT_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_acc_dt);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, msg_hdr->win_id);
    MPIR_Assert(win);

    base = MPIDIG_win_base_at_target(win);
    offset = win->disp_unit * msg_hdr->target_disp;

    MPIDIG_REQUEST(rreq, req->areq.win_ptr) = win;
    MPIDIG_REQUEST(rreq, req->areq.req_ptr) = msg_hdr->req_ptr;
    MPIDIG_REQUEST(rreq, req->areq.origin_datatype) = msg_hdr->origin_datatype;
    MPIDIG_REQUEST(rreq, req->areq.target_datatype) = MPI_DATATYPE_NULL;
    MPIDIG_REQUEST(rreq, req->areq.origin_count) = msg_hdr->origin_count;
    MPIDIG_REQUEST(rreq, req->areq.target_count) = msg_hdr->target_count;
    MPIDIG_REQUEST(rreq, req->areq.target_addr) = (void *) (offset + base);
    MPIDIG_REQUEST(rreq, req->areq.op) = msg_hdr->op;
    MPIDIG_REQUEST(rreq, req->areq.result_data_sz) = msg_hdr->result_data_sz;
    MPIDIG_REQUEST(rreq, rank) = msg_hdr->src_rank;

    void *flattened_dt = MPL_malloc(msg_hdr->flattened_sz, MPL_MEM_RMA);
    MPIR_Assert(flattened_dt);
    MPIDIG_REQUEST(rreq, req->areq.flattened_dt) = flattened_dt;
    MPIDIG_recv_init(1, msg_hdr->flattened_sz, flattened_dt, msg_hdr->flattened_sz, rreq);

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = acc_dt_target_cmpl_cb;
    MPIDIG_REQUEST(rreq, req->seq_no) = MPL_atomic_fetch_add_uint64(&MPIDI_global.nxt_seq_no, 1);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

  fn_exit:
    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_acc_dt);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACC_DT_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_get_acc_dt_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                    int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_DT_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_DT_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_acc_dt);

    /* the same handling processing as ACC except the completion handler function. */
    /* set is_async to 1 so we can get rreq back */
    MPIR_Request *rreq;
    mpi_errno = MPIDIG_acc_dt_target_msg_cb(handler_id, am_hdr, data,
                                            in_data_sz, is_local, 1, &rreq);

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = get_acc_dt_target_cmpl_cb;

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_acc_dt);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_DT_TARGET_MSG_CB);
    return mpi_errno;
}

int MPIDIG_get_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                             int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    MPIDIG_get_msg_t *msg_hdr = (MPIDIG_get_msg_t *) am_hdr;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = get_target_cmpl_cb;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, msg_hdr->win_id);
    MPIR_Assert(win);

    uintptr_t base = MPIDIG_win_base_at_target(win);
    size_t offset = win->disp_unit * msg_hdr->target_disp;

    MPIDIG_REQUEST(rreq, req->greq.win_ptr) = win;
    MPIDIG_REQUEST(rreq, req->greq.count) = msg_hdr->target_count;
    MPIDIG_REQUEST(rreq, req->greq.datatype) = msg_hdr->target_datatype;
    MPIDIG_REQUEST(rreq, req->greq.flattened_dt) = NULL;
    MPIDIG_REQUEST(rreq, req->greq.dt) = NULL;
    MPIDIG_REQUEST(rreq, req->greq.greq_ptr) = msg_hdr->greq_ptr;
    MPIDIG_REQUEST(rreq, rank) = msg_hdr->src_rank;

    if (msg_hdr->flattened_sz) {
        void *flattened_dt = MPL_malloc(msg_hdr->flattened_sz, MPL_MEM_BUFFER);
        MPIDIG_recv_init(1, msg_hdr->flattened_sz, flattened_dt, msg_hdr->flattened_sz, rreq);
        MPIDIG_REQUEST(rreq, req->greq.flattened_dt) = flattened_dt;
        MPIDIG_REQUEST(rreq, req->greq.addr) = (char *) base + offset;
    } else {
        MPIR_Assert(!in_data_sz || in_data_sz == 0);
        MPIDIG_recv_init(1, 0, NULL, 0, rreq);
        MPIDIG_REQUEST(rreq, req->greq.addr) = (char *) base + offset + msg_hdr->target_true_lb;
    }

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

  fn_exit:
    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_get_ack_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                 int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;

    MPIDIG_get_ack_msg_t *msg_hdr = (MPIDIG_get_ack_msg_t *) am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_ack);

    rreq = (MPIR_Request *) msg_hdr->greq_ptr;
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__RMA);

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = get_ack_target_cmpl_cb;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    MPIDIG_REQUEST(rreq, buffer) = MPIDIG_REQUEST(rreq, req->greq.addr);
    MPIDIG_REQUEST(rreq, count) = MPIDIG_REQUEST(rreq, req->greq.count);
    MPIDIG_REQUEST(rreq, datatype) = MPIDIG_REQUEST(rreq, req->greq.datatype);
    MPIDIG_recv_type_init(msg_hdr->target_data_sz, rreq);

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

    MPIDIG_rma_set_am_flag();
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACK_TARGET_MSG_CB);
    return mpi_errno;
}
