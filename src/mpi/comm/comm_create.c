/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm) __attribute__((weak,alias("PMPI_Comm_create")));
#endif
/* -- End Profiling Symbol Block */

/* prototypes to make the compiler happy in the case that PMPI_LOCAL expands to
 * nothing instead of "static" */
PMPI_LOCAL int MPIR_Comm_create_inter(MPID_Comm *comm_ptr, MPID_Group *group_ptr,
                                      MPID_Comm **newcomm_ptr);

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_create
#define MPI_Comm_create PMPI_Comm_create

/* This function allocates and calculates an array (*mapping_out) such that
 * (*mapping_out)[i] is the rank in (*mapping_comm) corresponding to local
 * rank i in the given group_ptr.
 *
 * Ownership of the (*mapping_out) array is transferred to the caller who is
 * responsible for freeing it. */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_create_calculate_mapping
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_create_calculate_mapping(MPID_Group  *group_ptr,
                                       MPID_Comm   *comm_ptr,
                                       int        **mapping_out,
                                       MPID_Comm **mapping_comm)
{
    int mpi_errno = MPI_SUCCESS;
    int subsetOfWorld = 0;
    int i, j;
    int n;
    int *mapping=0;
    MPIU_CHKPMEM_DECL(1);
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_CREATE_CALCULATE_MAPPING);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_CREATE_CALCULATE_MAPPING);

    *mapping_out = NULL;
    *mapping_comm = comm_ptr;

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
                              "comm-create - mapping into world[%d] = %d",
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
                mpi_errno = MPIR_Group_check_subset( group_ptr, comm_ptr );
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            MPID_END_ERROR_CHECKS;
        }
#           endif
        /* Override the comm to be used with the mapping array. */
        *mapping_comm = MPIR_Process.comm_world;
    }
    else {
        for (i=0; i<n; i++) {
            /* mapping[i] is the rank in the communicator of the process
               that is the ith element of the group */
            /* FIXME : BUBBLE SORT */
            mapping[i] = -1;
            for (j=0; j<comm_ptr->local_size; j++) {
                int comm_lpid;
                MPID_Comm_get_lpid( comm_ptr, j, &comm_lpid, FALSE );
                if (comm_lpid == group_ptr->lrank_to_lpid[i].lpid) {
                    mapping[i] = j;
                    break;
                }
            }
            MPIR_ERR_CHKANDJUMP1(mapping[i] == -1,mpi_errno,MPI_ERR_GROUP,
                                 "**groupnotincomm", "**groupnotincomm %d", i );
        }
    }

    MPIU_Assert(mapping != NULL);
    *mapping_out     = mapping;
    MPL_VG_CHECK_MEM_IS_DEFINED(*mapping_out, n * sizeof(**mapping_out));

    MPIU_CHKPMEM_COMMIT();
fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_CREATE_CALCULATE_MAPPING);
    return mpi_errno;
fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

/* mapping[i] is equivalent network mapping between the old
 * communicator and the new communicator.  Index 'i' in the old
 * communicator has the same network address as 'mapping[i]' in the
 * new communicator. */
