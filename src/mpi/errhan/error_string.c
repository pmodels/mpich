/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "errcodes.h"

/* -- Begin Profiling Symbol Block for routine MPI_Error_string */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Error_string = PMPI_Error_string
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Error_string  MPI_Error_string
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Error_string as PMPI_Error_string
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Error_string
#define MPI_Error_string PMPI_Error_string

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Error_string

/*@
   MPI_Error_string - Return a string for a given error code

Input Parameters:
. errorcode - Error code returned by an MPI routine or an MPI error class

Output Parameter:
+ string - Text that corresponds to the errorcode 
- resultlen - Length of string 

Notes:  Error codes are the values return by MPI routines (in C) or in the
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
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPI_Error_string";
#endif
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ERROR_STRING);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ERROR_STRING);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(string,"string",mpi_errno);
	    MPIR_ERRTEST_ARGNULL(resultlen,"resultlen",mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    MPIR_Err_get_string( errorcode, string, MPI_MAX_ERROR_STRING, NULL );
    *resultlen = (int)strlen( string );

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERROR_STRING);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_error_string",
	    "**mpi_error_string %d %s %p", errorcode, string, resultlen);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}

