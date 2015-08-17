/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_flush_all */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_flush_all = PMPI_Win_flush_all
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_flush_all  MPI_Win_flush_all
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_flush_all as PMPI_Win_flush_all
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_flush_all(MPI_Win win) __attribute__((weak,alias("PMPI_Win_flush_all")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_flush_all
#define MPI_Win_flush_all PMPI_Win_flush_all

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_flush_all

/*@
MPI_Win_flush_all - Complete all outstanding RMA operations at all targets


All RMA operations issued by the calling process to any target on the specified
window prior to this call and in the specified window will have completed
both at the origin and at the target when this call returns.

Input Parameters:
. win - window object (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_RANK
.N MPI_ERR_WIN
.N MPI_ERR_OTHER

.seealso: MPI_Win_flush MPI_Win_flush_local MPI_Win_flush_local_all MPI_Win_lock MPI_Win_lock_all
@*/
int MPI_Win_flush_all(MPI_Win win)
{
    static const char FCNAME[] = "MPI_Win_flush_all";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_FLUSH_ALL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_FLUSH_ALL);

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
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;

            /* TODO: Validate that the given window is in passive mode */
            /* TODO: Validate that the given rank is locked */
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Win_flush_all(win_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_FLUSH_ALL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_win_flush_all",
            "**mpi_win_flush_all %W", win);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
