/*
 * Copyright (C) by Argonne National Laboratory See COPYRIGHT in
 *     top-level directory
 */

#include "mpiimpl.h"
#include "errcodes.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Delete_error_string */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Delete_error_string = PMPIX_Delete_error_string
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Delete_error_string  MPIX_Delete_error_string
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Delete_error_string as PMPIX_Delete_error_string
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Delete_error_string(int errorcode)
    __attribute__ ((weak, alias("PMPIX_Delete_error_string")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Delete_error_string
#define MPIX_Delete_error_string PMPIX_Delete_error_string

#endif

/*@
   MPIX_Delete_error_string - Delete the error string associated with an MPI error code or
   class

Input Parameters:
+ errorcode - error code or class (integer)

   Notes:
According to the MPI-2 standard, it is erroneous to call 'MPIX_Delete_error_string'
for an error code or class with a value less than or equal
to 'MPI_ERR_LASTCODE'.  Thus, you cannot replace the predefined error messages
with this routine.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPIX_Delete_error_string(int errorcode)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_DELETE_ERROR_STRING);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_DELETE_ERROR_STRING);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* verify that errorclass is a dynamic class */
            MPIR_ERRTEST_ARGNULL(errorcode, "errcode", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Err_delete_msg(errorcode);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_DELETE_ERROR_STRING);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_delete_error_string", "**mpi_delete_error_string %d",
                                 errorcode);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
