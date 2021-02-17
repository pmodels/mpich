/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Init_thread */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Init_thread = PMPI_Init_thread
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Init_thread  MPI_Init_thread
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Init_thread as PMPI_Init_thread
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Init_thread(int *argc, char ***argv, int required, int *provided)
     __attribute__ ((weak, alias("PMPI_Init_thread")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Init_thread
#define MPI_Init_thread PMPI_Init_thread

#endif

/*@
   MPI_Init_thread - Initialize the MPI execution environment

Input/Output Parameters:
+ argc - argc (None)
- argv - argv (None)

Input Parameters:
. required - desired level of thread support (integer)

Output Parameters:
. provided - provided level of thread support (integer)

Command line arguments:
MPI specifies no command-line arguments but does allow an MPI
implementation to make use of them.  See 'MPI_INIT' for a description of
the command line arguments supported by 'MPI_INIT' and 'MPI_INIT_THREAD'.

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

Notes for Fortran:
Note that the Fortran binding for this routine does not have the 'argc' and
'argv' arguments. ('MPI_INIT_THREAD(required, provided, ierror)')

.N Errors
.N MPI_SUCCESS

.seealso: MPI_Init, MPI_Finalize
@*/

int MPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_INIT_THREAD);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_INIT_THREAD);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPL_atomic_load_int(&MPIR_Process.mpich_state) != MPICH_MPI_STATE__PRE_INIT) {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPI_Init_thread", __LINE__, MPI_ERR_OTHER, "**inittwice", 0);
                if (mpi_errno) {
                    goto fn_fail;
                }
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPIR_Init_thread_impl(argc, argv, required, provided);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_INIT_THREAD);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_init_thread", "**mpi_init_thread %p %p %d %p", argc, argv,
                                     required, provided);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
