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



    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_start(group, assert, win);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno = MPIDIG_mpi_win_start(group, assert, win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_complete(MPIR_Win * win)
{
    int mpi_errno;



    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_complete(win);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno = MPIDIG_mpi_win_complete(win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno;



    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_post(group, assert, win);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno = MPIDIG_mpi_win_post(group, assert, win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_wait(MPIR_Win * win)
{
    int mpi_errno;



    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_wait(win);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno = MPIDIG_mpi_win_wait(win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPID_Win_test(MPIR_Win * win, int *flag)
{
    int mpi_errno;



    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_test(win, flag);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno = MPIDIG_mpi_win_test(win, flag);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_lock(int lock_type, int rank, int assert, MPIR_Win * win)
{
    int mpi_errno;



    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_lock(lock_type, rank, assert, win, NULL);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno = MPIDIG_mpi_win_lock(lock_type, rank, assert, win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_unlock(int rank, MPIR_Win * win)
{
    int mpi_errno;



    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_unlock(rank, win, NULL);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno = MPIDIG_mpi_win_unlock(rank, win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_fence(int assert, MPIR_Win * win)
{
    int mpi_errno;



    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_fence(assert, win);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno = MPIDIG_mpi_win_fence(assert, win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_flush_local(int rank, MPIR_Win * win)
{
    int mpi_errno;



    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_flush_local(rank, win, NULL);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno = MPIDIG_mpi_win_flush_local(rank, win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_shared_query(MPIR_Win * win,
                                                   int rank, MPI_Aint * size, int *disp_unit,
                                                   void *baseptr)
{
    int mpi_errno;


#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno = MPIDIG_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_flush(int rank, MPIR_Win * win)
{
    int mpi_errno;



    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_flush(rank, win, NULL);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno = MPIDIG_mpi_win_flush(rank, win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_flush_local_all(MPIR_Win * win)
{
    int mpi_errno;



    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_flush_local_all(win);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno = MPIDIG_mpi_win_flush_local_all(win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_unlock_all(MPIR_Win * win)
{
    int mpi_errno;



    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_unlock_all(win);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno = MPIDIG_mpi_win_unlock_all(win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_sync(MPIR_Win * win)
{
    int mpi_errno;



    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_sync(win);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno = MPIDIG_mpi_win_sync(win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_flush_all(MPIR_Win * win)
{
    int mpi_errno;



    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_flush_all(win);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno = MPIDIG_mpi_win_flush_all(win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Win_lock_all(int assert, MPIR_Win * win)
{
    int mpi_errno;



    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_win_lock_all(assert, win);
    MPIR_ERR_CHECK(mpi_errno);
#else
    mpi_errno = MPIDIG_mpi_win_lock_all(assert, win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_WIN_H_INCLUDED */
