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


/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Intercomm_merge
#define MPI_Intercomm_merge PMPI_Intercomm_merge

/* This function creates mapping for new communicator
 * basing on network addresses of existing communicator.
 */

#undef FUNCNAME
#define FUNCNAME create_and_map
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int create_and_map(MPIR_Comm *comm_ptr, int local_high, MPIR_Comm *new_intracomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    /* Now we know which group comes first.  Build the new mapping
       from the existing comm */
    if (local_high) {
        /* remote group first */
        MPIR_Comm_map_dup(new_intracomm_ptr, comm_ptr, MPIR_COMM_MAP_DIR__R2L);

        MPIR_Comm_map_dup(new_intracomm_ptr, comm_ptr, MPIR_COMM_MAP_DIR__L2L);
        for (i = 0; i < comm_ptr->local_size; i++)
            if (i == comm_ptr->rank)
                new_intracomm_ptr->rank = comm_ptr->remote_size + i;
    }
    else {
        /* local group first */
        MPIR_Comm_map_dup(new_intracomm_ptr, comm_ptr, MPIR_COMM_MAP_DIR__L2L);
        for (i = 0; i < comm_ptr->local_size; i++)
            if (i == comm_ptr->rank)
                new_intracomm_ptr->rank = i;

        MPIR_Comm_map_dup(new_intracomm_ptr, comm_ptr, MPIR_COMM_MAP_DIR__R2L);
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Intercomm_merge_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Intercomm_merge_impl(MPIR_Comm *comm_ptr, int high, MPIR_Comm **new_intracomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int  local_high, remote_high, new_size;
    MPIR_Context_id_t new_context_id;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_COMM_KIND__INTERCOMM_MERGE_IMPL);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_COMM_KIND__INTERCOMM_MERGE_IMPL);
    /* Make sure that we have a local intercommunicator */
    if (!comm_ptr->local_comm) {
        /* Manufacture the local communicator */
        mpi_errno = MPII_Setup_intercomm_localcomm( comm_ptr );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    /* Find the "high" value of the other group of processes.  This
       will be used to determine which group is ordered first in
       the generated communicator.  high is logical */
    local_high = high;
    if (comm_ptr->rank == 0) {
        /* This routine allows use to use the collective communication
           context rather than the point-to-point context. */
        mpi_errno = MPIC_Sendrecv( &local_high, 1, MPI_INT, 0, 0,
                                   &remote_high, 1, MPI_INT, 0, 0, comm_ptr,
                                   MPI_STATUS_IGNORE, &errflag );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* If local_high and remote_high are the same, then order is arbitrary.
           we use the is_low_group in the intercomm in this case. */
        MPL_DBG_MSG_FMT(MPIR_DBG_COMM,VERBOSE,
                        (MPL_DBG_FDEST, "local_high=%d remote_high=%d is_low_group=%d",
                         local_high, remote_high, comm_ptr->is_low_group));
        if (local_high == remote_high) {
            local_high = !(comm_ptr->is_low_group);
        }
    }

    /*
       All processes in the local group now need to get the
       value of local_high, which may have changed if both groups
       of processes had the same value for high
    */
    mpi_errno = MPIR_Bcast_impl( &local_high, 1, MPI_INT, 0, comm_ptr->local_comm, &errflag );
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    /*
     * For the intracomm, we need a consistent context id.
     * That means that one of the two groups needs to use
     * the recvcontext_id and the other must use the context_id
     * The recvcontext_id is unique on each process, but another
     * communicator may use the context_id. Therefore, we do a small hack.
     * We set both flags indicating a sub-communicator (intra-node and
     * inter-node) to one. This is normally not possible (a communicator
     * is either intra- or inter-node) - which makes this context_id unique.
     *
     * There are two path to get it.
     * 1. If the remote and local groups of the intercomm belongs to the same
     *    comm_world. We can merge them into on group and do a
     *    MPIR_Get_contextid_sparse_group on MPI_COMM_WORLD.
     *
     * 2. If not, we fallback to the old "temorary intracomm contextid hack"
     */

    new_size = comm_ptr->local_size + comm_ptr->remote_size;

    if (MPID_INTERCOMM_NO_DYNPROC(comm_ptr)) {
        MPIR_Group *merge_group_ptr = NULL;
        MPIR_Group *comm_world_group = NULL;
        int *lupids;
        int lupid, i;

        MPL_DBG_MSG(MPIR_DBG_COMM,VERBOSE,"Intercomm_merge contextid fastpath");
        lupids = (int*) MPL_malloc(new_size * sizeof(int));

        if (!local_high) {
            for (i = 0; i < comm_ptr->local_size; i++) {
                MPID_Comm_get_lpid(comm_ptr, i, &lupid, FALSE);
                lupids[i] = lupid;
            }
            for (i = 0; i < comm_ptr->remote_size; i++) {
                MPID_Comm_get_lpid(comm_ptr, i, &lupid, TRUE);
                lupids[i+comm_ptr->local_size] = lupid;
            }
        } else {
            for (i = 0; i < comm_ptr->remote_size; i++) {
                MPID_Comm_get_lpid(comm_ptr, i, &lupid, TRUE);
                lupids[i] = lupid;
            }
            for (i = 0; i < comm_ptr->local_size; i++) {
                MPID_Comm_get_lpid(comm_ptr, i, &lupid, FALSE);
                lupids[i+comm_ptr->remote_size] = lupid;
            }
        }
        MPIR_Comm_group_impl(MPIR_Process.comm_world, &comm_world_group);
        MPIR_Group_incl_impl(comm_world_group, new_size, lupids, &merge_group_ptr);

        MPL_free(lupids);

        new_context_id = 0;
        mpi_errno = MPIR_Get_contextid_sparse_group(MPIR_Process.comm_world, merge_group_ptr,
                                                    MPIR_Process.attrs.tag_ub, &new_context_id, FALSE);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_Assert(new_context_id != 0);

        mpi_errno = MPIR_Group_release(comm_world_group);
        mpi_errno = MPIR_Group_release(merge_group_ptr);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    } else {
        mpi_errno = MPIR_Comm_create( new_intracomm_ptr );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        if (local_high) {
            (*new_intracomm_ptr)->context_id = MPIR_CONTEXT_SET_FIELD(SUBCOMM,comm_ptr->recvcontext_id, 3);
        }
        else {
            (*new_intracomm_ptr)->context_id = MPIR_CONTEXT_SET_FIELD(SUBCOMM, comm_ptr->context_id, 3);
        }
        (*new_intracomm_ptr)->recvcontext_id = (*new_intracomm_ptr)->context_id;
        (*new_intracomm_ptr)->remote_size    = (*new_intracomm_ptr)->local_size   = new_size;
        (*new_intracomm_ptr)->rank           = -1;
        (*new_intracomm_ptr)->comm_kind      = MPIR_COMM_KIND__INTRACOMM;

        /* Now we know which group comes first.  Build the new mapping
           from the existing comm */
        mpi_errno = create_and_map(comm_ptr, local_high, (*new_intracomm_ptr));
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* We've setup a temporary context id, based on the context id
           used by the intercomm.  This allows us to perform the allreduce
           operations within the context id algorithm, since we already
           have a valid (almost - see comm_create_hook) communicator.
           */
        mpi_errno = MPIR_Comm_commit((*new_intracomm_ptr));
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* printf( "About to get context id \n" ); fflush( stdout ); */
        /* In the multi-threaded case, MPIR_Get_contextid_sparse assumes that the
           calling routine already holds the single criticial section */
        new_context_id = 0;
        mpi_errno = MPIR_Get_contextid_sparse( (*new_intracomm_ptr), &new_context_id, FALSE );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_Assert(new_context_id != 0);

        /* We release this communicator that was involved just to
         * get valid context id and create true one
         */
        mpi_errno = MPIR_Comm_release(*new_intracomm_ptr);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    mpi_errno = MPIR_Comm_create( new_intracomm_ptr );
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    (*new_intracomm_ptr)->remote_size    = (*new_intracomm_ptr)->local_size   = new_size;
    (*new_intracomm_ptr)->rank           = -1;
    (*new_intracomm_ptr)->comm_kind      = MPIR_COMM_KIND__INTRACOMM;
    (*new_intracomm_ptr)->context_id = new_context_id;
    (*new_intracomm_ptr)->recvcontext_id = new_context_id;

    mpi_errno = create_and_map(comm_ptr, local_high, (*new_intracomm_ptr));
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Comm_commit((*new_intracomm_ptr));
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

 fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_COMM_KIND__INTERCOMM_MERGE_IMPL);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Intercomm_merge
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

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
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Comm *new_intracomm_ptr;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_INTERCOMM_MERGE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);  
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_INTERCOMM_MERGE);

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
    MPIR_Comm_get_ptr( intercomm, comm_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
	    /* If comm_ptr is not valid, it will be reset to null */
	    if (comm_ptr && comm_ptr->comm_kind != MPIR_COMM_KIND__INTERCOMM) {
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
	MPII_Setup_intercomm_localcomm( comm_ptr );
    }

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int acthigh;
            MPIR_Errflag_t errflag = MPIR_ERR_NONE;
	    /* Check for consistent valus of high in each local group.
               The Intel test suite checks for this; it is also an easy
               error to make */
	    acthigh = high ? 1 : 0;   /* Clamp high into 1 or 0 */
	    mpi_errno = MPIR_Allreduce_impl( MPI_IN_PLACE, &acthigh, 1, MPI_INT,
                                             MPI_SUM, comm_ptr->local_comm, &errflag );
	    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
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
    
    MPIR_OBJ_PUBLISH_HANDLE(*newintracomm, new_intracomm_ptr->handle);

    /* ... end of body of routine ... */
    
  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_INTERCOMM_MERGE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
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
