/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Initialized */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Initialized = PMPI_Initialized
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Initialized  MPI_Initialized
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Initialized as PMPI_Initialized
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Initialized(int *flag)  __attribute__ ((weak, alias("PMPI_Initialized")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Initialized
#define MPI_Initialized PMPI_Initialized

#endif

/*@
   MPI_Initialized - Indicates whether 'MPI_Init' has been called.

Output Parameters:
. flag - Flag is true if MPI_INIT has been called and false otherwise (logical)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
@*/

int MPI_Initialized(int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_INITIALIZED);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_INITIALIZED);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    *flag = (MPL_atomic_load_int(&MPIR_Process.mpich_state) == MPICH_MPI_STATE__POST_INIT);
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_INITIALIZED);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    if (MPL_atomic_load_int(&MPIR_Process.mpich_state) != MPICH_MPI_STATE__PRE_INIT && MPL_atomic_load_int(&MPIR_Process.mpich_state) != MPICH_MPI_STATE__POST_FINALIZED) {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER, "**mpi_initialized", "**mpi_initialized %p", flag);
        mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    }
#endif
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
