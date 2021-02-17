/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Query_thread */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Query_thread = PMPI_Query_thread
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Query_thread  MPI_Query_thread
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Query_thread as PMPI_Query_thread
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Query_thread(int *provided)  __attribute__ ((weak, alias("PMPI_Query_thread")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Query_thread
#define MPI_Query_thread PMPI_Query_thread

#endif

/*@
   MPI_Query_thread - Return the level of thread support provided by the MPI library

Output Parameters:
. provided - provided level of thread support (integer)

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

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
@*/

int MPI_Query_thread(int *provided)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_QUERY_THREAD);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_QUERY_THREAD);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(provided, "provided", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    *provided = MPIR_ThreadInfo.thread_provided;
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_QUERY_THREAD);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_query_thread", "**mpi_query_thread %p", provided);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
