/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Error_string */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Error_string = PMPI_Error_string
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Error_string  MPI_Error_string
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Error_string as PMPI_Error_string
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Error_string(int errorcode, char *string, int *resultlen)
     __attribute__ ((weak, alias("PMPI_Error_string")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Error_string
#define MPI_Error_string PMPI_Error_string

#endif

/*@
   MPI_Error_string - Return a string for a given error code

Input Parameters:
. errorcode - Error code returned by an MPI routine (integer)

Output Parameters:
+ string - Text that corresponds to the errorcode (string)
- resultlen - Length (in printable characters) of the result returned in string (integer)

Notes:
Error codes are the values return by MPI routines (in C) or in the
'ierr' argument (in Fortran).  These can be converted into error classes
with the routine 'MPI_Error_class'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
@*/

int MPI_Error_string(int errorcode, char *string, int *resultlen)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ERROR_STRING);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_ERROR_STRING);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(string, "string", mpi_errno);
            MPIR_ERRTEST_ARGNULL(resultlen, "resultlen", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    MPIR_Err_get_string(errorcode, string, MPI_MAX_ERROR_STRING, NULL);
    *resultlen = (int) strlen(string);
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_ERROR_STRING);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_error_string", "**mpi_error_string %d %p %p", errorcode,
                                     string, resultlen);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