/* WARNING: local_mapping and remote_mapping are stored in this
 * function.  The caller is responsible for their storage and will
 * need to retain them till Comm_commit. */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_create_map
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_create_map(int         local_n,
                         int         remote_n,
                         int        *local_mapping,
                         int        *remote_mapping,
                         MPID_Comm  *mapping_comm,
                         MPID_Comm  *newcomm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Comm_map_irregular(newcomm, mapping_comm, local_mapping,
                            local_n, MPIR_COMM_MAP_DIR_L2L, NULL);
    if (mapping_comm->comm_kind == MPID_INTERCOMM) {
        MPIR_Comm_map_irregular(newcomm, mapping_comm, remote_mapping,
                                remote_n, MPIR_COMM_MAP_DIR_R2R, NULL);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Comm_create_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* comm create impl for intracommunicators, assumes that the standard error
 * checking has already taken place in the calling function */
int MPIR_Comm_create_intra(MPID_Comm *comm_ptr, MPID_Group *group_ptr,
                           MPID_Comm **newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_Context_id_t new_context_id = 0;
    int *mapping = NULL;
    int n;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_CREATE_INTRA);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_CREATE_INTRA);

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTRACOMM);

    n = group_ptr->size;
    *newcomm_ptr = NULL;

    /* Create a new communicator from the specified group members */

    /* Creating the context id is collective over the *input* communicator,
       so it must be created before we decide if this process is a
       member of the group */
    /* In the multi-threaded case, MPIR_Get_contextid_sparse assumes that the
       calling routine already holds the single criticial section */
    mpi_errno = MPIR_Get_contextid_sparse( comm_ptr, &new_context_id,
                                           group_ptr->rank == MPI_UNDEFINED );
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIU_Assert(new_context_id != 0);

    if (group_ptr->rank != MPI_UNDEFINED) {
        MPID_Comm *mapping_comm = NULL;

        mpi_errno = MPIR_Comm_create_calculate_mapping(group_ptr, comm_ptr,
                                                       &mapping, &mapping_comm);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* Get the new communicator structure and context id */

        mpi_errno = MPIR_Comm_create( newcomm_ptr );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        (*newcomm_ptr)->recvcontext_id = new_context_id;
        (*newcomm_ptr)->rank           = group_ptr->rank;
        (*newcomm_ptr)->comm_kind      = comm_ptr->comm_kind;
        /* Since the group has been provided, let the new communicator know
           about the group */
        (*newcomm_ptr)->local_comm     = 0;
        (*newcomm_ptr)->local_group    = group_ptr;
        MPIR_Group_add_ref( group_ptr );

        (*newcomm_ptr)->remote_group   = group_ptr;
        MPIR_Group_add_ref( group_ptr );
        (*newcomm_ptr)->context_id     = (*newcomm_ptr)->recvcontext_id;
        (*newcomm_ptr)->remote_size    = (*newcomm_ptr)->local_size = n;

        /* Setup the communicator's network address mapping.  This is for the remote group,
           which is the same as the local group for intracommunicators */
        mpi_errno = MPIR_Comm_create_map(n, 0,
                                         mapping,
                                         NULL,
                                         mapping_comm,
                                         *newcomm_ptr);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Comm_commit(*newcomm_ptr);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        /* This process is not in the group */
        new_context_id = 0;
    }

fn_exit:
    if (mapping)
        MPIU_Free(mapping);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_CREATE_INTRA);
    return mpi_errno;
fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (*newcomm_ptr != NULL) {
        MPIR_Comm_release(*newcomm_ptr);
        new_context_id = 0; /* MPIR_Comm_release frees the new ctx id */
    }
    if (new_context_id != 0 && group_ptr->rank != MPI_UNDEFINED) {
        MPIR_Free_contextid(new_context_id);
    }
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_create_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* comm create impl for intercommunicators, assumes that the standard error
 * checking has already taken place in the calling function */
