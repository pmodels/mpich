/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_free = PMPI_Comm_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_free  MPI_Comm_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_free as PMPI_Comm_free
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_free
#define MPI_Comm_free PMPI_Comm_free

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_free_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Comm_free_impl(MPID_Comm * comm_ptr)
{
    return MPIR_Comm_release(comm_ptr, 0);
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_free
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPI_Comm_free - Marks the communicator object for deallocation

Input Parameter:
. comm - Communicator to be destroyed (handle) 

Notes:
This routine `frees` a communicator.  Because the communicator may still
be in use by other MPI routines, the actual communicator storage will not
be freed until all references to this communicator are removed.  For most
users, the effect of this routine is the same as if it was in fact freed
at this time of this call.  

Null Handles:
The MPI 1.1 specification, in the section on opaque objects, explicitly
disallows freeing a null communicator.  The text from the standard is:
.vb
 A null handle argument is an erroneous IN argument in MPI calls, unless an
 exception is explicitly stated in the text that defines the function. Such
 exception is allowed for handles to request objects in Wait and Test calls
 (sections Communication Completion and Multiple Completions ). Otherwise, a
 null handle can only be passed to a function that allocates a new object and
 returns a reference to it in the handle.
.ve

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_ARG
@*/
int MPI_Comm_free(MPI_Comm *comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_FREE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_FREE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(*comm, mpi_errno);
            if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( *comm, comm_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
	    
	    /* Cannot free the predefined communicators */
	    if (HANDLE_GET_KIND(*comm) == HANDLE_KIND_BUILTIN) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
                      MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_COMM,
					  "**commperm", "**commperm %s", 
						  comm_ptr->name );
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPIR_Comm_free_impl(comm_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    
    *comm = MPI_COMM_NULL;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_FREE);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_comm_free", "**mpi_comm_free %p", comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
