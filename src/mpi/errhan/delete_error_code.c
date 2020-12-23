/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Delete_error_code */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Delete_error_code = PMPIX_Delete_error_code
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Delete_error_code  MPIX_Delete_error_code
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Delete_error_code as PMPIX_Delete_error_code
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Delete_error_code(int errorcode)
    __attribute__ ((weak, alias("PMPIX_Delete_error_code")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Delete_error_code
#define MPIX_Delete_error_code PMPIX_Delete_error_code

#endif

/*@
   MPIX_Delete_error_code - Delete an MPI error code

Input Parameters:
.  errorcode - Error code to be deleted.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
@*/
int MPIX_Delete_error_code(int errorcode)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_DELETE_ERROR_CODE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_DELETE_ERROR_CODE);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* FIXME: verify that errorclass is a dynamic class */
            MPIR_ERRTEST_ARGNULL(errorcode, "errorcode", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Delete_error_code_impl(errorcode);
    if (mpi_errno) {
        goto fn_fail;
    }

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_DELETE_ERROR_CODE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_delete_error_code", "**mpi_delete_error_code %d",
                                 errorcode);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
