/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
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

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_win_cmpl_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_WIN_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_WIN_CMPL_HOOK);

#ifndef MPICH_UCX_AM_ONLY
    if (MPIDI_UCX_is_reachable_win(win) && MPIDI_UCX_win_need_flush(win)) {
        ucs_status_t ucp_status;
        int vni = MPIDI_VCI(MPIDI_COMM_VCI(win->comm_ptr)).vni;
        /* maybe we just flush all eps here? More efficient for smaller communicators... */
        ucp_status = ucp_worker_flush(MPIDI_UCX_VNI(vni).worker);
        MPIDI_UCX_CHK_STATUS(ucp_status);
        MPIDI_UCX_win_unset_sync(win);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_WIN_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_win_local_cmpl_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_WIN_LOCAL_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_WIN_LOCAL_CMPL_HOOK);

#ifndef MPICH_UCX_AM_ONLY
    if (MPIDI_UCX_is_reachable_win(win) && MPIDI_UCX_win_need_flush_local(win)) {
        ucs_status_t ucp_status;

        /* currently, UCP does not support local flush, so we have to call
         * a global flush. This is not good for performance - but OK for now */
        ucp_status = ucp_worker_flush(MPIDI_UCX_VNI(0).worker);
        MPIDI_UCX_CHK_STATUS(ucp_status);

        /* TODO: should set to FLUSH after replace with real local flush. */
        MPIDI_UCX_win_unset_sync(win);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_WIN_LOCAL_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_target_cmpl_hook(int rank, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_TARGET_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_TARGET_CMPL_HOOK);

#ifndef MPICH_UCX_AM_ONLY
    if (MPIDI_UCX_is_reachable_target(rank, win) &&
        /* including cases where FLUSH_LOCAL or FLUSH is set */
        MPIDI_UCX_WIN(win).target_sync[rank].need_sync >= MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL) {

        ucs_status_t ucp_status;
        int vni = MPIDI_VCI(MPIDI_COMM_VCI(win->comm_ptr)).vni;
        ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(win->comm_ptr, rank, vni);
        /* only flush the endpoint */
        ucp_status = ucp_ep_flush(ep);
        MPIDI_UCX_CHK_STATUS(ucp_status);
        MPIDI_UCX_WIN(win).target_sync[rank].need_sync = MPIDI_UCX_WIN_SYNC_UNSET;
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_TARGET_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_target_local_cmpl_hook(int rank, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_TARGET_LOCAL_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_TARGET_LOCAL_CMPL_HOOK);

#ifndef MPICH_UCX_AM_ONLY
    if (MPIDI_UCX_is_reachable_target(rank, win) &&
        MPIDI_UCX_WIN(win).target_sync[rank].need_sync == MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL) {
        ucs_status_t ucp_status;

        ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(win->comm_ptr, rank, 0);
        /* currently, UCP does not support local flush, so we have to call
         * a global flush. This is not good for performance - but OK for now */
        ucp_status = ucp_ep_flush(ep);
        MPIDI_UCX_CHK_STATUS(ucp_status);

        /* TODO: should set to FLUSH after replace with real local flush. */
        MPIDI_UCX_WIN(win).target_sync[rank].need_sync = MPIDI_UCX_WIN_SYNC_UNSET;
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_TARGET_LOCAL_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* UCX_WIN_H_INCLUDED */
