/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_lock */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_lock = PMPI_Win_lock
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_lock  MPI_Win_lock
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_lock as PMPI_Win_lock
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win)
     __attribute__ ((weak, alias("PMPI_Win_lock")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_lock
#define MPI_Win_lock PMPI_Win_lock

#endif

/*@
   MPI_Win_lock - Begin an RMA access epoch at the target process

Input Parameters:
+ lock_type - either MPI_LOCK_EXCLUSIVE or MPI_LOCK_SHARED (state)
. rank - rank of locked window (non-negative integer)
. assert - program assertion (integer)
- win - window object (handle)

Notes:
The name of this routine is misleading.  In particular, this
routine need not block, except when the target process is the calling
process.

Implementations may restrict the use of RMA communication that is
synchronized
by lock calls to windows in memory allocated by 'MPI_Alloc_mem'. Locks can
be used portably only in such memory.

The 'assert' argument is used to indicate special conditions for the
fence that an implementation may use to optimize the 'MPI_Win_lock'
operation.  The value zero is always correct.  Other assertion values
may be or''ed together.  Assertions that are valid for 'MPI_Win_lock' are\:

. MPI_MODE_NOCHECK - no other process holds, or will attempt to acquire a
  conflicting lock, while the caller holds the window lock. This is useful
  when mutual exclusion is achieved by other means, but the coherence
  operations that may be attached to the lock and unlock calls are still
  required.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_RANK
.N MPI_ERR_WIN
@*/

int MPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WIN_LOCK);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_WIN_LOCK);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_WIN(win, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    MPIR_Win_get_ptr(win, win_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Win_valid_ptr(win_ptr, mpi_errno);
            if (mpi_errno) {
                goto fn_fail;
            }
            MPIR_ERRTEST_SEND_RANK(win_ptr->comm_ptr, rank, mpi_errno);
            if (assert != (assert & (MPI_MODE_NOCHECK))) {
                MPIR_ERR_SET1(mpi_errno, MPI_ERR_ARG, "**assert", "**assert %d", assert);
                goto fn_fail;
            }
            if (lock_type != MPI_LOCK_SHARED && lock_type != MPI_LOCK_EXCLUSIVE) {
                MPIR_ERR_SET1(mpi_errno, MPI_ERR_ARG, "**locktype", "**locktype %d", lock_type);
                goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Return immediately for dummy process */
    if (unlikely(rank == MPI_PROC_NULL)) {
        goto fn_exit;
    }

    /* ... body of routine ... */
    mpi_errno = MPID_Win_lock(lock_type, rank, assert, win_ptr);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_WIN_LOCK);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_win_lock", "**mpi_win_lock %d %d %A %W", lock_type, rank,
                                     assert, win);
#endif
    mpi_errno = MPIR_Err_return_win(win_ptr, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
