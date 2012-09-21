/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "rma.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Win_unlock_all */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Win_unlock_all = PMPIX_Win_unlock_all
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Win_unlock_all  MPIX_Win_unlock_all
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Win_unlock_all as PMPIX_Win_unlock_all
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Win_unlock_all
#define MPIX_Win_unlock_all PMPIX_Win_unlock_all

#endif

#undef FUNCNAME
#define FUNCNAME MPIX_Win_unlock_all

/*@
   MPIX_Win_unlock_all - Completes an RMA access epoch all processes on the
   given window

   Input Parameters:
. win - window object (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_RANK
.N MPI_ERR_WIN
.N MPI_ERR_OTHER

.seealso: MPIX_Win_lock_all
@*/
int MPIX_Win_unlock_all(MPI_Win win)
{
    static const char FCNAME[] = "MPIX_Win_unlock_all";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_WIN_UNLOCK_ALL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_WIN_UNLOCK_ALL);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_WIN(win, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Win_get_ptr( win, win_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
            /* If win_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;

            /* Test that the rank we are unlocking is the rank that we locked */
            if (win_ptr->lockRank != MPID_WIN_STATE_LOCKED_ALL) {
                if (win_ptr->lockRank == MPID_WIN_STATE_UNLOCKED) {
                    MPIU_ERR_SET(mpi_errno,MPI_ERR_RANK,"**winunlockwithoutlock");
                }
                else {
                    MPIU_ERR_SET2(mpi_errno,MPI_ERR_RANK,
                    "**mismatchedlockrank", 
                    "**mismatchedlockrank %d %d", MPID_WIN_STATE_LOCKED_ALL, win_ptr->lockRank );
                }
                if (mpi_errno) goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPIU_RMA_CALL(win_ptr,Win_unlock_all(win_ptr));
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    /* Clear the lockRank on success with the unlock */
    /* FIXME: Should this always be cleared, even on failure? */
    win_ptr->lockRank = MPID_WIN_STATE_UNLOCKED;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_WIN_UNLOCK_ALL);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpix_win_unlock_all", 
            "**mpix_win_unlock_all %W", win);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

