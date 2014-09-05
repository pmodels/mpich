/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm) __attribute__((weak,alias("PMPI_Intercomm_merge")));
#endif
/* -- End Profiling Symbol Block */

/* These functions help implement the merge procedure */
static int MPIR_Intercomm_merge_create_and_map_vcrt(MPID_Comm *comm_ptr, int local_high, MPID_Comm *new_intracomm_ptr);


/* This function creates VCRT for new communicator
 * basing on VCRT of existing communicator.
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Intercomm_merge_create_and_map_vcrt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIR_Intercomm_merge_create_and_map_vcrt(MPID_Comm *comm_ptr, int local_high, MPID_Comm *new_intracomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, j;

    /* Now we know which group comes first.  Build the new vcr
       from the existing vcrs */
    mpi_errno = MPID_VCRT_Create( comm_ptr->local_size + comm_ptr->remote_size, &new_intracomm_ptr->vcrt );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPID_VCRT_Get_ptr( new_intracomm_ptr->vcrt, &new_intracomm_ptr->vcr );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (local_high) {
        /* remote group first */
        j = 0;
        for (i = 0; i < comm_ptr->remote_size; i++) {
            mpi_errno = MPID_VCR_Dup( comm_ptr->vcr[i], &new_intracomm_ptr->vcr[j++] );
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
        for (i = 0; i < comm_ptr->local_size; i++) {
            if (i == comm_ptr->rank) new_intracomm_ptr->rank = j;
            mpi_errno = MPID_VCR_Dup( comm_ptr->local_vcr[i], &new_intracomm_ptr->vcr[j++] );
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }
    else {
        /* local group first */
        j = 0;
        for (i = 0; i < comm_ptr->local_size; i++) {
            if (i == comm_ptr->rank) new_intracomm_ptr->rank = j;
            mpi_errno = MPID_VCR_Dup( comm_ptr->local_vcr[i], &new_intracomm_ptr->vcr[j++] );
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
        for (i = 0; i < comm_ptr->remote_size; i++) {
            mpi_errno = MPID_VCR_Dup( comm_ptr->vcr[i], &new_intracomm_ptr->vcr[j++] );
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }

fn_fail:
    return mpi_errno;
}

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Intercomm_merge
#define MPI_Intercomm_merge PMPI_Intercomm_merge

#undef FUNCNAME
#define FUNCNAME MPIR_Intercomm_merge_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Intercomm_merge_impl(MPID_Comm *comm_ptr, int high, MPID_Comm **new_intracomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int  local_high, remote_high, new_size;
    MPIR_Context_id_t new_context_id;
    int errflag = FALSE;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_INTERCOMM_MERGE_IMPL);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_INTERCOMM_MERGE_IMPL);
    /* Make sure that we have a local intercommunicator */
    if (!comm_ptr->local_comm) {
        /* Manufacture the local communicator */
        mpi_errno = MPIR_Setup_intercomm_localcomm( comm_ptr );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* Find the "high" value of the other group of processes.  This
       will be used to determine which group is ordered first in
       the generated communicator.  high is logical */
    local_high = high;
    if (comm_ptr->rank == 0) {
        /* This routine allows use to use the collective communication
           context rather than the point-to-point context. */
        mpi_errno = MPIC_Sendrecv( &local_high, 1, MPI_INT, 0, 0,
                                      &remote_high, 1, MPI_INT, 0, 0, comm_ptr->handle,
                                      MPI_STATUS_IGNORE, &errflag );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        
        /* If local_high and remote_high are the same, then order is arbitrary.
           we use the gpids of the rank 0 member of the local and remote
           groups to choose an order in this case. */
        if (local_high == remote_high) {
            int ingpid[2], outgpid[2];
            
            mpi_errno = MPID_GPID_Get( comm_ptr, 0, ingpid );
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            
            mpi_errno = MPIC_Sendrecv( ingpid, 2, MPI_INT, 0, 1,
                                          outgpid, 2, MPI_INT, 0, 1, comm_ptr->handle,
                                          MPI_STATUS_IGNORE, &errflag );
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

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
    mpi_errno = MPIR_Bcast_impl( &local_high, 1, MPI_INT, 0, comm_ptr->local_comm, &errflag );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    mpi_errno = MPIR_Comm_create( new_intracomm_ptr );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    new_size = comm_ptr->local_size + comm_ptr->remote_size;

    /* FIXME: For the intracomm, we need a consistent context id.
       That means that one of the two groups needs to use
       the recvcontext_id and the other must use the context_id */
    if (local_high) {
        (*new_intracomm_ptr)->context_id = comm_ptr->recvcontext_id + 2; /* See below */
    }
    else {
        (*new_intracomm_ptr)->context_id = comm_ptr->context_id + 2; /* See below */
    }
    (*new_intracomm_ptr)->recvcontext_id = (*new_intracomm_ptr)->context_id;
    (*new_intracomm_ptr)->remote_size    = (*new_intracomm_ptr)->local_size   = new_size;
    (*new_intracomm_ptr)->rank           = -1;
    (*new_intracomm_ptr)->comm_kind      = MPID_INTRACOMM;

    /* Now we know which group comes first.  Build the new vcr
       from the existing vcrs */
    mpi_errno = MPIR_Intercomm_merge_create_and_map_vcrt(comm_ptr, local_high, (*new_intracomm_ptr));
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* We've setup a temporary context id, based on the context id
       used by the intercomm.  This allows us to perform the allreduce
       operations within the context id algorithm, since we already
       have a valid (almost - see comm_create_hook) communicator.
    */
    mpi_errno = MPIR_Comm_commit((*new_intracomm_ptr));
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* printf( "About to get context id \n" ); fflush( stdout ); */
    /* In the multi-threaded case, MPIR_Get_contextid assumes that the
       calling routine already holds the single criticial section */
    new_context_id = 0;
    mpi_errno = MPIR_Get_contextid( (*new_intracomm_ptr), &new_context_id );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_Assert(new_context_id != 0);

    /* We release this communicator that was involved just to
     * get valid context id and create true one
     */
    mpi_errno = MPIR_Comm_release( (*new_intracomm_ptr),0);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Comm_create( new_intracomm_ptr );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    (*new_intracomm_ptr)->remote_size    = (*new_intracomm_ptr)->local_size   = new_size;
    (*new_intracomm_ptr)->rank           = -1;
    (*new_intracomm_ptr)->comm_kind      = MPID_INTRACOMM;
    (*new_intracomm_ptr)->context_id = new_context_id;
    (*new_intracomm_ptr)->recvcontext_id = new_context_id;

    mpi_errno = MPIR_Intercomm_merge_create_and_map_vcrt(comm_ptr, local_high, (*new_intracomm_ptr));
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Comm_commit((*new_intracomm_ptr));
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_INTERCOMM_MERGE_IMPL);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Intercomm_merge
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)

/*@
MPI_Intercomm_merge - Creates an intracommuncator from an intercommunicator

Input Parameters:
+ intercomm - Intercommunicator (handle)
- high - Used to order the groups within comm (logical)
  when creating the new communicator.  This is a boolean value; the group
  that sets high true has its processes ordered `after` the group that sets 
  this value to false.  If all processes in the intercommunicator provide
  the same value, the choice of which group is ordered first is arbitrary.

Output Parameters:
. newintracomm - Created intracommunicator (handle)

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
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Comm *new_intracomm_ptr;
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
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
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
            int errflag = FALSE;
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

    /* ... body of routine ... */

    mpi_errno = MPIR_Intercomm_merge_impl(comm_ptr, high, &new_intracomm_ptr);
    if (mpi_errno) goto fn_fail;
    
    MPIU_OBJ_PUBLISH_HANDLE(*newintracomm, new_intracomm_ptr->handle);

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
