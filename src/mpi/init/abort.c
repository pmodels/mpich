/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* -- Begin Profiling Symbol Block for routine MPI_Abort */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Abort = PMPI_Abort
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Abort  MPI_Abort
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Abort as PMPI_Abort
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Abort
#define MPI_Abort PMPI_Abort

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Abort

/*@
   MPI_Abort - Terminates MPI execution environment

Input Parameters:
+ comm - communicator of tasks to abort 
- errorcode - error code to return to invoking environment 

Notes:
Terminates all MPI processes associated with the communicator 'comm'; in
most systems (all to date), terminates `all` processes.

.N NotThreadSafe
Because the 'MPI_Abort' routine is intended to ensure that an MPI
process (and possibly an entire job), it cannot wait for a thread to 
release a lock or other mechanism for atomic access.

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Abort(MPI_Comm comm, int errorcode)
{
    static const char FCNAME[] = "MPI_Abort";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    /* FIXME: 100 is arbitrary and may not be long enough */
    char abort_str[100], comm_name[MPI_MAX_OBJECT_NAME];
    int len = MPI_MAX_OBJECT_NAME;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ABORT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    /* FIXME: It isn't clear that Abort should wait for a critical section,
       since that could result in the Abort hanging if another routine is
       hung holding the critical section.  Also note the "not thread-safe"
       comment in the description of MPI_Abort above. */
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ABORT);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    if (!comm_ptr)
    {
	/* Use comm world if the communicator is not valid */
	comm_ptr = MPIR_Process.comm_world;
    }

    
    MPIR_Comm_get_name_impl(comm_ptr, comm_name, &len);
    if (len == 0)
    {
	MPIU_Snprintf(comm_name, MPI_MAX_OBJECT_NAME, "comm=0x%X", comm);
    }
    /* FIXME: This is not internationalized */
    MPIU_Snprintf(abort_str, 100, "application called MPI_Abort(%s, %d) - process %d", comm_name, errorcode, comm_ptr->rank);
    mpi_errno = MPID_Abort( comm_ptr, mpi_errno, errorcode, abort_str );
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    /* MPID_Abort() should never return MPI_SUCCESS */
    MPIU_Assert(0);
    /* --END ERROR HANDLING-- */
    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ABORT);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    /* It is not clear that doing aborting abort makes sense.  We may
       want to specify that erroneous arguments to MPI_Abort will
       cause an immediate abort. */
       
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_abort",
	    "**mpi_abort %C %d", comm, errorcode);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
