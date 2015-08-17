/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_flush_local */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_flush_local = PMPI_Win_flush_local
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_flush_local  MPI_Win_flush_local
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_flush_local as PMPI_Win_flush_local
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_flush_local(int rank, MPI_Win win) __attribute__((weak,alias("PMPI_Win_flush_local")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_flush_local
#define MPI_Win_flush_local PMPI_Win_flush_local

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_flush_local

/*@
MPI_Win_flush_local - Complete locally all outstanding RMA operations at the
given target


Locally completes at the origin all outstanding RMA operations initiated by the
calling process to the target process specified by rank on the specified
window. For example, after this routine completes, the user may reuse any
buffers provided to put, get, or accumulate operations.

Input Parameters:
+ rank - rank of window (nonnegative integer)
- win - window object (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_RANK
.N MPI_ERR_WIN
.N MPI_ERR_OTHER

.seealso: MPI_Win_flush MPI_Win_flush_all MPI_Win_flush_local_all MPI_Win_lock MPI_Win_lock_all
@*/
int MPI_Win_flush_local(int rank, MPI_Win win)
{
    static const char FCNAME[] = "MPI_Win_flush_local";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_FLUSH_LOCAL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_FLUSH_LOCAL);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_WIN(win, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI object handles to object pointers */
    MPID_Win_get_ptr( win, win_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Comm * comm_ptr;
            
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;

            comm_ptr = win_ptr->comm_ptr;
            MPIR_ERRTEST_SEND_RANK(comm_ptr, rank, mpi_errno);

            /* TODO: Validate that the given window is in passive mode */
            /* TODO: Validate that the given rank is locked */
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Win_flush_local(rank, win_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_FLUSH_LOCAL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_win_flush_local",
            "**mpi_win_flush_local %d %W", rank, win);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
