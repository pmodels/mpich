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
#ifndef CH4_WIN_H_INCLUDED
#define CH4_WIN_H_INCLUDED

#include "ch4_impl.h"
#include "ch4r_win.h"

MPL_STATIC_INLINE_PREFIX int MPID_Win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_START);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_start(group, assert, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#else
    mpi_errno = MPIDIG_mpi_win_start(group, assert, win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_START);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_complete(MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_COMPLETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_COMPLETE);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_complete(win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#else
    mpi_errno = MPIDIG_mpi_win_complete(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_POST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_POST);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_post(group, assert, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#else
    mpi_errno = MPIDIG_mpi_win_post(group, assert, win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_POST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_wait(MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_WAIT);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_wait(win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#else
    mpi_errno = MPIDIG_mpi_win_wait(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_WAIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPID_Win_test(MPIR_Win * win, int *flag)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_TEST);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_test(win, flag);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#else
    mpi_errno = MPIDIG_mpi_win_test(win, flag);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_TEST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_lock(int lock_type, int rank, int assert, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_LOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_LOCK);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_lock(lock_type, rank, assert, win, NULL);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#else
    mpi_errno = MPIDIG_mpi_win_lock(lock_type, rank, assert, win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_LOCK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_unlock(int rank, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_UNLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_UNLOCK);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_unlock(rank, win, NULL);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#else
    mpi_errno = MPIDIG_mpi_win_unlock(rank, win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_fence(int assert, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_FENCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_FENCE);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_fence(assert, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#else
    mpi_errno = MPIDIG_mpi_win_fence(assert, win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_FENCE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_flush_local(int rank, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_FLUSH_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_FLUSH_LOCAL);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_flush_local(rank, win, NULL);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#else
    mpi_errno = MPIDIG_mpi_win_flush_local(rank, win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_FLUSH_LOCAL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_shared_query(MPIR_Win * win,
                                                   int rank, MPI_Aint * size, int *disp_unit,
                                                   void *baseptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_SHARED_QUERY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_SHARED_QUERY);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#else
    mpi_errno = MPIDIG_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_SHARED_QUERY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_flush(int rank, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_FLUSH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_FLUSH);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_flush(rank, win, NULL);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#else
    mpi_errno = MPIDIG_mpi_win_flush(rank, win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_FLUSH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_flush_local_all(MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_FLUSH_LOCAL_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_FLUSH_LOCAL_ALL);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_flush_local_all(win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#else
    mpi_errno = MPIDIG_mpi_win_flush_local_all(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_FLUSH_LOCAL_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_unlock_all(MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_UNLOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_UNLOCK_ALL);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_unlock_all(win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#else
    mpi_errno = MPIDIG_mpi_win_unlock_all(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_UNLOCK_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_sync(MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_SYNC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_SYNC);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_sync(win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#else
    mpi_errno = MPIDIG_mpi_win_sync(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_SYNC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_flush_all(MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_FLUSH_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_FLUSH_ALL);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_flush_all(win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#else
    mpi_errno = MPIDIG_mpi_win_flush_all(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_FLUSH_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_lock_all(int assert, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_LOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_LOCK_ALL);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_lock_all(assert, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#else
    mpi_errno = MPIDIG_mpi_win_lock_all(assert, win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_LOCK_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_WIN_H_INCLUDED */
