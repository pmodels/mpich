/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_create = PMPI_Comm_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_create  MPI_Comm_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_create as PMPI_Comm_create
#endif
/* -- End Profiling Symbol Block */

/* prototypes to make the compiler happy in the case that PMPI_LOCAL expands to
 * nothing instead of "static" */
PMPI_LOCAL int MPIR_Comm_create_calculate_mapping(MPID_Group  *group_ptr,
                                                  MPID_Comm   *comm_ptr,
                                                  MPID_VCR   **mapping_vcr_out,
                                                  int        **mapping_out);

PMPI_LOCAL int MPIR_Comm_create_create_and_map_vcrt(int n,
                                                    int *mapping,
                                                    MPID_VCR *mapping_vcr,
                                                    MPID_VCRT *out_vcrt,
                                                    MPID_VCR **out_vcr);

PMPI_LOCAL int MPIR_Comm_create_intra(MPID_Comm *comm_ptr, MPID_Group *group_ptr, MPI_Comm *newcomm);
PMPI_LOCAL int MPIR_Comm_create_inter(MPID_Comm *comm_ptr, MPID_Group *group_ptr, MPI_Comm *newcomm);


/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_create
#define MPI_Comm_create PMPI_Comm_create

/* This function allocates and calculates an array (*mapping_out) such that
 * (*mapping_out)[i] is the rank in (*mapping_vcr_out) corresponding to local
 * rank i in the given group_ptr.
 *
 * Ownership of the (*mapping_out) array is transferred to the caller who is
 * responsible for freeing it. */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_create_calculate_mapping
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
PMPI_LOCAL int MPIR_Comm_create_calculate_mapping(MPID_Group  *group_ptr,
                                                  MPID_Comm   *comm_ptr,
                                                  MPID_VCR   **mapping_vcr_out,
                                                  int        **mapping_out)
{
    int mpi_errno = MPI_SUCCESS;
    int subsetOfWorld = 0;
    int i, j;
    int n;
    int *mapping=0;
    int vcr_size;
    MPID_VCR *vcr;
    MPIU_CHKPMEM_DECL(1);
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_CREATE_CALCULATE_MAPPING);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_CREATE_CALCULATE_MAPPING);

    *mapping_out = NULL;
    *mapping_vcr_out = NULL;

    /* N.B. For intracomms only the comm_ptr->vcr is valid and populated,
     * however local_size and remote_size are always set to the same value for
     * intracomms.  For intercomms both are valid and populated, with the
     * local_vcr holding VCs corresponding to the local_group, local_comm, and
     * local_size.
     *
     * For this mapping calculation we always want the logically local vcr,
     * regardless of whether it is stored in the "plain" vcr or local_vcr. */
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
        vcr      = comm_ptr->local_vcr;
        vcr_size = comm_ptr->local_size;
    }
    else {
        vcr      = comm_ptr->vcr;
        vcr_size = comm_ptr->remote_size;
    }

    n = group_ptr->size;
    MPIU_CHKPMEM_MALLOC(mapping,int*,n*sizeof(int),mpi_errno,"mapping");

    /* Make sure that the processes for this group are contained within
       the input communicator.  Also identify the mapping from the ranks of
       the old communicator to the new communicator.
       We do this by matching the lpids of the members of the group
       with the lpids of the members of the input communicator.
       It is an error if the group contains a reference to an lpid that
       does not exist in the communicator.

       An important special case is groups (and communicators) that
       are subsets of MPI_COMM_WORLD.  In this case, the lpids are
       exactly the same as the ranks in comm world.
    */

    /* we examine the group's lpids in both the intracomm and non-comm_world cases */
    MPIR_Group_setup_lpid_list( group_ptr );

    /* Optimize for groups contained within MPI_COMM_WORLD. */
    if (comm_ptr->comm_kind == MPID_INTRACOMM) {
        int wsize;
        subsetOfWorld = 1;
        wsize         = MPIR_Process.comm_world->local_size;
        for (i=0; i<n; i++) {
            int g_lpid = group_ptr->lrank_to_lpid[i].lpid;

            /* This mapping is relative to comm world */
            MPIU_DBG_MSG_FMT(COMM,VERBOSE,
                             (MPIU_DBG_FDEST,
                              "comm-create - mapping into world[%d] = %d\n",
                              i, g_lpid ));
            if (g_lpid < wsize) {
                mapping[i] = g_lpid;
            }
            else {
                subsetOfWorld = 0;
                break;
            }
        }
    }
    MPIU_DBG_MSG_D(COMM,VERBOSE, "subsetOfWorld=%d", subsetOfWorld );
    if (subsetOfWorld) {
#           ifdef HAVE_ERROR_CHECKING
        {
            MPID_BEGIN_ERROR_CHECKS;
            {
                int idx;
                mpi_errno = MPIR_GroupCheckVCRSubset( group_ptr, vcr_size, vcr, &idx );
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            }
            MPID_END_ERROR_CHECKS;
        }
#           endif
        /* Override the vcr to be used with the mapping array. */
        vcr = MPIR_Process.comm_world->vcr;
        vcr_size = MPIR_Process.comm_world->local_size;
    }
    else {
        for (i=0; i<n; i++) {
            /* mapping[i] is the rank in the communicator of the process
               that is the ith element of the group */
            /* FIXME : BUBBLE SORT */
            mapping[i] = -1;
            for (j=0; j<vcr_size; j++) {
                int comm_lpid;
                MPID_VCR_Get_lpid( vcr[j], &comm_lpid );
                if (comm_lpid == group_ptr->lrank_to_lpid[i].lpid) {
                    mapping[i] = j;
                    break;
                }
            }
            MPIU_ERR_CHKANDJUMP1(mapping[i] == -1,mpi_errno,MPI_ERR_GROUP,
                                 "**groupnotincomm", "**groupnotincomm %d", i );
        }
    }

    MPIU_Assert(vcr != NULL);
    MPIU_Assert(mapping != NULL);
    *mapping_vcr_out = vcr;
    *mapping_out     = mapping;
    MPL_VG_CHECK_MEM_IS_DEFINED(*mapping_vcr_out, vcr_size * sizeof(**mapping_vcr_out));
    MPL_VG_CHECK_MEM_IS_DEFINED(*mapping_out, n * sizeof(**mapping_out));

    MPIU_CHKPMEM_COMMIT();
fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_CREATE_CALCULATE_MAPPING);
    return mpi_errno;
fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

/* This function creates a new VCRT and assigns it to out_vcrt, creates a new
 * vcr and assigns it to out_vcr, and then populates it from the mapping_vcr
 * array according to the rank mapping table provided.
 *
 * mapping[i] is the index in the old vcr of index i in the new vcr */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_create_and_map_vcrt
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
PMPI_LOCAL int MPIR_Comm_create_create_and_map_vcrt(int         n,
                                                    int        *mapping,
                                                    MPID_VCR   *mapping_vcr,
                                                    MPID_VCRT  *out_vcrt,
                                                    MPID_VCR  **out_vcr)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPID_VCR *vcr = NULL;

    MPID_VCRT_Create(n, out_vcrt);
    MPID_VCRT_Get_ptr(*out_vcrt, out_vcr);
    vcr = *out_vcr;
    for (i=0; i<n; i++) {
        MPIU_DBG_MSG_FMT(COMM,VERBOSE,
                         (MPIU_DBG_FDEST, "dupping from mapping_vcr=%p rank=%d into new_rank=%d/%d in new_vcr=%p",
                          mapping_vcr, mapping[i], i, n, vcr));
        mpi_errno = MPID_VCR_Dup(mapping_vcr[mapping[i]], &vcr[i]);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

fn_fail:
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Comm_create_intra
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* comm create impl for intracommunicators, assumes that the standard error
 * checking has already taken place in the calling function */
PMPI_LOCAL int MPIR_Comm_create_intra(MPID_Comm *comm_ptr, MPID_Group *group_ptr, MPI_Comm *newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Context_id_t new_context_id = 0;
    MPID_Comm *newcomm_ptr = NULL;
    int *mapping = NULL;
    int n;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_CREATE_INTRA);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_CREATE_INTRA);

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTRACOMM);

    n = group_ptr->size;
    *newcomm = MPI_COMM_NULL;

    /* Create a new communicator from the specified group members */

    /* Creating the context id is collective over the *input* communicator,
       so it must be created before we decide if this process is a
       member of the group */
    /* In the multi-threaded case, MPIR_Get_contextid assumes that the
       calling routine already holds the single criticial section */
    /* TODO should be converted to use MPIR_Get_contextid_sparse instead */
    mpi_errno = MPIR_Get_contextid( comm_ptr, &new_context_id );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_Assert(new_context_id != 0);

    if (group_ptr->rank != MPI_UNDEFINED) {
        MPID_VCR *mapping_vcr = NULL;

        mpi_errno = MPIR_Comm_create_calculate_mapping(group_ptr, comm_ptr, 
						       &mapping_vcr, &mapping);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        /* Get the new communicator structure and context id */

        mpi_errno = MPIR_Comm_create( &newcomm_ptr );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        newcomm_ptr->recvcontext_id = new_context_id;
        newcomm_ptr->rank           = group_ptr->rank;
        newcomm_ptr->comm_kind      = comm_ptr->comm_kind;
        /* Since the group has been provided, let the new communicator know
           about the group */
        newcomm_ptr->local_comm     = 0;
        newcomm_ptr->local_group    = group_ptr;
        MPIR_Group_add_ref( group_ptr );

        newcomm_ptr->remote_group   = group_ptr;
        MPIR_Group_add_ref( group_ptr );
        newcomm_ptr->context_id     = newcomm_ptr->recvcontext_id;
        newcomm_ptr->remote_size    = newcomm_ptr->local_size = n;

        /* Setup the communicator's vc table.  This is for the remote group,
           which is the same as the local group for intracommunicators */
        mpi_errno = MPIR_Comm_create_create_and_map_vcrt(n,
                                                         mapping,
                                                         mapping_vcr,
                                                         &newcomm_ptr->vcrt,
                                                         &newcomm_ptr->vcr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        /* Notify the device of this new communicator */
        MPID_Dev_comm_create_hook( newcomm_ptr );
        mpi_errno = MPIR_Comm_commit(newcomm_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        MPIU_OBJ_PUBLISH_HANDLE(*newcomm, newcomm_ptr->handle);
    }
    else {
        /* This process is not in the group */
        MPIR_Free_contextid( new_context_id );
        new_context_id = 0;
    }

fn_exit:
    if (mapping)
        MPIU_Free(mapping);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_CREATE_INTRA);
    return mpi_errno;
fn_fail:
    if (newcomm_ptr != NULL) {
        MPIR_Comm_release(newcomm_ptr, 0/*isDisconnect*/);
        new_context_id = 0; /* MPIR_Comm_release frees the new ctx id */
    }
    if (new_context_id != 0)
        MPIR_Free_contextid(new_context_id);
    *newcomm = MPI_COMM_NULL;

    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_create_inter
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* comm create impl for intercommunicators, assumes that the standard error
 * checking has already taken place in the calling function */
PMPI_LOCAL int MPIR_Comm_create_inter(MPID_Comm *comm_ptr, MPID_Group *group_ptr, MPI_Comm *newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Context_id_t new_context_id;
    MPID_Comm *newcomm_ptr = NULL;
    MPI_Comm comm = comm_ptr->handle;
    int *mapping = NULL;
    int *remote_mapping = NULL;
    int remote_size = -1;
    int rinfo[2];
    MPID_VCR *mapping_vcr = NULL;
    MPID_VCR *remote_mapping_vcr = NULL;
    int errflag = FALSE;
    MPIU_CHKLMEM_DECL(1);
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_CREATE_INTER);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_CREATE_INTER);

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTERCOMM);

    /* Create a new communicator from the specified group members */

    /* If there is a context id cache in oldcomm, use it here.  Otherwise,
       use the appropriate algorithm to get a new context id. 
       Creating the context id is collective over the *input* communicator,
       so it must be created before we decide if this process is a 
       member of the group */
    /* In the multi-threaded case, MPIR_Get_contextid assumes that the
       calling routine already holds the single criticial section */
    if (!comm_ptr->local_comm) {
        MPIR_Setup_intercomm_localcomm( comm_ptr );
    }
    mpi_errno = MPIR_Get_contextid( comm_ptr->local_comm, &new_context_id );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_Assert(new_context_id != 0);
    MPIU_Assert(new_context_id != comm_ptr->recvcontext_id);

    remote_mapping_vcr = comm_ptr->vcr;

    mpi_errno = MPIR_Comm_create_calculate_mapping(group_ptr, comm_ptr, 
						   &mapping_vcr, &mapping);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (group_ptr->rank != MPI_UNDEFINED) {
        /* Get the new communicator structure and context id */
        mpi_errno = MPIR_Comm_create( &newcomm_ptr );
        if (mpi_errno) goto fn_fail;

        newcomm_ptr->recvcontext_id = new_context_id;
        newcomm_ptr->rank           = group_ptr->rank;
        newcomm_ptr->comm_kind      = comm_ptr->comm_kind;
        /* Since the group has been provided, let the new communicator know
           about the group */
        newcomm_ptr->local_comm     = 0;
        newcomm_ptr->local_group    = group_ptr;
        MPIR_Group_add_ref( group_ptr );

        newcomm_ptr->local_size   = group_ptr->size;
        newcomm_ptr->remote_group = 0;

        newcomm_ptr->is_low_group = comm_ptr->is_low_group;
    }

    /* There is an additional step.  We must communicate the information
       on the local context id and the group members, given by the ranks
       so that the remote process can construct the appropriate VCRT
       First we exchange group sizes and context ids.  Then the
       ranks in the remote group, from which the remote VCRT can
       be constructed.  We need to use the "collective" context in the
       original intercommunicator */
    if (comm_ptr->rank == 0) {
        int info[2];
        info[0] = new_context_id;
        info[1] = group_ptr->size;

        mpi_errno = MPIC_Sendrecv(info, 2, MPI_INT, 0, 0,
                                  rinfo, 2, MPI_INT, 0, 0,
                                  comm, MPI_STATUS_IGNORE );
        if (mpi_errno) { MPIU_ERR_POP( mpi_errno ); }
        if (newcomm_ptr != NULL) {
            newcomm_ptr->context_id = rinfo[0];
        }
        remote_size = rinfo[1];

        MPIU_CHKLMEM_MALLOC(remote_mapping,int*,
                            remote_size*sizeof(int),
                            mpi_errno,"remote_mapping");

        /* Populate and exchange the ranks */
        mpi_errno = MPIC_Sendrecv( mapping, group_ptr->size, MPI_INT, 0, 0,
                                   remote_mapping, remote_size, MPI_INT, 0, 0,
                                   comm, MPI_STATUS_IGNORE );
        if (mpi_errno) { MPIU_ERR_POP( mpi_errno ); }

        /* Broadcast to the other members of the local group */
        mpi_errno = MPIR_Bcast_impl( rinfo, 2, MPI_INT, 0,
                                     comm_ptr->local_comm, &errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        mpi_errno = MPIR_Bcast_impl( remote_mapping, remote_size, MPI_INT, 0,
                                     comm_ptr->local_comm, &errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    }
    else {
        /* The other processes */
        /* Broadcast to the other members of the local group */
        mpi_errno = MPIR_Bcast_impl( rinfo, 2, MPI_INT, 0,
                                     comm_ptr->local_comm, &errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
        if (newcomm_ptr != NULL) {
            newcomm_ptr->context_id = rinfo[0];
        }
        remote_size = rinfo[1];
        MPIU_CHKLMEM_MALLOC(remote_mapping,int*,
                            remote_size*sizeof(int),
                            mpi_errno,"remote_mapping");
        mpi_errno = MPIR_Bcast_impl( remote_mapping, remote_size, MPI_INT, 0,
                                     comm_ptr->local_comm, &errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    }

    MPIU_Assert(remote_size >= 0);

    if (group_ptr->rank != MPI_UNDEFINED) {
        newcomm_ptr->remote_size    = remote_size;
        /* Now, everyone has the remote_mapping, and can apply that to
           the vcr table. */

        /* Setup the communicator's local vc table from the local mapping. */
        mpi_errno = MPIR_Comm_create_create_and_map_vcrt(group_ptr->size,
                                                         mapping,
                                                         mapping_vcr,
                                                         &newcomm_ptr->local_vcrt,
                                                         &newcomm_ptr->local_vcr);

        /* Setup the communicator's vc table.  This is for the remote group */
        mpi_errno = MPIR_Comm_create_create_and_map_vcrt(remote_size,
                                                         remote_mapping,
                                                         remote_mapping_vcr,
                                                         &newcomm_ptr->vcrt,
                                                         &newcomm_ptr->vcr);

        /* Notify the device of this new communicator */
        MPID_Dev_comm_create_hook( newcomm_ptr );
        mpi_errno = MPIR_Comm_commit(newcomm_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        if (remote_size > 0) {
            *newcomm = newcomm_ptr->handle;
        }
        else {
            /* It's possible that no members of the other side of comm were
             * members of the group that they passed, which we only know after
             * receiving/bcasting the remote_size above.  We must return
             * MPI_COMM_NULL in this case, but we can't free the newcomm_ptr
             * immediately after the communication above because
             * MPIR_Comm_release won't work correctly with a half-constructed
             * comm. */
            *newcomm = MPI_COMM_NULL;
            mpi_errno = MPIR_Comm_release(newcomm_ptr, /*isDisconnect=*/FALSE);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }
    else {
        /* This process is not in the group */
        MPIR_Free_contextid( new_context_id );
        *newcomm = MPI_COMM_NULL;
    }

fn_exit:
    MPIU_CHKLMEM_FREEALL();
    if (mapping)
        MPIU_Free(mapping);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_CREATE_INTER);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* !defined(MPICH_MPI_FROM_PMPI) */

#undef FUNCNAME
#define FUNCNAME MPI_Comm_create
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@

MPI_Comm_create - Creates a new communicator

Input Parameters:
+ comm - communicator (handle) 
- group - group, which is a subset of the group of 'comm'  (handle) 

Output Parameter:
. comm_out - new communicator (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_GROUP

.seealso: MPI_Comm_free
@*/
int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Group *group_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_CREATE);

    /* Validate parameters, and convert MPI object handles to object pointers */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
	
	MPID_Comm_get_ptr( comm, comm_ptr );
	
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */

	    MPIR_ERRTEST_GROUP(group, mpi_errno);
            if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
	
	MPID_Group_get_ptr( group, group_ptr );

        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Check the group ptr */
	    MPID_Group_valid_ptr( group_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   else
    {
	MPID_Comm_get_ptr( comm, comm_ptr );
	MPID_Group_get_ptr( group, group_ptr );
    }
#   endif

    /* ... body of routine ...  */
    if (comm_ptr->comm_kind == MPID_INTRACOMM) {
        mpi_errno = MPIR_Comm_create_intra(comm_ptr, group_ptr, newcomm);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
        MPIU_Assert(comm_ptr->comm_kind == MPID_INTERCOMM);
        mpi_errno = MPIR_Comm_create_inter(comm_ptr, group_ptr, newcomm);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_comm_create",
	    "**mpi_comm_create %C %G %p", comm, group, newcomm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

