/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Finalize */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Finalize = PMPI_Finalize
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Finalize  MPI_Finalize
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Finalize as PMPI_Finalize
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Finalize(void)  __attribute__ ((weak, alias("PMPI_Finalize")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Finalize
#define MPI_Finalize PMPI_Finalize

#endif

/*@
   MPI_Finalize - Terminates MPI execution environment

Notes:
All processes must call this routine before exiting.  The number of
processes running `after` this routine is called is undefined;
it is best not to perform much more than a 'return rc' after calling
'MPI_Finalize'.

Thread and Signal Safety:
The MPI standard requires that 'MPI_Finalize' be called `only` by the same
thread that initialized MPI with either 'MPI_Init' or 'MPI_Init_thread'.

.N Fortran

.N Errors
.N MPI_SUCCESS

@*/

int MPI_Finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_FINALIZE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_FINALIZE);

    /* ... body of routine ... */
    mpi_errno = MPIR_Finalize_impl();
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_FINALIZE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_finalize", 0);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
