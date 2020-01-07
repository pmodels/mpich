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
#ifndef POSIX_WIN_H_INCLUDED
#define POSIX_WIN_H_INCLUDED

#include "posix_impl.h"

static inline int MPIDI_POSIX_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_start(group, assert, win);



    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_complete(MPIR_Win * win)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_complete(win);



    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_post(group, assert, win);



    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_wait(MPIR_Win * win)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_wait(win);



    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_test(MPIR_Win * win, int *flag)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_test(win, flag);



    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_lock(int lock_type, int rank, int assert, MPIR_Win * win)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_lock(lock_type, rank, assert, win);



    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_unlock(int rank, MPIR_Win * win)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_unlock(rank, win);



    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_fence(int assert, MPIR_Win * win)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_fence(assert, win);



    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_shared_query(MPIR_Win * win,
                                                   int rank,
                                                   MPI_Aint * size, int *disp_unit, void *baseptr)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);



    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_flush(int rank, MPIR_Win * win)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_flush(rank, win);



    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_flush_local_all(MPIR_Win * win)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_flush_local_all(win);



    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_unlock_all(MPIR_Win * win)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_unlock_all(win);



    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_flush_local(int rank, MPIR_Win * win)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_flush_local(rank, win);



    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_sync(MPIR_Win * win)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_sync(win);



    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_flush_all(MPIR_Win * win)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_flush_all(win);



    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    int mpi_errno;




    mpi_errno = MPIDIG_mpi_win_lock_all(assert, win);



    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_rma_win_cmpl_hook(MPIR_Win * win ATTRIBUTE((unused)))
{



    /* Always perform barrier to ensure ordering of local load/store. */
    OPA_read_write_barrier();


    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_rma_win_local_cmpl_hook(MPIR_Win * win ATTRIBUTE((unused)))
{



    /* Always perform barrier to ensure ordering of local load/store. */
    OPA_read_write_barrier();


    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_rma_target_cmpl_hook(int rank ATTRIBUTE((unused)),
                                                              MPIR_Win * win ATTRIBUTE((unused)))
{



    /* Always perform barrier to ensure ordering of local load/store. */
    OPA_read_write_barrier();


    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_rma_target_local_cmpl_hook(int rank ATTRIBUTE((unused)),
                                                                    MPIR_Win *
                                                                    win ATTRIBUTE((unused)))
{



    /* Always perform barrier to ensure ordering of local load/store. */
    OPA_read_write_barrier();


    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_rma_op_cs_enter_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win = NULL;



    posix_win = &win->dev.shm.posix;
    MPIDI_POSIX_RMA_MUTEX_LOCK(posix_win->shm_mutex_ptr);

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_rma_op_cs_exit_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win = NULL;



    posix_win = &win->dev.shm.posix;
    MPIDI_POSIX_RMA_MUTEX_UNLOCK(posix_win->shm_mutex_ptr);

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* POSIX_WIN_H_INCLUDED */
