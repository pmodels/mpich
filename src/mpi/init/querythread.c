/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Query_thread */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Query_thread = PMPI_Query_thread
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Query_thread  MPI_Query_thread
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Query_thread as PMPI_Query_thread
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Query_thread
#define MPI_Query_thread PMPI_Query_thread
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Query_thread

/*@
   MPI_Query_thread - Return the level of thread support provided by the MPI 
    library

   Output Parameter:
.  provided - Level of thread support provided.  This is the same value
   that was returned in the 'provided' argument in 'MPI_Init_thread'.

   Notes:
   The valid values for the level of thread support are\:
+ MPI_THREAD_SINGLE - Only one thread will execute. 
. MPI_THREAD_FUNNELED - The process may be multi-threaded, but only the main 
  thread will make MPI calls (all MPI calls are funneled to the 
   main thread). 
. MPI_THREAD_SERIALIZED - The process may be multi-threaded, and multiple 
  threads may make MPI calls, but only one at a time: MPI calls are not 
  made concurrently from two distinct threads (all MPI calls are serialized). 
- MPI_THREAD_MULTIPLE - Multiple threads may call MPI, with no restrictions. 

   If 'MPI_Init' was called instead of 'MPI_Init_thread', the level of
   thread support is defined by the implementation.  This routine allows
   you to find out the provided level.  It is also useful for library 
   routines that discover that MPI has already been initialized and
   wish to determine what level of thread support is available.

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Query_thread( int *provided )
{
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPI_Query_thread";
#endif
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_QUERY_THREAD);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_QUERY_THREAD);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(provided,"provided",mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* ... body of routine ...  */
    *provided = MPIR_ThreadInfo.thread_provided;
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_QUERY_THREAD);
    return mpi_errno;
    
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_query_thread",
	    "**mpi_query_thread %p", provided);
    }
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
