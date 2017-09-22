/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_remote_size */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_remote_size = PMPI_Comm_remote_size
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_remote_size  MPI_Comm_remote_size
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_remote_size as PMPI_Comm_remote_size
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_remote_size(MPI_Comm comm, int *size) __attribute__((weak,alias("PMPI_Comm_remote_size")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_remote_size
#define MPI_Comm_remote_size PMPI_Comm_remote_size

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_remote_size
#undef FCNAME
#define FCNAME "MPI_Comm_remote_size"

/*@

MPI_Comm_remote_size - Determines the size of the remote group 
                       associated with an inter-communictor

Input Parameters:
. comm - communicator (handle) 

Output Parameters:
. size - number of processes in the remote group of 'comm'  (integer) 

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_ARG
@*/
int MPI_Comm_remote_size(MPI_Comm comm, int *size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_REMOTE_SIZE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_REMOTE_SIZE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, TRUE );
	    /* If comm_ptr is not valid, it will be reset to null */
	    if (comm_ptr && comm_ptr->comm_kind != MPIR_COMM_KIND__INTERCOMM) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
                        MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_COMM, 
						  "**commnotinter", 0 );
	    }
            if (mpi_errno) goto fn_fail;
            MPIR_ERRTEST_ARGNULL(size, "size", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    *size = comm_ptr->remote_size;
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_REMOTE_SIZE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_comm_remote_size",
	    "**mpi_comm_remote_size %C %p", comm, size);
    }
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}

