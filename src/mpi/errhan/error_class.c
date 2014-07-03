/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "errcodes.h"

/* -- Begin Profiling Symbol Block for routine MPI_Error_class */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Error_class = PMPI_Error_class
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Error_class  MPI_Error_class
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Error_class as PMPI_Error_class
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Error_class(int errorcode, int *errorclass) __attribute__((weak,alias("PMPI_Error_class")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Error_class
#define MPI_Error_class PMPI_Error_class

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Error_class

/*@
   MPI_Error_class - Converts an error code into an error class

Input Parameters:
. errorcode - Error code returned by an MPI routine 

Output Parameters:
. errorclass - Error class associated with 'errorcode'

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Error_class(int errorcode, int *errorclass)
{
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPI_Error_class";
#endif
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ERROR_CLASS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ERROR_CLASS);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(errorclass,"errorclass",mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* We include the dynamic bit because this is needed to fully
       describe the dynamic error classes */
    *errorclass = errorcode & (ERROR_CLASS_MASK | ERROR_DYN_MASK);
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERROR_CLASS);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_error_class",
	    "**mpi_error_class %d %p", errorcode, errorclass);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}

