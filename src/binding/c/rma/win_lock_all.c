/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_lock_all */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_lock_all = PMPI_Win_lock_all
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_lock_all  MPI_Win_lock_all
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_lock_all as PMPI_Win_lock_all
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_lock_all(int assert, MPI_Win win)  __attribute__ ((weak, alias("PMPI_Win_lock_all")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_lock_all
#define MPI_Win_lock_all PMPI_Win_lock_all

#endif

/*@
   MPI_Win_lock_all - Begin an RMA access epoch at all processes on the given window

Input Parameters:
+ assert - program assertion (integer)
- win - window object (handle)

Notes:

This call is not collective.

The 'assert' argument is used to indicate special conditions for the fence that
an implementation may use to optimize the 'MPI_Win_lock_all' operation.  The
value zero is always correct.  Other assertion values may be or''ed together.
Assertions that are valid for 'MPI_Win_lock_all' are\:

. 'MPI_MODE_NOCHECK' - No other process holds, or will attempt to acquire a
  conflicting lock, while the caller holds the window lock. This is useful
  when mutual exclusion is achieved by other means, but the coherence
  operations that may be attached to the lock and unlock calls are still
  required.

There may be additional overheads associated with using 'MPI_Win_lock' and
'MPI_Win_lock_all' concurrently on the same window. These overheads could be
avoided by specifying the assertion 'MPI_MODE_NOCHECK' when possible

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_WIN
.seealso: MPI_Win_unlock_all
@*/

int MPI_Win_lock_all(int assert, MPI_Win win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WIN_LOCK_ALL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_WIN_LOCK_ALL);

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
            if (assert != (assert & (MPI_MODE_NOCHECK))) {
                MPIR_ERR_SET1(mpi_errno, MPI_ERR_ARG, "**assert", "**assert %d", assert);
                goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPID_Win_lock_all(assert, win_ptr);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_WIN_LOCK_ALL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_win_lock_all", "**mpi_win_lock_all %A %W", assert, win);
#endif
    mpi_errno = MPIR_Err_return_win(win_ptr, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
