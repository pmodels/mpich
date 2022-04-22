/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_WIN_H_INCLUDED
#define UCX_WIN_H_INCLUDED

#include "ucx_impl.h"

MPL_STATIC_INLINE_PREFIX bool MPIDI_UCX_win_need_flush(MPIR_Win * win)
{
    int rank;
    bool need_flush = false;
    /* including cases where FLUSH_LOCAL or FLUSH is set */
    for (rank = 0; rank < win->comm_ptr->local_size; rank++)
        need_flush |=
            (MPIDI_UCX_WIN(win).target_sync[rank].need_sync >= MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL);
    return need_flush;
}

MPL_STATIC_INLINE_PREFIX bool MPIDI_UCX_win_need_flush_local(MPIR_Win * win)
{
    int rank;
    bool need_flush_local = false;
    for (rank = 0; rank < win->comm_ptr->local_size; rank++)
        need_flush_local |=
            (MPIDI_UCX_WIN(win).target_sync[rank].need_sync == MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL);
    return need_flush_local;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_win_unset_sync(MPIR_Win * win)
{
    int rank;
    for (rank = 0; rank < win->comm_ptr->local_size; rank++)
        MPIDI_UCX_WIN(win).target_sync[rank].need_sync = MPIDI_UCX_WIN_SYNC_UNSET;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    return MPIDIG_mpi_win_start(group, assert, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_complete(MPIR_Win * win)
{
    return MPIDIG_mpi_win_complete(win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    return MPIDIG_mpi_win_post(group, assert, win);
}


MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_wait(MPIR_Win * win)
{
    return MPIDIG_mpi_win_wait(win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_test(MPIR_Win * win, int *flag)
{
    return MPIDIG_mpi_win_test(win, flag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_lock(int lock_type, int rank, int assert,
                                                   MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDIG_mpi_win_lock(lock_type, rank, assert, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_unlock(int rank, MPIR_Win * win,
                                                     MPIDI_av_entry_t * addr)
{
    return MPIDIG_mpi_win_unlock(rank, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_fence(int assert, MPIR_Win * win)
{
    return MPIDIG_mpi_win_fence(assert, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_shared_query(MPIR_Win * win,
                                                           int rank,
                                                           MPI_Aint * size, int *disp_unit,
                                                           void *baseptr)
{
    return MPIDIG_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush(int rank, MPIR_Win * win,
                                                    MPIDI_av_entry_t * addr)
{
    return MPIDIG_mpi_win_flush(rank, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush_local_all(MPIR_Win * win)
{
    return MPIDIG_mpi_win_flush_local_all(win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_unlock_all(MPIR_Win * win)
{
    return MPIDIG_mpi_win_unlock_all(win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush_local(int rank, MPIR_Win * win,
                                                          MPIDI_av_entry_t * addr)
{
    return MPIDIG_mpi_win_flush_local(rank, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_sync(MPIR_Win * win)
{
    return MPIDIG_mpi_win_sync(win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush_all(MPIR_Win * win)
{
    return MPIDIG_mpi_win_flush_all(win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    return MPIDIG_mpi_win_lock_all(assert, win);
}

/* replacing deprecated ucp_worker_flush */
MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_flush_cmpl_cb(void *request, ucs_status_t status)
{
}

MPL_STATIC_INLINE_PREFIX ucs_status_t MPIDI_UCX_flush(int vni)
{
    void *request = ucp_worker_flush_nb(MPIDI_UCX_global.ctx[vni].worker,
                                        0, &MPIDI_UCX_flush_cmpl_cb);
    if (request == NULL) {
        return UCS_OK;
    } else if (UCS_PTR_IS_ERR(request)) {
        return UCS_PTR_STATUS(request);
    } else {
        ucs_status_t status;
        do {
            ucp_worker_progress(MPIDI_UCX_global.ctx[vni].worker);
            status = ucp_request_check_status(request);
        } while (status == UCS_INPROGRESS);
        ucp_request_release(request);
        return status;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_win_cmpl_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (MPIDI_UCX_WIN(win).info_table && MPIDI_UCX_win_need_flush(win)) {
        ucs_status_t ucp_status;
        int vni = MPIDI_WIN(win, am_vci);
        MPIDI_UCX_THREAD_CS_ENTER_VCI(vni);
        ucp_status = MPIDI_UCX_flush(vni);
        MPIDI_UCX_THREAD_CS_EXIT_VCI(vni);
        MPIDI_UCX_CHK_STATUS(ucp_status);
        MPIDI_UCX_win_unset_sync(win);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_win_local_cmpl_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (MPIDI_UCX_WIN(win).info_table && MPIDI_UCX_win_need_flush_local(win)) {
        ucs_status_t ucp_status;

        /* currently, UCP does not support local flush, so we have to call
         * a global flush. This is not good for performance - but OK for now */
        int vni = MPIDI_WIN(win, am_vci);
        MPIDI_UCX_THREAD_CS_ENTER_VCI(vni);
        ucp_status = MPIDI_UCX_flush(vni);
        MPIDI_UCX_THREAD_CS_EXIT_VCI(vni);
        MPIDI_UCX_CHK_STATUS(ucp_status);

        /* TODO: should set to FLUSH after replace with real local flush. */
        MPIDI_UCX_win_unset_sync(win);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_target_cmpl_hook(int rank, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (MPIDI_UCX_is_reachable_target(rank, win, MPIDI_WIN(win, winattr)) &&
        /* including cases where FLUSH_LOCAL or FLUSH is set */
        MPIDI_UCX_WIN(win).target_sync[rank].need_sync >= MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL) {

        ucs_status_t ucp_status;
        int vni = MPIDI_WIN(win, am_vci);
        int vni_target = MPIDI_WIN_TARGET_VCI(win, rank);
        ucp_ep_h ep = MPIDI_UCX_WIN_TO_EP(win, rank, vni, vni_target);
        /* only flush the endpoint */
        MPIDI_UCX_THREAD_CS_ENTER_VCI(vni);
        ucp_status = ucp_ep_flush(ep);
        MPIDI_UCX_THREAD_CS_EXIT_VCI(vni);
        MPIDI_UCX_CHK_STATUS(ucp_status);
        MPIDI_UCX_WIN(win).target_sync[rank].need_sync = MPIDI_UCX_WIN_SYNC_UNSET;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_target_local_cmpl_hook(int rank, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (MPIDI_UCX_is_reachable_target(rank, win, MPIDI_WIN(win, winattr)) &&
        MPIDI_UCX_WIN(win).target_sync[rank].need_sync == MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL) {
        ucs_status_t ucp_status;

        int vni = MPIDI_WIN(win, am_vci);
        int vni_target = MPIDI_WIN_TARGET_VCI(win, rank);
        ucp_ep_h ep = MPIDI_UCX_WIN_TO_EP(win, rank, vni, vni_target);
        /* currently, UCP does not support local flush, so we have to call
         * a global flush. This is not good for performance - but OK for now */
        MPIDI_UCX_THREAD_CS_ENTER_VCI(vni);
        ucp_status = ucp_ep_flush(ep);
        MPIDI_UCX_THREAD_CS_EXIT_VCI(vni);
        MPIDI_UCX_CHK_STATUS(ucp_status);

        /* TODO: should set to FLUSH after replace with real local flush. */
        MPIDI_UCX_WIN(win).target_sync[rank].need_sync = MPIDI_UCX_WIN_SYNC_UNSET;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* UCX_WIN_H_INCLUDED */
