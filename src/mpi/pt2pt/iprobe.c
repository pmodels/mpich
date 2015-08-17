/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Iprobe */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Iprobe = PMPI_Iprobe
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Iprobe  MPI_Iprobe
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Iprobe as PMPI_Iprobe
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status) __attribute__((weak,alias("PMPI_Iprobe")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Iprobe
#define MPI_Iprobe PMPI_Iprobe

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Iprobe

/*@
    MPI_Iprobe - Nonblocking test for a message

Input Parameters:
+ source - source rank, or  'MPI_ANY_SOURCE' (integer) 
. tag - tag value or  'MPI_ANY_TAG' (integer) 
- comm - communicator (handle) 

Output Parameters:
+ flag - True if a message with the specified source, tag, and communicator 
    is available (logical) 
- status - status object (Status) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_TAG
.N MPI_ERR_RANK

@*/
int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, 
	       MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Iprobe";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_IPROBE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_IPROBE);
    
    /* Validate handle parameters needing to be converted */
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
    MPID_Comm_get_ptr( comm, comm_ptr );
    
    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Validate communicator */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno) goto fn_fail;
	    
	    MPIR_ERRTEST_ARGNULL( flag, "flag", mpi_errno );
	    MPIR_ERRTEST_RECV_TAG(tag,mpi_errno);
	    if (comm_ptr) {
		MPIR_ERRTEST_RECV_RANK(comm_ptr, source, mpi_errno);
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* FIXME: Is this correct for intercomms? */
    mpi_errno = MPID_Iprobe(source, tag, comm_ptr, MPID_CONTEXT_INTRA_PT2PT, 
			    flag, status);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_IPROBE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_iprobe",
	    "**mpi_iprobe %i %t %C %p %p", source, tag, comm, flag, status);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
