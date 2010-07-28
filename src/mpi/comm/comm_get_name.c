/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_get_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_get_name = PMPI_Comm_get_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_get_name  MPI_Comm_get_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_get_name as PMPI_Comm_get_name
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_get_name
#define MPI_Comm_get_name PMPI_Comm_get_name

void MPIR_Comm_get_name_impl(MPID_Comm *comm_ptr, char *comm_name, int *resultlen)
{
    /* The user must allocate a large enough section of memory */
    MPIU_Strncpy(comm_name, comm_ptr->name, MPI_MAX_OBJECT_NAME);
    *resultlen = (int)strlen(comm_name);
    return;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_get_name
#undef FCNAME
#define FCNAME "MPI_Comm_get_name"

/*@
  MPI_Comm_get_name - Return the print name from the communicator

  Input Parameter:
. comm - Communicator to get name of (handle)

  Output Parameters:
+ comm_name - On output, contains the name of the communicator.  It must
  be an array of size at least 'MPI_MAX_OBJECT_NAME'.
- resultlen - Number of characters in name

 Notes:

.N COMMNULL

.N ThreadSafeNoUpdate

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
@*/
int MPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_GET_NAME);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_GET_NAME);

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
#   endif /* HAVE_ERROR_CHECKING */

    /* Validate parameters and objects (post conversion) */
    MPID_Comm_get_ptr( comm, comm_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Comm_valid_ptr( comm_ptr, mpi_errno );

	    /* If comm_ptr is not valid, it will be reset to null */
	    MPIR_ERRTEST_ARGNULL( comm_name, "comm_name", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( resultlen, "resultlen", mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    MPIR_Comm_get_name_impl(comm_ptr, comm_name, resultlen);
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_GET_NAME);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_comm_get_name",
	    "**mpi_comm_get_name %C %p %p", comm, comm_name, resultlen);
    }
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}

