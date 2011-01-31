/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Intercomm_merge */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Intercomm_merge = PMPI_Intercomm_merge
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Intercomm_merge  MPI_Intercomm_merge
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Intercomm_merge as PMPI_Intercomm_merge
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Intercomm_merge
#define MPI_Intercomm_merge PMPI_Intercomm_merge

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Intercomm_merge

/*@
MPI_Intercomm_merge - Creates an intracommuncator from an intercommunicator

Input Parameters:
+ comm - Intercommunicator (handle)
- high - Used to order the groups within comm (logical)
  when creating the new communicator.  This is a boolean value; the group
  that sets high true has its processes ordered `after` the group that sets 
  this value to false.  If all processes in the intercommunicator provide
  the same value, the choice of which group is ordered first is arbitrary.

Output Parameter:
. comm_out - Created intracommunicator (handle)

Notes:
 While all processes may provide the same value for the 'high' parameter,
 this requires the MPI implementation to determine which group of 
 processes should be ranked first. 

.N ThreadSafe

.N Fortran

Algorithm:
.Eb
.i Allocate contexts 
.i Local and remote group leaders swap high values
.i Determine the high value.
.i Merge the two groups and make the intra-communicator
.Ee

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_EXHAUSTED

.seealso: MPI_Intercomm_create, MPI_Comm_free
@*/
int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm)
{
    static const char FCNAME[] = "MPI_Intercomm_merge";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Comm *newcomm_ptr;
    int  local_high, remote_high, i, j, new_size;
    MPIR_Context_id_t new_context_id;
    int errflag = FALSE;
    MPIU_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INTERCOMM_MERGE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);  
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INTERCOMM_MERGE);

    MPIU_THREADPRIV_GET;
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(intercomm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( intercomm, comm_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
	    if (comm_ptr && comm_ptr->comm_kind != MPID_INTERCOMM) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
		    MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_COMM,
						  "**commnotinter", 0 );
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    
    /* Make sure that we have a local intercommunicator */
    if (!comm_ptr->local_comm) {
	/* Manufacture the local communicator */
	MPIR_Setup_intercomm_localcomm( comm_ptr );
    }

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int acthigh;
	    /* Check for consistent valus of high in each local group.
	     The Intel test suite checks for this; it is also an easy
	     error to make */
	    acthigh = high ? 1 : 0;   /* Clamp high into 1 or 0 */
	    mpi_errno = MPIR_Allreduce_impl( MPI_IN_PLACE, &acthigh, 1, MPI_INT,
                                             MPI_SUM, comm_ptr->local_comm, &errflag );
	    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
	    /* acthigh must either == 0 or the size of the local comm */
	    if (acthigh != 0 && acthigh != comm_ptr->local_size) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
		    MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG, 
						  "**notsame",
						  "**notsame %s %s", "high", 
						  "MPI_Intercomm_merge" );
		goto fn_fail;
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Find the "high" value of the other group of processes.  This
       will be used to determine which group is ordered first in
       the generated communicator.  high is logical */
    local_high = high;
    if (comm_ptr->rank == 0) {
	/* This routine allows use to use the collective communication
	   context rather than the point-to-point context. */
	MPIC_Sendrecv( &local_high, 1, MPI_INT, 0, 0, 
		       &remote_high, 1, MPI_INT, 0, 0, intercomm, 
		       MPI_STATUS_IGNORE );
	
	/* If local_high and remote_high are the same, then order is arbitrary.
	   we use the gpids of the rank 0 member of the local and remote
	   groups to choose an order in this case. */
	if (local_high == remote_high) {
	    int ingpid[2], outgpid[2];
	    
	    MPID_GPID_Get( comm_ptr, 0, ingpid );
	    MPIC_Sendrecv( ingpid, 2, MPI_INT, 0, 1,
			   outgpid, 2, MPI_INT, 0, 1, intercomm, 
			   MPI_STATUS_IGNORE );

	    /* Note that the gpids cannot be the same because we are 
	       starting from a valid intercomm */
	    if (ingpid[0] < outgpid[0] ||
		(ingpid[0] == outgpid[0] && ingpid[1] < outgpid[1])) {
		local_high = 0;
	    }
	    else {
		local_high = 1;

		/* req#3930: The merge algorithm will deadlock if the gpids are inadvertently the
		   same due to implementation bugs in the MPID_GPID_Get() function */
		MPIU_Assert( !(ingpid[0] == outgpid[0] && ingpid[1] == outgpid[1]) );
	    }
	}
    }

    /* 
       All processes in the local group now need to get the 
       value of local_high, which may have changed if both groups
       of processes had the same value for high
    */
    mpi_errno = MPIR_Bcast_impl( &local_high, 1, MPI_INT, 0,
                                 comm_ptr->local_comm, &errflag );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    mpi_errno = MPIR_Comm_create( &newcomm_ptr );
    if (mpi_errno) goto fn_fail;

    new_size = comm_ptr->local_size + comm_ptr->remote_size;

    /* FIXME: For the intracomm, we need a consistent context id.  
       That means that one of the two groups needs to use 
       the recvcontext_id and the other must use the context_id */
    if (local_high) {
	newcomm_ptr->context_id	= comm_ptr->recvcontext_id + 2; /* See below */
    }
    else {
	newcomm_ptr->context_id	= comm_ptr->context_id + 2; /* See below */
    }
    newcomm_ptr->recvcontext_id	= newcomm_ptr->context_id;
    newcomm_ptr->remote_size	= newcomm_ptr->local_size   = new_size;
    newcomm_ptr->rank		= -1;
    newcomm_ptr->comm_kind	= MPID_INTRACOMM;

    /* Now we know which group comes first.  Build the new vcr 
       from the existing vcrs */
    MPID_VCRT_Create( new_size, &newcomm_ptr->vcrt );
    MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, &newcomm_ptr->vcr );
    if (local_high) {
	/* remote group first */
	j = 0;
	for (i=0; i<comm_ptr->remote_size; i++) {
	    MPID_VCR_Dup( comm_ptr->vcr[i], &newcomm_ptr->vcr[j++] );
	}
	for (i=0; i<comm_ptr->local_size; i++) {
	    if (i == comm_ptr->rank) newcomm_ptr->rank = j;
	    MPID_VCR_Dup( comm_ptr->local_vcr[i], &newcomm_ptr->vcr[j++] );
	}
    }
    else {
	/* local group first */
	j = 0;
	for (i=0; i<comm_ptr->local_size; i++) {
	    if (i == comm_ptr->rank) newcomm_ptr->rank = j;
	    MPID_VCR_Dup( comm_ptr->local_vcr[i], &newcomm_ptr->vcr[j++] );
	}
	for (i=0; i<comm_ptr->remote_size; i++) {
	    MPID_VCR_Dup( comm_ptr->vcr[i], &newcomm_ptr->vcr[j++] );
	}
    }

    /* We've setup a temporary context id, based on the context id
       used by the intercomm.  This allows us to perform the allreduce
       operations within the context id algorithm, since we already
       have a valid (almost - see comm_create_hook) communicator.
    */
    /* printf( "About to get context id \n" ); fflush( stdout ); */
    /* In the multi-threaded case, MPIR_Get_contextid assumes that the
       calling routine already holds the single criticial section */
    new_context_id = 0;
    mpi_errno = MPIR_Get_contextid( newcomm_ptr, &new_context_id );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_Assert(new_context_id != 0);

    newcomm_ptr->context_id	= new_context_id;
    newcomm_ptr->recvcontext_id	= new_context_id;

    /* Notify the device of this new communicator */
    MPID_Dev_comm_create_hook( newcomm_ptr );
    mpi_errno = MPIR_Comm_commit(newcomm_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    *newintracomm = newcomm_ptr->handle;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_MERGE);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_intercomm_merge",
	    "**mpi_intercomm_merge %C %d %p", intercomm, high, newintracomm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
