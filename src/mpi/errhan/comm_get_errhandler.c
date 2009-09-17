/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_get_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_get_errhandler = PMPI_Comm_get_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_get_errhandler  MPI_Comm_get_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_get_errhandler as PMPI_Comm_get_errhandler
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_get_errhandler
#define MPI_Comm_get_errhandler PMPI_Comm_get_errhandler

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_get_errhandler

/*@
   MPI_Comm_get_errhandler - Get the error handler attached to a communicator

   Input Parameter:
. comm - communicator (handle) 

   Output Parameter:
. errhandler - handler currently associated with communicator (handle) 

.N ThreadSafeNoUpdate

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
@*/
int MPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler)
{
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPI_Comm_get_errhandler";
#endif
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIU_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_GET_ERRHANDLER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_GET_ERRHANDLER);
    
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
            /* Validate comm_ptr; if comm_ptr is not valid, it will be reset to null  */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    MPIR_ERRTEST_ARGNULL(errhandler,"errhandler",mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* Check for default error handler */
    if (!comm_ptr->errhandler) {
	*errhandler = MPI_ERRORS_ARE_FATAL;
    }
    else {
	*errhandler = comm_ptr->errhandler->handle;
	MPIR_Errhandler_add_ref(comm_ptr->errhandler);
    }
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_GET_ERRHANDLER);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_comm_get_errhandler",
	    "**mpi_comm_get_errhandler %C %p", comm, errhandler);
    }
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}

