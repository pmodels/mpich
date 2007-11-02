/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Errhandler_get */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Errhandler_get = PMPI_Errhandler_get
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Errhandler_get  MPI_Errhandler_get
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Errhandler_get as PMPI_Errhandler_get
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Errhandler_get
#define MPI_Errhandler_get PMPI_Errhandler_get

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Errhandler_get

/*@
  MPI_Errhandler_get - Gets the error handler for a communicator

Input Parameter:
. comm - communicator to get the error handler from (handle) 

Output Parameter:
. errhandler - MPI error handler currently associated with communicator
(handle) 

.N ThreadSafe

.N Fortran

Note on Implementation:

The MPI Standard was unclear on whether this routine required the user to call 
'MPI_Errhandler_free' once for each call made to this routine in order to 
free the error handler.  After some debate, the MPI Forum added an explicit
statement that users are required to call 'MPI_Errhandler_free' when the
return value from this routine is no longer needed.  This behavior is similar
to the other MPI routines for getting objects; for example, 'MPI_Comm_group' 
requires that the user call 'MPI_Group_free' when the group returned
by 'MPI_Comm_group' is no longer needed.

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_ARG
@*/
int MPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler *errhandler)
{
    static const char FCNAME[] = "MPI_Errhandler_get";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIU_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ERRHANDLER_GET);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("errhan");
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ERRHANDLER_GET);

    MPIU_THREADPRIV_GET;
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr; if comm_ptr is not value, it will be reset to null */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    MPIR_ERRTEST_ARGNULL(errhandler, "errhandler", mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    MPIR_Nest_incr();
    mpi_errno = NMPI_Comm_get_errhandler( comm, errhandler );
    MPIR_Nest_decr();
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERRHANDLER_GET);
    MPIU_THREAD_SINGLE_CS_EXIT("errhan");
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_errhandler_get",
	    "**mpi_errhandler_get %C %p", comm, errhandler);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

