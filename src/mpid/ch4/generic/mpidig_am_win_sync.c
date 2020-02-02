#include "mpidimpl.h"

static int MPIDIG_win_ctrl_target_msg_cb(int handler_id, void *am_hdr,
                                         void **data, size_t * p_data_sz,
                                         int is_local, int *is_contig,
                                         MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                         MPIR_Request ** req);

MPIR_T_pvar_timer_t PVAR_TIMER_rma_winlock_getlocallock ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_win_ctrl ATTRIBUTE((unused));

int MPIDIG_am_win_sync_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_COMPLETE, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_POST, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_LOCK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_LOCK_ACK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_UNLOCK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_UNLOCK_ACK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_LOCKALL, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_LOCKALL_ACK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_UNLOCKALL, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_UNLOCKALL_ACK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    /* rma_targetcb_win_ctrl */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_win_ctrl,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for WIN CTRL (in seconds)");

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int win_lock_advance(MPIR_Win * win, int is_local);
static void win_lock_req_proc(int handler_id, int is_local,
                              const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win);
static void win_lock_ack_proc(int handler_id, const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win);
static int win_unlock_proc(const MPIDIG_win_cntrl_msg_t * info, int is_local, MPIR_Win * win);
static void win_complete_proc(const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win);
static void win_post_proc(const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win);
static void win_unlock_done(const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win);

static int MPIDIG_win_ctrl_target_msg_cb(int handler_id, void *am_hdr,
                                         void **data, size_t * p_data_sz,
                                         int is_local, int *is_contig,
                                         MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                         MPIR_Request ** req)
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
            win_lock_req_proc(handler_id, is_local, msg_hdr, win);
            break;

        case MPIDIG_WIN_LOCK_ACK:
        case MPIDIG_WIN_LOCKALL_ACK:
            win_lock_ack_proc(handler_id, msg_hdr, win);
            break;

        case MPIDIG_WIN_UNLOCK:
        case MPIDIG_WIN_UNLOCKALL:
            mpi_errno = win_unlock_proc(msg_hdr, is_local, win);
            MPIR_ERR_CHECK(mpi_errno);
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
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_CTRL_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* internal functions */
static int win_lock_advance(MPIR_Win * win, int is_local)
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

        MPIDIG_win_cntrl_msg_t am_hdr;
        int handler_id;
        am_hdr.win_id = MPIDIG_WIN(win, win_id);
        am_hdr.origin_rank = win->comm_ptr->rank;

        if (lock->mtype == MPIDIG_WIN_LOCK)
            handler_id = MPIDIG_WIN_LOCK_ACK;
        else if (lock->mtype == MPIDIG_WIN_LOCKALL)
            handler_id = MPIDIG_WIN_LOCKALL_ACK;
        else
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**rmasync");

        MPIDIG_AM_SEND_HDR(is_local, lock->rank, win->comm_ptr, handler_id);
        MPIR_ERR_CHECK(mpi_errno);

        MPL_free(lock);

        mpi_errno = win_lock_advance(win, is_local);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_LOCK_ADVANCE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void win_lock_req_proc(int handler_id, int is_local,
                              const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win)
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

    win_lock_advance(win, is_local);
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

static int win_unlock_proc(const MPIDIG_win_cntrl_msg_t * info, int is_local, MPIR_Win * win)
{

    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_UNLOCK_PROC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_UNLOCK_PROC);

    /* NOTE: origin blocking waits in lock or lockall call till lock granted. */
    --MPIDIG_WIN(win, sync).lock_recvd.count;
    MPIR_Assert((int) MPIDIG_WIN(win, sync).lock_recvd.count >= 0);
    win_lock_advance(win, is_local);

    MPIDIG_win_cntrl_msg_t am_hdr;
    am_hdr.win_id = MPIDIG_WIN(win, win_id);
    am_hdr.origin_rank = win->comm_ptr->rank;

    MPIDIG_AM_SEND_HDR(is_local, info->origin_rank, win->comm_ptr, MPIDIG_WIN_UNLOCK_ACK);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_UNLOCK_PROC);
    return mpi_errno;
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
