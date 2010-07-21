/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Errhandler_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Errhandler_create = PMPI_Errhandler_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Errhandler_create  MPI_Errhandler_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Errhandler_create as PMPI_Errhandler_create
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Errhandler_create
#define MPI_Errhandler_create PMPI_Errhandler_create

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Errhandler_create

/*@
  MPI_Errhandler_create - Creates an MPI-style errorhandler

Input Parameter:
. function - user defined error handling procedure 

Output Parameter:
. errhandler - MPI error handler (handle) 

Notes:
The MPI Standard states that an implementation may make the output value 
(errhandler) simply the address of the function.  However, the action of 
'MPI_Errhandler_free' makes this impossible, since it is required to set the
value of the argument to 'MPI_ERRHANDLER_NULL'.  In addition, the actual
error handler must remain until all communicators that use it are 
freed.

.N ThreadSafe

.N Deprecated
The replacement routine for this function is 'MPI_Comm_create_errhandler'.

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_EXHAUSTED

.seealso: MPI_Comm_create_errhandler, MPI_Errhandler_free
@*/
int MPI_Errhandler_create(MPI_Handler_function *function, 
                          MPI_Errhandler *errhandler)
{
    static const char FCNAME[] = "MPI_Errhandler_create";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ERRHANDLER_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ERRHANDLER_CREATE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(function, "function", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(errhandler, "errhandler", mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Comm_create_errhandler_impl( function, errhandler );
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERRHANDLER_CREATE);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_errhandler_create",
	    "**mpi_errhandler_create %p %p", function, errhandler);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

