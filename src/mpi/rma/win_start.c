/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "rma.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_start */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_start = PMPI_Win_start
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_start  MPI_Win_start
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_start as PMPI_Win_start
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_start
#define MPI_Win_start PMPI_Win_start

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_start

/*@
   MPI_Win_start - Start an RMA access epoch for MPI

  Input Parameters:
+ group - group of target processes (handle) 
. assert - Used to optimize this call; zero may be used as a default.
  See notes. (integer) 
- win - window object (handle) 

   Notes:
   The 'assert' argument is used to indicate special conditions for the
   fence that an implementation may use to optimize the 'MPI_Win_start' 
   operation.  The value zero is always correct.  Other assertion values
   may be or''ed together.  Assertions tha are valid for 'MPI_Win_start' are\:

. MPI_MODE_NOCHECK - the matching calls to 'MPI_WIN_POST' have already 
  completed on all target processes when the call to 'MPI_WIN_START' is made. 
  The nocheck option can be specified in a start call if and only if it is 
  specified in each matching post call. This is similar to the optimization 
  of ready-send that may save a handshake when the handshake is implicit in 
  the code. (However, ready-send is matched by a regular receive, whereas 
  both start and post must specify the nocheck option.) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_OTHER
@*/
int MPI_Win_start(MPI_Group group, int assert, MPI_Win win)
{
    static const char FCNAME[] = "MPI_Win_start";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_Group *group_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_START);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_START);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_WIN(win, mpi_errno);
	    MPIR_ERRTEST_GROUP(group, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Get handles to MPI objects. */
    MPID_Win_get_ptr( win, win_ptr );
    MPID_Group_get_ptr(group, group_ptr);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;

            MPID_Group_valid_ptr(group_ptr, mpi_errno);

            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIU_RMA_CALL(win_ptr,Win_start(group_ptr, assert, win_ptr));
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_START);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_win_start", 
	    "**mpi_win_start %G %A %W", group, assert, win);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

