/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

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

#undef FUNCNAME
#define FUNCNAME MPI_Win_lock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Win_lock - Begin an RMA access epoch at the target process.

Input Parameters:
+ lock_type - Indicates whether other processes may access the target
   window at the same time (if 'MPI_LOCK_SHARED') or not ('MPI_LOCK_EXCLUSIVE')
. rank - rank of locked window (nonnegative integer)
. assert - Used to optimize this call; zero may be used as a default.
  See notes. (integer)
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
.N MPI_ERR_OTHER
@*/
int MPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WIN_LOCK);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_WIN_LOCK);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_WIN(win, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Win_get_ptr(win, win_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Comm *comm_ptr;

            /* Validate win_ptr */
            MPIR_Win_valid_ptr(win_ptr, mpi_errno);
            /* If win_ptr is not valid, it will be reset to null */
            if (mpi_errno)
                goto fn_fail;

            if (assert != 0 && assert != MPI_MODE_NOCHECK) {
                MPIR_ERR_SET1(mpi_errno, MPI_ERR_ARG,
                              "**lockassertval", "**lockassertval %d", assert);
                if (mpi_errno)
                    goto fn_fail;
            }
            if (lock_type != MPI_LOCK_SHARED && lock_type != MPI_LOCK_EXCLUSIVE) {
                MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**locktype");
                if (mpi_errno)
                    goto fn_fail;
            }

            comm_ptr = win_ptr->comm_ptr;
            MPIR_ERRTEST_SEND_RANK(comm_ptr, rank, mpi_errno);

            /* TODO: Test if window is unlocked */

            /* TODO: Validate that window is not in active mode */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Win_lock(lock_type, rank, assert, win_ptr);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_WIN_LOCK);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_win_lock", "**mpi_win_lock %d %d %A %W", lock_type, rank,
                                 assert, win);
    }
#endif
    mpi_errno = MPIR_Err_return_win(win_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
