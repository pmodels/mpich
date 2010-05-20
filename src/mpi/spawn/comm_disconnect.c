/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_disconnect */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_disconnect = PMPI_Comm_disconnect
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_disconnect  MPI_Comm_disconnect
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_disconnect as PMPI_Comm_disconnect
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_disconnect
#define MPI_Comm_disconnect PMPI_Comm_disconnect

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_disconnect

/*@
   MPI_Comm_disconnect - Disconnect from a communicator

   Input Parameter:
.  comm - communicator (handle) 

Notes:
This routine waits for all pending communication to complete, then frees the
communicator and sets 'comm' to 'MPI_COMM_NULL'.  It may not be called 
with 'MPI_COMM_WORLD' or 'MPI_COMM_SELF'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.seealso MPI_Comm_connect, MPI_Comm_join
@*/
int MPI_Comm_disconnect(MPI_Comm * comm)
{
    static const char FCNAME[] = "MPI_Comm_disconnect";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_DISCONNECT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_DISCONNECT);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(*comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( *comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno)
	    {
		goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /*
     * Since outstanding I/O bumps the reference count on the communicator, 
     * we wait until we hold the last reference count to
     * ensure that all communication has completed.  The reference count
     * is 1 when the communicator is created, and it is incremented
     * only for pending communication operations (and decremented when
     * those complete).
     */
    /* FIXME-MT should we be checking this? */
    if (MPIU_Object_get_ref(comm_ptr) > 1)
    {
	MPID_Progress_state progress_state;
	
	MPID_Progress_start(&progress_state);
	while (MPIU_Object_get_ref(comm_ptr) > 1)
	{
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		MPID_Progress_end(&progress_state);
		goto fn_fail;
	    }
	    /* --END ERROR HANDLING-- */
	}
	MPID_Progress_end(&progress_state);
    }
    
    mpi_errno = MPID_Comm_disconnect(comm_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    
    *comm = MPI_COMM_NULL;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_DISCONNECT);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_comm_disconnect",
	    "**mpi_comm_disconnect %C", *comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
