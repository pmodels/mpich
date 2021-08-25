/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_WIN_H_INCLUDED
#define POSIX_WIN_H_INCLUDED

#include "posix_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_rma_win_cmpl_hook(MPIR_Win * win)
{
    MPIR_FUNC_ENTER;

    /* Always perform barrier to ensure ordering of local load/store. */
    MPL_atomic_read_write_barrier();

    MPIDI_POSIX_win_t *posix_win = &win->dev.shm.posix;
    MPIDI_POSIX_rma_outstanding_req_flushall(posix_win);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_rma_win_local_cmpl_hook(MPIR_Win * win)
{
    MPIR_FUNC_ENTER;

    /* Always perform barrier to ensure ordering of local load/store. */
    MPL_atomic_read_write_barrier();

    MPIDI_POSIX_win_t *posix_win = &win->dev.shm.posix;
    MPIDI_POSIX_rma_outstanding_req_flushall(posix_win);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_rma_target_cmpl_hook(int rank ATTRIBUTE((unused)),
                                                              MPIR_Win * win)
{
    MPIR_FUNC_ENTER;

    /* Always perform barrier to ensure ordering of local load/store. */
    MPL_atomic_read_write_barrier();

    MPIDI_POSIX_win_t *posix_win = &win->dev.shm.posix;
    MPIDI_POSIX_rma_outstanding_req_flushall(posix_win);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_rma_target_local_cmpl_hook(int rank ATTRIBUTE((unused)),
                                                                    MPIR_Win * win)
{
    MPIR_FUNC_ENTER;

    /* Always perform barrier to ensure ordering of local load/store. */
    MPL_atomic_read_write_barrier();

    MPIDI_POSIX_win_t *posix_win = &win->dev.shm.posix;
    MPIDI_POSIX_rma_outstanding_req_flushall(posix_win);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_rma_op_cs_enter_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win = NULL;
    MPIR_FUNC_ENTER;

    posix_win = &win->dev.shm.posix;
    MPIDI_POSIX_RMA_MUTEX_LOCK(posix_win->shm_mutex_ptr);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_rma_op_cs_exit_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win = NULL;
    MPIR_FUNC_ENTER;

    posix_win = &win->dev.shm.posix;
    MPIDI_POSIX_RMA_MUTEX_UNLOCK(posix_win->shm_mutex_ptr);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* non-inlined function prototypes */

int MPIDI_POSIX_shm_win_init_hook(MPIR_Win * win);

#endif /* POSIX_WIN_H_INCLUDED */
