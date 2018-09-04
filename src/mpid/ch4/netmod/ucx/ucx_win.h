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

static inline int MPIDI_NM_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    return MPIDIG_mpi_win_set_info(win, info);
}

static inline int MPIDI_NM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    return MPIDIG_mpi_win_start(group, assert, win);
}

static inline int MPIDI_NM_mpi_win_complete(MPIR_Win * win)
{
    return MPIDIG_mpi_win_complete(win);
}

static inline int MPIDI_NM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{

    return MPIDIG_mpi_win_post(group, assert, win);
}

static inline int MPIDI_NM_mpi_win_wait(MPIR_Win * win)
{
    return MPIDIG_mpi_win_wait(win);
}

static inline int MPIDI_NM_mpi_win_test(MPIR_Win * win, int *flag)
{
    return MPIDIG_mpi_win_test(win, flag);
}

static inline int MPIDI_NM_mpi_win_lock(int lock_type, int rank, int assert,
                                        MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDIG_mpi_win_lock(lock_type, rank, assert, win);
}

static inline int MPIDI_NM_mpi_win_unlock(int rank, MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDIG_mpi_win_unlock(rank, win);
}

static inline int MPIDI_NM_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    return MPIDIG_mpi_win_get_info(win, info_p_p);
}

static inline int MPIDI_NM_mpi_win_fence(int assert, MPIR_Win * win)
{
    return MPIDIG_mpi_win_fence(assert, win);
}

static inline int MPIDI_NM_mpi_win_flush(int rank, MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDIG_mpi_win_flush(rank, win);
}

static inline int MPIDI_NM_mpi_win_flush_local_all(MPIR_Win * win)
{
    return MPIDIG_mpi_win_flush_local_all(win);
}

static inline int MPIDI_NM_mpi_win_unlock_all(MPIR_Win * win)
{
    return MPIDIG_mpi_win_unlock_all(win);
}

static inline int MPIDI_NM_mpi_win_flush_local(int rank, MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDIG_mpi_win_flush_local(rank, win);
}

static inline int MPIDI_NM_mpi_win_sync(MPIR_Win * win)
{
    return MPIDIG_mpi_win_sync(win);
}

static inline int MPIDI_NM_mpi_win_flush_all(MPIR_Win * win)
{
    return MPIDIG_mpi_win_flush_all(win);
}

static inline int MPIDI_NM_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    return MPIDIG_mpi_win_lock_all(assert, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_win_cmpl_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_WIN_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_WIN_CMPL_HOOK);

#ifndef MPICH_UCX_AM_ONLY
    ucs_status_t ucp_status;

    /* maybe we just flush all eps here? More efficient for smaller communicators... */
    ucp_status = ucp_worker_flush(MPIDI_UCX_global.worker);
    MPIDI_UCX_CHK_STATUS(ucp_status);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_WIN_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_win_local_cmpl_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_WIN_LOCAL_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_WIN_LOCAL_CMPL_HOOK);

#ifndef MPICH_UCX_AM_ONLY
    ucs_status_t ucp_status;

    /* currently, UCP does not support local flush, so we have to call
     * a global flush. This is not good for performance - but OK for now */
    if (MPIDI_UCX_WIN(win).need_local_flush == 1) {
        ucp_status = ucp_worker_flush(MPIDI_UCX_global.worker);
        MPIDI_UCX_CHK_STATUS(ucp_status);
        MPIDI_UCX_WIN(win).need_local_flush = 0;
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_WIN_LOCAL_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_target_cmpl_hook(int rank, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_TARGET_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_TARGET_CMPL_HOOK);

#ifndef MPICH_UCX_AM_ONLY
    ucs_status_t ucp_status;

    if (rank != MPI_PROC_NULL) {
        ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(win->comm_ptr, rank);
        /* only flush the endpoint */
        ucp_status = ucp_ep_flush(ep);
        MPIDI_UCX_CHK_STATUS(ucp_status);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_TARGET_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_target_local_cmpl_hook(int rank, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_TARGET_LOCAL_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_TARGET_LOCAL_CMPL_HOOK);

#ifndef MPICH_UCX_AM_ONLY
    ucs_status_t ucp_status;

    if (rank != MPI_PROC_NULL) {
        ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(win->comm_ptr, rank);
        /* currently, UCP does not support local flush, so we have to call
         * a global flush. This is not good for performance - but OK for now */

        if (MPIDI_UCX_WIN(win).need_local_flush == 1) {
            ucp_status = ucp_ep_flush(ep);
            MPIDI_UCX_CHK_STATUS(ucp_status);
            MPIDI_UCX_WIN(win).need_local_flush = 0;
        }
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_TARGET_LOCAL_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* UCX_WIN_H_INCLUDED */