PMPI_LOCAL int MPIR_Comm_create_inter(MPID_Comm *comm_ptr, MPID_Group *group_ptr,
                                      MPID_Comm **newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_Context_id_t new_context_id;
    int *mapping = NULL;
    int *remote_mapping = NULL;
    MPID_Comm *mapping_comm = NULL;
    int remote_size = -1;
    int rinfo[2];
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
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
    /* In the multi-threaded case, MPIR_Get_contextid_sparse assumes that the
       calling routine already holds the single criticial section */
    if (!comm_ptr->local_comm) {
        MPIR_Setup_intercomm_localcomm( comm_ptr );
    }
    mpi_errno = MPIR_Get_contextid_sparse( comm_ptr->local_comm, &new_context_id, FALSE );
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIU_Assert(new_context_id != 0);
    MPIU_Assert(new_context_id != comm_ptr->recvcontext_id);

    mpi_errno = MPIR_Comm_create_calculate_mapping(group_ptr, comm_ptr, 
						   &mapping, &mapping_comm);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    *newcomm_ptr = NULL;

    if (group_ptr->rank != MPI_UNDEFINED) {
        /* Get the new communicator structure and context id */
        mpi_errno = MPIR_Comm_create( newcomm_ptr );
        if (mpi_errno) goto fn_fail;

        (*newcomm_ptr)->recvcontext_id = new_context_id;
        (*newcomm_ptr)->rank           = group_ptr->rank;
        (*newcomm_ptr)->comm_kind      = comm_ptr->comm_kind;
        /* Since the group has been provided, let the new communicator know
           about the group */
        (*newcomm_ptr)->local_comm     = 0;
        (*newcomm_ptr)->local_group    = group_ptr;
        MPIR_Group_add_ref( group_ptr );

        (*newcomm_ptr)->local_size   = group_ptr->size;
        (*newcomm_ptr)->remote_group = 0;

        (*newcomm_ptr)->is_low_group = comm_ptr->is_low_group;
    }

    /* There is an additional step.  We must communicate the
       information on the local context id and the group members,
       given by the ranks so that the remote process can construct the
       appropriate network address mapping.
       First we exchange group sizes and context ids.  Then the ranks
       in the remote group, from which the remote network address
       mapping can be constructed.  We need to use the "collective"
       context in the original intercommunicator */
    if (comm_ptr->rank == 0) {
        int info[2];
        info[0] = new_context_id;
        info[1] = group_ptr->size;

        mpi_errno = MPIC_Sendrecv(info, 2, MPI_INT, 0, 0,
                                     rinfo, 2, MPI_INT, 0, 0,
                                     comm_ptr, MPI_STATUS_IGNORE, &errflag );
        if (mpi_errno) { MPIR_ERR_POP( mpi_errno ); }
        if (*newcomm_ptr != NULL) {
            (*newcomm_ptr)->context_id = rinfo[0];
        }
        remote_size = rinfo[1];

        MPIU_CHKLMEM_MALLOC(remote_mapping,int*,
                            remote_size*sizeof(int),
                            mpi_errno,"remote_mapping");

        /* Populate and exchange the ranks */
        mpi_errno = MPIC_Sendrecv( mapping, group_ptr->size, MPI_INT, 0, 0,
                                      remote_mapping, remote_size, MPI_INT, 0, 0,
                                      comm_ptr, MPI_STATUS_IGNORE, &errflag );
        if (mpi_errno) { MPIR_ERR_POP( mpi_errno ); }

        /* Broadcast to the other members of the local group */
        mpi_errno = MPIR_Bcast_impl( rinfo, 2, MPI_INT, 0,
                                     comm_ptr->local_comm, &errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPIR_Bcast_impl( remote_mapping, remote_size, MPI_INT, 0,
                                     comm_ptr->local_comm, &errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    }
    else {
        /* The other processes */
        /* Broadcast to the other members of the local group */
        mpi_errno = MPIR_Bcast_impl( rinfo, 2, MPI_INT, 0,
                                     comm_ptr->local_comm, &errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
        if (*newcomm_ptr != NULL) {
            (*newcomm_ptr)->context_id = rinfo[0];
        }
        remote_size = rinfo[1];
        MPIU_CHKLMEM_MALLOC(remote_mapping,int*,
                            remote_size*sizeof(int),
                            mpi_errno,"remote_mapping");
        mpi_errno = MPIR_Bcast_impl( remote_mapping, remote_size, MPI_INT, 0,
                                     comm_ptr->local_comm, &errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    }

    MPIU_Assert(remote_size >= 0);

    if (group_ptr->rank != MPI_UNDEFINED) {
        (*newcomm_ptr)->remote_size    = remote_size;
        /* Now, everyone has the remote_mapping, and can apply that to
           the network address mapping. */

        /* Setup the communicator's network addresses from the local mapping. */
        mpi_errno = MPIR_Comm_create_map(group_ptr->size,
                                         remote_size,
                                         mapping,
                                         remote_mapping,
                                         mapping_comm,
                                         *newcomm_ptr);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Comm_commit(*newcomm_ptr);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        if (remote_size <= 0) {
            /* It's possible that no members of the other side of comm were
             * members of the group that they passed, which we only know after
             * receiving/bcasting the remote_size above.  We must return
             * MPI_COMM_NULL in this case, but we can't free the newcomm_ptr
             * immediately after the communication above because
             * MPIR_Comm_release won't work correctly with a half-constructed
             * comm. */
            mpi_errno = MPIR_Comm_release(*newcomm_ptr);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            *newcomm_ptr = NULL;
        }
    }
    else {
        /* This process is not in the group */
        MPIR_Free_contextid( new_context_id );
        *newcomm_ptr = NULL;
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
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPI_Comm_create - Creates a new communicator

Input Parameters:
+ comm - communicator (handle) 
- group - group, which is a subset of the group of 'comm'  (handle) 

Output Parameters:
. newcomm - new communicator (handle)

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
    MPID_Comm *comm_ptr = NULL, *newcomm_ptr;
    MPID_Group *group_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_CREATE);

    /* Validate parameters, and convert MPI object handles to object pointers */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;

        MPID_Comm_get_ptr( comm, comm_ptr );

        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            /* If comm_ptr is not valid, it will be reset to null */

            /* only test for MPI_GROUP_NULL after attempting to convert the comm
             * so that any errhandlers on comm will (correctly) be invoked */
            MPIR_ERRTEST_GROUP(group, mpi_errno);
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
        mpi_errno = MPIR_Comm_create_intra(comm_ptr, group_ptr, &newcomm_ptr);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        MPIU_Assert(comm_ptr->comm_kind == MPID_INTERCOMM);
        mpi_errno = MPIR_Comm_create_inter(comm_ptr, group_ptr, &newcomm_ptr);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    if (newcomm_ptr)
        MPID_OBJ_PUBLISH_HANDLE(*newcomm, newcomm_ptr->handle);
    else
        *newcomm = MPI_COMM_NULL;
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
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

