/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : COMMUNICATOR
      description : cvars that control communicator construction and operation

cvars:
    - name        : MPIR_CVAR_COMM_SPLIT_USE_QSORT
      category    : COMMUNICATOR
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Use qsort(3) in the implementation of MPI_Comm_split instead of bubble sort.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIR_Comm_rank_impl(MPIR_Comm * comm_ptr, int *rank)
{
    *rank = MPIR_Comm_rank(comm_ptr);
    return MPI_SUCCESS;
}

int MPIR_Comm_size_impl(MPIR_Comm * comm_ptr, int *size)
{
    *size = MPIR_Comm_size(comm_ptr);
    return MPI_SUCCESS;
}

int MPIR_Comm_remote_size_impl(MPIR_Comm * comm_ptr, int *size)
{
    *size = comm_ptr->remote_size;
    return MPI_SUCCESS;
}

int MPIR_Comm_set_name_impl(MPIR_Comm * comm_ptr, const char *comm_name)
{
    MPID_THREAD_CS_ENTER(POBJ, comm_ptr->mutex);
    MPID_THREAD_CS_ENTER(VCI, comm_ptr->mutex);
    MPL_strncpy(comm_ptr->name, comm_name, MPI_MAX_OBJECT_NAME);
    MPID_THREAD_CS_EXIT(POBJ, comm_ptr->mutex);
    MPID_THREAD_CS_EXIT(VCI, comm_ptr->mutex);
    return MPI_SUCCESS;
}

int MPIR_Comm_test_inter_impl(MPIR_Comm * comm_ptr, int *flag)
{
    *flag = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM);
    return MPI_SUCCESS;
}

/* used in MPIR_Comm_group_impl and MPIR_Comm_create_group_impl */
static int comm_create_local_group(MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *group_ptr;
    int n = comm_ptr->local_size;

    mpi_errno = MPIR_Group_create(n, &group_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    group_ptr->is_local_dense_monotonic = TRUE;

    int comm_world_size = MPIR_Process.size;
    for (int i = 0; i < n; i++) {
        uint64_t lpid;
        (void) MPID_Comm_get_lpid(comm_ptr, i, &lpid, FALSE);
        group_ptr->lrank_to_lpid[i].lpid = lpid;
        if (lpid > comm_world_size || (i > 0 && group_ptr->lrank_to_lpid[i - 1].lpid != (lpid - 1))) {
            group_ptr->is_local_dense_monotonic = FALSE;
        }
    }

    group_ptr->size = n;
    group_ptr->rank = comm_ptr->rank;
    group_ptr->idx_of_first_lpid = -1;

    comm_ptr->local_group = group_ptr;

    /* FIXME : Add a sanity check that the size of the group is the same as
     * the size of the communicator.  This helps catch corrupted
     * communicators */

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_compare_impl(MPIR_Comm * comm_ptr1, MPIR_Comm * comm_ptr2, int *result)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr1->comm_kind != comm_ptr2->comm_kind) {
        *result = MPI_UNEQUAL;
    } else if (comm_ptr1->handle == comm_ptr2->handle) {
        *result = MPI_IDENT;
    } else if (comm_ptr1->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        MPIR_Group *group_ptr1, *group_ptr2;

        mpi_errno = MPIR_Comm_group_impl(comm_ptr1, &group_ptr1);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Comm_group_impl(comm_ptr2, &group_ptr2);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Group_compare_impl(group_ptr1, group_ptr2, result);
        MPIR_ERR_CHECK(mpi_errno);
        /* If the groups are the same but the contexts are different, then
         * the communicators are congruent */
        if (*result == MPI_IDENT)
            *result = MPI_CONGRUENT;
        mpi_errno = MPIR_Group_free_impl(group_ptr1);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Group_free_impl(group_ptr2);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* INTER_COMM */
        int lresult, rresult;
        MPIR_Group *group_ptr1, *group_ptr2;
        MPIR_Group *rgroup_ptr1, *rgroup_ptr2;

        /* Get the groups and see what their relationship is */
        mpi_errno = MPIR_Comm_group_impl(comm_ptr1, &group_ptr1);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Comm_group_impl(comm_ptr2, &group_ptr2);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Group_compare_impl(group_ptr1, group_ptr2, &lresult);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Comm_remote_group_impl(comm_ptr1, &rgroup_ptr1);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Comm_remote_group_impl(comm_ptr2, &rgroup_ptr2);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Group_compare_impl(rgroup_ptr1, rgroup_ptr2, &rresult);
        MPIR_ERR_CHECK(mpi_errno);

        /* Choose the result that is "least" strong. This works
         * due to the ordering of result types in mpi.h */
        (*result) = (rresult > lresult) ? rresult : lresult;

        /* They can't be identical since they're not the same
         * handle, they are congruent instead */
        if ((*result) == MPI_IDENT)
            (*result) = MPI_CONGRUENT;

        /* Free the groups */
        mpi_errno = MPIR_Group_free_impl(group_ptr1);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Group_free_impl(group_ptr2);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Group_free_impl(rgroup_ptr1);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Group_free_impl(rgroup_ptr2);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This function allocates and calculates an array (*mapping_out) such that
 * (*mapping_out)[i] is the rank in (*mapping_comm) corresponding to local
 * rank i in the given group_ptr.
 *
 * Ownership of the (*mapping_out) array is transferred to the caller who is
 * responsible for freeing it. */
int MPII_Comm_create_calculate_mapping(MPIR_Group * group_ptr,
                                       MPIR_Comm * comm_ptr,
                                       int **mapping_out, MPIR_Comm ** mapping_comm)
{
    int mpi_errno = MPI_SUCCESS;
    int subsetOfWorld = 0;
    int i, j;
    int n;
    int *mapping = 0;
    MPIR_CHKPMEM_DECL(1);

    MPIR_FUNC_ENTER;

    *mapping_out = NULL;
    *mapping_comm = comm_ptr;

    n = group_ptr->size;
    MPIR_CHKPMEM_MALLOC(mapping, int *, n * sizeof(int), mpi_errno, "mapping", MPL_MEM_ADDRESS);

    /* Make sure that the processes for this group are contained within
     * the input communicator.  Also identify the mapping from the ranks of
     * the old communicator to the new communicator.
     * We do this by matching the lpids of the members of the group
     * with the lpids of the members of the input communicator.
     * It is an error if the group contains a reference to an lpid that
     * does not exist in the communicator.
     *
     * An important special case is groups (and communicators) that
     * are subsets of MPI_COMM_WORLD.  In this case, the lpids are
     * exactly the same as the ranks in comm world.
     */

    /* we examine the group's lpids in both the intracomm and non-comm_world cases */
    MPII_Group_setup_lpid_list(group_ptr);

    /* Optimize for groups contained within MPI_COMM_WORLD. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        int wsize;
        subsetOfWorld = 1;
        wsize = MPIR_Process.size;
        for (i = 0; i < n; i++) {
            uint64_t g_lpid = group_ptr->lrank_to_lpid[i].lpid;

            /* This mapping is relative to comm world */
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                            (MPL_DBG_FDEST,
                             "comm-create - mapping into world[%d] = %" PRIu64, i, g_lpid));
            if (g_lpid < wsize) {
                mapping[i] = g_lpid;
            } else {
                subsetOfWorld = 0;
                break;
            }
        }
    }
    MPL_DBG_MSG_D(MPIR_DBG_COMM, VERBOSE, "subsetOfWorld=%d", subsetOfWorld);
    if (subsetOfWorld) {
#ifdef HAVE_ERROR_CHECKING
        {
            MPID_BEGIN_ERROR_CHECKS;
            {
                mpi_errno = MPIR_Group_check_subset(group_ptr, comm_ptr);
                MPIR_ERR_CHECK(mpi_errno);
            }
            MPID_END_ERROR_CHECKS;
        }
#endif
        /* Override the comm to be used with the mapping array. */
        *mapping_comm = MPIR_Process.comm_world;
    } else {
        for (i = 0; i < n; i++) {
            /* mapping[i] is the rank in the communicator of the process
             * that is the ith element of the group */
            /* FIXME : BUBBLE SORT */
            mapping[i] = -1;
            for (j = 0; j < comm_ptr->local_size; j++) {
                uint64_t comm_lpid;
                MPID_Comm_get_lpid(comm_ptr, j, &comm_lpid, FALSE);
                if (comm_lpid == group_ptr->lrank_to_lpid[i].lpid) {
                    mapping[i] = j;
                    break;
                }
            }
            MPIR_ERR_CHKANDJUMP1(mapping[i] == -1, mpi_errno, MPI_ERR_GROUP,
                                 "**groupnotincomm", "**groupnotincomm %d", i);
        }
    }

    MPIR_Assert(mapping != NULL);
    *mapping_out = mapping;
    MPL_VG_CHECK_MEM_IS_DEFINED(*mapping_out, n * sizeof(**mapping_out));

    MPIR_CHKPMEM_COMMIT();
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

/* mapping[i] is equivalent network mapping between the old
 * communicator and the new communicator.  Index 'i' in the old
 * communicator has the same network address as 'mapping[i]' in the
 * new communicator. */
/* WARNING: local_mapping and remote_mapping are stored in this
 * function.  The caller is responsible for their storage and will
 * need to retain them till Comm_commit. */
int MPII_Comm_create_map(int local_n,
                         int remote_n,
                         int *local_mapping,
                         int *remote_mapping, MPIR_Comm * mapping_comm, MPIR_Comm * newcomm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Comm_map_irregular(newcomm, mapping_comm, local_mapping,
                            local_n, MPIR_COMM_MAP_DIR__L2L, NULL);
    if (mapping_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        MPIR_Comm_map_irregular(newcomm, mapping_comm, remote_mapping,
                                remote_n, MPIR_COMM_MAP_DIR__R2R, NULL);
    }
    return mpi_errno;
}


/* comm create impl for intracommunicators, assumes that the standard error
 * checking has already taken place in the calling function */
int MPIR_Comm_create_intra(MPIR_Comm * comm_ptr, MPIR_Group * group_ptr, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Context_id_t new_context_id = 0;
    int *mapping = NULL;
    int n;

    MPIR_FUNC_ENTER;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    n = group_ptr->size;
    *newcomm_ptr = NULL;

    /* Create a new communicator from the specified group members */

    /* Creating the context id is collective over the *input* communicator,
     * so it must be created before we decide if this process is a
     * member of the group */
    /* In the multi-threaded case, MPIR_Get_contextid_sparse assumes that the
     * calling routine already holds the single critical section */
    mpi_errno = MPIR_Get_contextid_sparse(comm_ptr, &new_context_id,
                                          group_ptr->rank == MPI_UNDEFINED);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(new_context_id != 0);

    if (group_ptr->rank != MPI_UNDEFINED) {
        MPIR_Comm *mapping_comm = NULL;

        mpi_errno = MPII_Comm_create_calculate_mapping(group_ptr, comm_ptr,
                                                       &mapping, &mapping_comm);
        MPIR_ERR_CHECK(mpi_errno);

        /* Get the new communicator structure and context id */

        mpi_errno = MPIR_Comm_create(newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        (*newcomm_ptr)->recvcontext_id = new_context_id;
        (*newcomm_ptr)->rank = group_ptr->rank;
        (*newcomm_ptr)->comm_kind = comm_ptr->comm_kind;
        /* Since the group has been provided, let the new communicator know
         * about the group */
        (*newcomm_ptr)->local_comm = 0;
        (*newcomm_ptr)->local_group = group_ptr;
        MPIR_Group_add_ref(group_ptr);

        (*newcomm_ptr)->remote_group = group_ptr;
        MPIR_Group_add_ref(group_ptr);
        (*newcomm_ptr)->context_id = (*newcomm_ptr)->recvcontext_id;
        (*newcomm_ptr)->remote_size = (*newcomm_ptr)->local_size = n;

        /* Setup the communicator's network address mapping.  This is for the remote group,
         * which is the same as the local group for intracommunicators */
        mpi_errno = MPII_Comm_create_map(n, 0, mapping, NULL, mapping_comm, *newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        (*newcomm_ptr)->tainted = comm_ptr->tainted;
        mpi_errno = MPIR_Comm_commit(*newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* This process is not in the group */
        new_context_id = 0;
    }

  fn_exit:
    MPL_free(mapping);

    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (*newcomm_ptr != NULL) {
        MPIR_Comm_release(*newcomm_ptr);
        new_context_id = 0;     /* MPIR_Comm_release frees the new ctx id */
    }
    if (new_context_id != 0 && group_ptr->rank != MPI_UNDEFINED) {
        MPIR_Free_contextid(new_context_id);
    }
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}

/* comm create impl for intercommunicators, assumes that the standard error
 * checking has already taken place in the calling function */
int MPIR_Comm_create_inter(MPIR_Comm * comm_ptr, MPIR_Group * group_ptr, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Context_id_t new_context_id;
    int *mapping = NULL;
    int *remote_mapping = NULL;
    MPIR_Comm *mapping_comm = NULL;
    int remote_size = -1;
    int rinfo[2];
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_CHKLMEM_DECL(1);

    MPIR_FUNC_ENTER;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM);

    /* Create a new communicator from the specified group members */

    /* If there is a context id cache in oldcomm, use it here.  Otherwise,
     * use the appropriate algorithm to get a new context id.
     * Creating the context id is collective over the *input* communicator,
     * so it must be created before we decide if this process is a
     * member of the group */
    /* In the multi-threaded case, MPIR_Get_contextid_sparse assumes that the
     * calling routine already holds the single critical section */
    if (!comm_ptr->local_comm) {
        MPII_Setup_intercomm_localcomm(comm_ptr);
    }
    mpi_errno = MPIR_Get_contextid_sparse(comm_ptr->local_comm, &new_context_id, FALSE);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(new_context_id != 0);
    MPIR_Assert(new_context_id != comm_ptr->recvcontext_id);

    mpi_errno = MPII_Comm_create_calculate_mapping(group_ptr, comm_ptr, &mapping, &mapping_comm);
    MPIR_ERR_CHECK(mpi_errno);

    *newcomm_ptr = NULL;

    if (group_ptr->rank != MPI_UNDEFINED) {
        /* Get the new communicator structure and context id */
        mpi_errno = MPIR_Comm_create(newcomm_ptr);
        if (mpi_errno)
            goto fn_fail;

        (*newcomm_ptr)->recvcontext_id = new_context_id;
        (*newcomm_ptr)->rank = group_ptr->rank;
        (*newcomm_ptr)->comm_kind = comm_ptr->comm_kind;
        /* Since the group has been provided, let the new communicator know
         * about the group */
        (*newcomm_ptr)->local_comm = 0;
        (*newcomm_ptr)->local_group = group_ptr;
        MPIR_Group_add_ref(group_ptr);

        (*newcomm_ptr)->local_size = group_ptr->size;
        (*newcomm_ptr)->remote_group = 0;

        (*newcomm_ptr)->is_low_group = comm_ptr->is_low_group;
    }

    /* There is an additional step.  We must communicate the
     * information on the local context id and the group members,
     * given by the ranks so that the remote process can construct the
     * appropriate network address mapping.
     * First we exchange group sizes and context ids.  Then the ranks
     * in the remote group, from which the remote network address
     * mapping can be constructed.  We need to use the "collective"
     * context in the original intercommunicator */
    if (comm_ptr->rank == 0) {
        int info[2];
        info[0] = new_context_id;
        info[1] = group_ptr->size;

        mpi_errno = MPIC_Sendrecv(info, 2, MPI_INT, 0, 0,
                                  rinfo, 2, MPI_INT, 0, 0, comm_ptr, MPI_STATUS_IGNORE, &errflag);
        MPIR_ERR_CHECK(mpi_errno);
        if (*newcomm_ptr != NULL) {
            (*newcomm_ptr)->context_id = rinfo[0];
        }
        remote_size = rinfo[1];

        MPIR_CHKLMEM_MALLOC(remote_mapping, int *,
                            remote_size * sizeof(int),
                            mpi_errno, "remote_mapping", MPL_MEM_ADDRESS);

        /* Populate and exchange the ranks */
        mpi_errno = MPIC_Sendrecv(mapping, group_ptr->size, MPI_INT, 0, 0,
                                  remote_mapping, remote_size, MPI_INT, 0, 0,
                                  comm_ptr, MPI_STATUS_IGNORE, &errflag);
        MPIR_ERR_CHECK(mpi_errno);

        /* Broadcast to the other members of the local group */
        mpi_errno = MPIR_Bcast(rinfo, 2, MPI_INT, 0, comm_ptr->local_comm, &errflag);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Bcast(remote_mapping, remote_size, MPI_INT, 0,
                               comm_ptr->local_comm, &errflag);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    } else {
        /* The other processes */
        /* Broadcast to the other members of the local group */
        mpi_errno = MPIR_Bcast(rinfo, 2, MPI_INT, 0, comm_ptr->local_comm, &errflag);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
        if (*newcomm_ptr != NULL) {
            (*newcomm_ptr)->context_id = rinfo[0];
        }
        remote_size = rinfo[1];
        MPIR_CHKLMEM_MALLOC(remote_mapping, int *,
                            remote_size * sizeof(int),
                            mpi_errno, "remote_mapping", MPL_MEM_ADDRESS);
        mpi_errno = MPIR_Bcast(remote_mapping, remote_size, MPI_INT, 0,
                               comm_ptr->local_comm, &errflag);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    }

    MPIR_Assert(remote_size >= 0);

    if (group_ptr->rank != MPI_UNDEFINED) {
        (*newcomm_ptr)->remote_size = remote_size;
        /* Now, everyone has the remote_mapping, and can apply that to
         * the network address mapping. */

        /* Setup the communicator's network addresses from the local mapping. */
        mpi_errno = MPII_Comm_create_map(group_ptr->size,
                                         remote_size,
                                         mapping, remote_mapping, mapping_comm, *newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        (*newcomm_ptr)->tainted = comm_ptr->tainted;
        mpi_errno = MPIR_Comm_commit(*newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        if (remote_size <= 0) {
            /* It's possible that no members of the other side of comm were
             * members of the group that they passed, which we only know after
             * receiving/bcasting the remote_size above.  We must return
             * MPI_COMM_NULL in this case, but we can't free the newcomm_ptr
             * immediately after the communication above because
             * MPIR_Comm_release won't work correctly with a half-constructed
             * comm. */
            mpi_errno = MPIR_Comm_release(*newcomm_ptr);
            MPIR_ERR_CHECK(mpi_errno);
            *newcomm_ptr = NULL;
        }
    } else {
        /* This process is not in the group */
        MPIR_Free_contextid(new_context_id);
        *newcomm_ptr = NULL;
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPL_free(mapping);

    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_create_impl(MPIR_Comm * comm_ptr, MPIR_Group * group_ptr, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIR_Comm_create_intra(comm_ptr, group_ptr, newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM);
        mpi_errno = MPIR_Comm_create_inter(comm_ptr, group_ptr, newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* comm create group impl; assumes that the standard error checking
 * has already taken place in the calling function */
int MPIR_Comm_create_group_impl(MPIR_Comm * comm_ptr, MPIR_Group * group_ptr, int tag,
                                MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Context_id_t new_context_id = 0;
    int *mapping = NULL;
    int n;

    MPIR_FUNC_ENTER;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    n = group_ptr->size;
    *newcomm_ptr = NULL;

    /* Shift tag into the tagged coll space */
    tag |= MPIR_TAG_COLL_BIT;

    /* Create a new communicator from the specified group members */

    if (group_ptr->rank != MPI_UNDEFINED) {
        MPIR_Comm *mapping_comm = NULL;

        /* For this routine, creation of the id is collective over the input
         *group*, so processes not in the group do not participate. */

        mpi_errno = MPIR_Get_contextid_sparse_group(comm_ptr, group_ptr, tag, &new_context_id, 0);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(new_context_id != 0);

        mpi_errno = MPII_Comm_create_calculate_mapping(group_ptr, comm_ptr,
                                                       &mapping, &mapping_comm);
        MPIR_ERR_CHECK(mpi_errno);

        /* Get the new communicator structure and context id */

        mpi_errno = MPIR_Comm_create(newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        (*newcomm_ptr)->recvcontext_id = new_context_id;
        (*newcomm_ptr)->rank = group_ptr->rank;
        (*newcomm_ptr)->comm_kind = comm_ptr->comm_kind;
        /* Since the group has been provided, let the new communicator know
         * about the group */
        (*newcomm_ptr)->local_comm = 0;
        (*newcomm_ptr)->local_group = group_ptr;
        MPIR_Group_add_ref(group_ptr);

        (*newcomm_ptr)->remote_group = group_ptr;
        MPIR_Group_add_ref(group_ptr);
        (*newcomm_ptr)->context_id = (*newcomm_ptr)->recvcontext_id;
        (*newcomm_ptr)->remote_size = (*newcomm_ptr)->local_size = n;

        /* Setup the communicator's vc table.  This is for the remote group,
         * which is the same as the local group for intracommunicators */
        mpi_errno = MPII_Comm_create_map(n, 0, mapping, NULL, mapping_comm, *newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        (*newcomm_ptr)->tainted = comm_ptr->tainted;
        mpi_errno = MPIR_Comm_commit(*newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* This process is not in the group */
        new_context_id = 0;
    }

  fn_exit:
    MPL_free(mapping);

    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (*newcomm_ptr != NULL) {
        MPIR_Comm_release(*newcomm_ptr);
        new_context_id = 0;     /* MPIR_Comm_release frees the new ctx id */
    }
    if (new_context_id != 0)
        MPIR_Free_contextid(new_context_id);
    /* --END ERROR HANDLING-- */

    goto fn_exit;
}

int MPIR_Comm_dup_impl(MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Comm_dup(comm_ptr, NULL, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Comm_copy_stream(comm_ptr, *newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_dup_with_info_impl(MPIR_Comm * comm_ptr, MPIR_Info * info, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Comm_dup(comm_ptr, info, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Comm_copy_stream(comm_ptr, *newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIR_Comm_create_from_group_impl uses a string as tag (from MPI specification), we
 * need translate the string into an integer tag using hash function */
static int get_tag_from_stringtag(const char *stringtag)
{
    unsigned hash;
    int n = strlen(stringtag);
    HASH_VALUE(stringtag, n, hash);

    return hash % (MPIR_Process.attrs.tag_ub);
}

static bool is_world_group(MPIR_Group * group_ptr)
{
    return (group_ptr->size == MPIR_Process.size && group_ptr->size > 1);
}

static bool is_self_group(MPIR_Group * group_ptr)
{
    return (group_ptr->size == 1);
}

int MPIR_Comm_create_from_group_impl(MPIR_Group * group_ptr, const char *stringtag,
                                     MPIR_Info * info_ptr, MPIR_Errhandler * errhan_ptr,
                                     MPIR_Comm ** p_newcom_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int use_comm_world = 0;

    /* NOTE: only world or self is supported without first establishing comm_world.  */

    MPL_initlock_lock(&MPIR_init_lock);
    if (MPIR_Process.comm_world) {
        use_comm_world = 1;
    } else if (is_world_group(group_ptr)) {
        mpi_errno = MPIR_init_comm_world();
        use_comm_world = 1;
    } else if (!MPIR_Process.comm_self && is_self_group(group_ptr)) {
        mpi_errno = MPIR_init_comm_self();
    }
    MPL_initlock_unlock(&MPIR_init_lock);
    MPIR_ERR_CHECK(mpi_errno);

    if (use_comm_world) {
        /* NOTE: tag will be used with MPIR_TAG_COLL_BIT on, ref. MPIR_Get_contextid_sparse_group */
        int tag = get_tag_from_stringtag(stringtag);

        /* Because the group_ptr may not be derived from a communicator, local_group in
         * comm_world may not have been created */
        static MPL_initlock_t lock = MPL_INITLOCK_INITIALIZER;
        MPL_initlock_lock(&lock);
        if (!MPIR_Process.comm_world->local_group) {
            mpi_errno = comm_create_local_group(MPIR_Process.comm_world);
        }
        MPL_initlock_unlock(&lock);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Comm_create_group_impl(MPIR_Process.comm_world, group_ptr, tag, p_newcom_ptr);
    } else {
        /* Currently only self comm is allowed here */
        MPIR_Assert(is_self_group(group_ptr));

        mpi_errno = MPIR_Comm_dup_impl(MPIR_Process.comm_self, p_newcom_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (*p_newcom_ptr) {
        if (info_ptr) {
            MPII_Comm_set_hints(*p_newcom_ptr, info_ptr, true);
        }

        if (errhan_ptr) {
            MPIR_Comm_set_errhandler_impl(*p_newcom_ptr, errhan_ptr);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* a restricted implementation of MPI_Intercomm_create_from_groups.
 * Require comm_world, and remote_group part of comm_world.
 * TODO: remote_group from different comm_world
 */
int MPIR_Intercomm_create_from_groups_impl(MPIR_Group * local_group_ptr, int local_leader,
                                           MPIR_Group * remote_group_ptr, int remote_leader,
                                           const char *stringtag,
                                           MPIR_Info * info_ptr, MPIR_Errhandler * errhan_ptr,
                                           MPIR_Comm ** p_newintercom_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIR_Process.comm_world);

    MPIR_Comm *local_comm;
    mpi_errno = MPIR_Comm_create_from_group_impl(local_group_ptr, stringtag, info_ptr, errhan_ptr,
                                                 &local_comm);
    MPIR_ERR_CHECK(mpi_errno);

    int tag = get_tag_from_stringtag(stringtag);
    /* FIXME: ensure lpid is from comm_world */
    uint64_t remote_lpid = remote_group_ptr->lrank_to_lpid[remote_leader].lpid;
    MPIR_Assert(remote_lpid < MPIR_Process.size);
    mpi_errno = MPIR_Intercomm_create_impl(local_comm, local_leader,
                                           MPIR_Process.comm_world, (int) remote_lpid,
                                           tag, p_newintercom_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Comm_release(local_comm);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_free_impl(MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Comm_release(comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    if (comm_ptr == MPIR_Process.comm_parent) {
        MPIR_Process.comm_parent = NULL;
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_get_info_impl(MPIR_Comm * comm_ptr, MPIR_Info ** info_p_p)
{
    int mpi_errno = MPI_SUCCESS;

    /* Allocate an empty info object */
    mpi_errno = MPIR_Info_alloc(info_p_p);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    MPII_Comm_get_hints(comm_ptr, *info_p_p);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_get_name_impl(MPIR_Comm * comm_ptr, char *comm_name, int *resultlen)
{
    /* The user must allocate a large enough section of memory */
    MPL_strncpy(comm_name, comm_ptr->name, MPI_MAX_OBJECT_NAME);
    *resultlen = (int) strlen(comm_name);
    return MPI_SUCCESS;
}

int MPIR_Comm_group_impl(MPIR_Comm * comm_ptr, MPIR_Group ** group_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;
    /* Create a local group if necessary */
    if (!comm_ptr->local_group) {
        mpi_errno = comm_create_local_group(comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }
    *group_ptr = comm_ptr->local_group;

    MPIR_Group_add_ref(*group_ptr);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

int MPIR_Comm_idup_impl(MPIR_Comm * comm_ptr, MPIR_Comm ** newcommp, MPIR_Request ** reqp)
{
    return MPIR_Comm_idup_with_info_impl(comm_ptr, NULL, newcommp, reqp);
}

int MPIR_Comm_idup_with_info_impl(MPIR_Comm * comm_ptr, MPIR_Info * info,
                                  MPIR_Comm ** newcommp, MPIR_Request ** reqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Attribute *new_attributes = 0;

    /* Copy attributes, executing the attribute copy functions */
    /* This accesses the attribute dup function through the perprocess
     * structure to prevent comm_dup from forcing the linking of the
     * attribute functions.  The actual function is (by default)
     * MPIR_Attr_dup_list
     */
    if (MPIR_Process.attr_dup) {
        mpi_errno = MPIR_Process.attr_dup(comm_ptr->handle, comm_ptr->attributes, &new_attributes);
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPII_Comm_copy_data(comm_ptr, info, newcommp);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Comm_copy_stream(comm_ptr, *newcommp);
    MPIR_ERR_CHECK(mpi_errno);

    (*newcommp)->attributes = new_attributes;

    /* We now have a mostly-valid new communicator, so begin the process of
     * allocating a context ID to use for actual communication */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        mpi_errno = MPIR_Get_intercomm_contextid_nonblock(comm_ptr, *newcommp, reqp);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno = MPIR_Get_contextid_nonblock(comm_ptr, *newcommp, reqp);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_remote_group_impl(MPIR_Comm * comm_ptr, MPIR_Group ** group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, n;

    MPIR_FUNC_ENTER;
    /* Create a group and populate it with the local process ids */
    if (!comm_ptr->remote_group) {
        n = comm_ptr->remote_size;
        mpi_errno = MPIR_Group_create(n, group_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        for (i = 0; i < n; i++) {
            uint64_t lpid;
            (void) MPID_Comm_get_lpid(comm_ptr, i, &lpid, TRUE);
            (*group_ptr)->lrank_to_lpid[i].lpid = lpid;
            /* TODO calculate is_local_dense_monotonic */
        }
        (*group_ptr)->size = n;
        (*group_ptr)->rank = MPI_UNDEFINED;
        (*group_ptr)->idx_of_first_lpid = -1;

        comm_ptr->remote_group = *group_ptr;
    } else {
        *group_ptr = comm_ptr->remote_group;
    }
    MPIR_Group_add_ref(comm_ptr->remote_group);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

int MPIR_Comm_set_info_impl(MPIR_Comm * comm_ptr, MPIR_Info * info_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPII_Comm_set_hints(comm_ptr, info_ptr, false);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Intercomm_create_impl(MPIR_Comm * local_comm_ptr, int local_leader,
                               MPIR_Comm * peer_comm_ptr, int remote_leader, int tag,
                               MPIR_Comm ** new_intercomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Context_id_t final_context_id, recvcontext_id;
    int remote_size = 0;
    uint64_t *remote_lpids = NULL;
    int comm_info[3];
    int is_low_group = 0;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    /* Shift tag into the tagged coll space */
    tag |= MPIR_TAG_COLL_BIT;

    mpi_errno = MPID_Intercomm_exchange_map(local_comm_ptr, local_leader,
                                            peer_comm_ptr, remote_leader,
                                            &remote_size, &remote_lpids, &is_low_group);
    MPIR_ERR_CHECK(mpi_errno);

    /*
     * Create the contexts.  Each group will have a context for sending
     * to the other group. All processes must be involved.  Because
     * we know that the local and remote groups are disjoint, this
     * step will complete
     */
    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                    (MPL_DBG_FDEST, "About to get contextid (local_size=%d) on rank %d",
                     local_comm_ptr->local_size, local_comm_ptr->rank));
    /* In the multi-threaded case, MPIR_Get_contextid_sparse assumes that the
     * calling routine already holds the single critical section */
    /* TODO: Make sure this is tag-safe */
    mpi_errno = MPIR_Get_contextid_sparse(local_comm_ptr, &recvcontext_id, FALSE);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(recvcontext_id != 0);
    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "Got contextid=%d", recvcontext_id));

    /* Leaders can now swap context ids and then broadcast the value
     * to the local group of processes */
    if (local_comm_ptr->rank == local_leader) {
        MPIR_Context_id_t remote_context_id;

        mpi_errno =
            MPIC_Sendrecv(&recvcontext_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, remote_leader, tag,
                          &remote_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, remote_leader, tag,
                          peer_comm_ptr, MPI_STATUS_IGNORE, &errflag);
        MPIR_ERR_CHECK(mpi_errno);

        final_context_id = remote_context_id;

        /* Now, send all of our local processes the remote_lpids,
         * along with the final context id */
        comm_info[0] = final_context_id;
        MPL_DBG_MSG(MPIR_DBG_COMM, VERBOSE, "About to bcast on local_comm");
        mpi_errno = MPIR_Bcast(comm_info, 1, MPI_INT, local_leader, local_comm_ptr, &errflag);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
        MPL_DBG_MSG_D(MPIR_DBG_COMM, VERBOSE, "end of bcast on local_comm of size %d",
                      local_comm_ptr->local_size);
    } else {
        /* we're the other processes */
        MPL_DBG_MSG(MPIR_DBG_COMM, VERBOSE, "About to receive bcast on local_comm");
        mpi_errno = MPIR_Bcast(comm_info, 1, MPI_INT, local_leader, local_comm_ptr, &errflag);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* Extract the context and group sign information */
        final_context_id = comm_info[0];
    }

    /* At last, we now have the information that we need to build the
     * intercommunicator */

    /* All processes in the local_comm now build the communicator */

    mpi_errno = MPIR_Comm_create(new_intercomm_ptr);
    if (mpi_errno)
        goto fn_fail;

    (*new_intercomm_ptr)->context_id = final_context_id;
    (*new_intercomm_ptr)->recvcontext_id = recvcontext_id;
    (*new_intercomm_ptr)->remote_size = remote_size;
    (*new_intercomm_ptr)->local_size = local_comm_ptr->local_size;
    (*new_intercomm_ptr)->rank = local_comm_ptr->rank;
    (*new_intercomm_ptr)->comm_kind = MPIR_COMM_KIND__INTERCOMM;
    (*new_intercomm_ptr)->local_comm = 0;
    (*new_intercomm_ptr)->is_low_group = is_low_group;

    mpi_errno = MPID_Create_intercomm_from_lpids(*new_intercomm_ptr, remote_size, remote_lpids);
    if (mpi_errno)
        goto fn_fail;

    MPIR_Comm_map_dup(*new_intercomm_ptr, local_comm_ptr, MPIR_COMM_MAP_DIR__L2L);

    /* Inherit the error handler (if any) */
    MPID_THREAD_CS_ENTER(POBJ, local_comm_ptr->mutex);
    MPID_THREAD_CS_ENTER(VCI, local_comm_ptr->mutex);
    (*new_intercomm_ptr)->errhandler = local_comm_ptr->errhandler;
    if (local_comm_ptr->errhandler) {
        MPIR_Errhandler_add_ref(local_comm_ptr->errhandler);
    }
    MPID_THREAD_CS_EXIT(POBJ, local_comm_ptr->mutex);
    MPID_THREAD_CS_EXIT(VCI, local_comm_ptr->mutex);

    (*new_intercomm_ptr)->tainted = 1;
    mpi_errno = MPIR_Comm_commit(*new_intercomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);


  fn_exit:
    MPL_free(remote_lpids);
    remote_lpids = NULL;
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Peer intercomm is a 1-to-1 intercomm, internally created by device layer
 * to facilitate connecting dynamic processes */

int MPIR_peer_intercomm_create(MPIR_Context_id_t context_id, MPIR_Context_id_t recvcontext_id,
                               uint64_t remote_lpid, int is_low_group, MPIR_Comm ** newcomm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Comm_create(newcomm);
    MPIR_ERR_CHECK(mpi_errno);

    (*newcomm)->context_id = context_id;
    (*newcomm)->recvcontext_id = recvcontext_id;
    (*newcomm)->remote_size = 1;
    (*newcomm)->local_size = 1;
    (*newcomm)->rank = 0;
    (*newcomm)->comm_kind = MPIR_COMM_KIND__INTERCOMM;
    (*newcomm)->local_comm = 0;
    (*newcomm)->is_low_group = is_low_group;

    mpi_errno = MPID_Create_intercomm_from_lpids(*newcomm, 1, &remote_lpid);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Comm *comm_self = MPIR_Process.comm_self;
    MPIR_Comm_map_dup(*newcomm, comm_self, MPIR_COMM_MAP_DIR__L2L);

    /* Inherit the error handler  */
    MPID_THREAD_CS_ENTER(POBJ, comm_self->mutex);
    MPID_THREAD_CS_ENTER(VCI, comm_self->mutex);
    (*newcomm)->errhandler = comm_self->errhandler;
    if (comm_self->errhandler) {
        MPIR_Errhandler_add_ref(comm_self->errhandler);
    }
    MPID_THREAD_CS_EXIT(POBJ, comm_self->mutex);
    MPID_THREAD_CS_EXIT(VCI, comm_self->mutex);

    (*newcomm)->tainted = 1;
    mpi_errno = MPIR_Comm_commit(*newcomm);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This function creates mapping for new communicator
 * basing on network addresses of existing communicator.
 */

static int create_and_map(MPIR_Comm * comm_ptr, int local_high, MPIR_Comm * new_intracomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    /* Now we know which group comes first.  Build the new mapping
     * from the existing comm */
    if (local_high) {
        /* remote group first */
        MPIR_Comm_map_dup(new_intracomm_ptr, comm_ptr, MPIR_COMM_MAP_DIR__R2L);

        MPIR_Comm_map_dup(new_intracomm_ptr, comm_ptr, MPIR_COMM_MAP_DIR__L2L);
        for (i = 0; i < comm_ptr->local_size; i++)
            if (i == comm_ptr->rank)
                new_intracomm_ptr->rank = comm_ptr->remote_size + i;
    } else {
        /* local group first */
        MPIR_Comm_map_dup(new_intracomm_ptr, comm_ptr, MPIR_COMM_MAP_DIR__L2L);
        for (i = 0; i < comm_ptr->local_size; i++)
            if (i == comm_ptr->rank)
                new_intracomm_ptr->rank = i;

        MPIR_Comm_map_dup(new_intracomm_ptr, comm_ptr, MPIR_COMM_MAP_DIR__R2L);
    }

    return mpi_errno;
}

int MPIR_Intercomm_merge_impl(MPIR_Comm * comm_ptr, int high, MPIR_Comm ** new_intracomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int local_high, remote_high, new_size;
    MPIR_Context_id_t new_context_id;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;
    /* Make sure that we have a local intercommunicator */
    if (!comm_ptr->local_comm) {
        /* Manufacture the local communicator */
        mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Find the "high" value of the other group of processes.  This
     * will be used to determine which group is ordered first in
     * the generated communicator.  high is logical */
    local_high = high;
    if (comm_ptr->rank == 0) {
        /* This routine allows use to use the collective communication
         * context rather than the point-to-point context. */
        mpi_errno = MPIC_Sendrecv(&local_high, 1, MPI_INT, 0, 0,
                                  &remote_high, 1, MPI_INT, 0, 0, comm_ptr,
                                  MPI_STATUS_IGNORE, &errflag);
        MPIR_ERR_CHECK(mpi_errno);

        /* If local_high and remote_high are the same, then order is arbitrary.
         * we use the is_low_group in the intercomm in this case. */
        MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                        (MPL_DBG_FDEST, "local_high=%d remote_high=%d is_low_group=%d",
                         local_high, remote_high, comm_ptr->is_low_group));
        if (local_high == remote_high) {
            local_high = !(comm_ptr->is_low_group);
        }
    }

    /*
     * All processes in the local group now need to get the
     * value of local_high, which may have changed if both groups
     * of processes had the same value for high
     */
    mpi_errno = MPIR_Bcast(&local_high, 1, MPI_INT, 0, comm_ptr->local_comm, &errflag);
    MPIR_ERR_CHECK(mpi_errno);
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
     */

    new_size = comm_ptr->local_size + comm_ptr->remote_size;

    mpi_errno = MPIR_Comm_create(new_intracomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    if (local_high) {
        (*new_intracomm_ptr)->context_id =
            MPIR_CONTEXT_SET_FIELD(SUBCOMM, comm_ptr->recvcontext_id, 3);
    } else {
        (*new_intracomm_ptr)->context_id = MPIR_CONTEXT_SET_FIELD(SUBCOMM, comm_ptr->context_id, 3);
    }
    (*new_intracomm_ptr)->recvcontext_id = (*new_intracomm_ptr)->context_id;
    (*new_intracomm_ptr)->remote_size = (*new_intracomm_ptr)->local_size = new_size;
    (*new_intracomm_ptr)->rank = -1;
    (*new_intracomm_ptr)->comm_kind = MPIR_COMM_KIND__INTRACOMM;

    /* Now we know which group comes first.  Build the new mapping
     * from the existing comm */
    mpi_errno = create_and_map(comm_ptr, local_high, (*new_intracomm_ptr));
    MPIR_ERR_CHECK(mpi_errno);

    /* We've setup a temporary context id, based on the context id
     * used by the intercomm.  This allows us to perform the allreduce
     * operations within the context id algorithm, since we already
     * have a valid (almost - see comm_create_hook) communicator.
     */
    (*new_intracomm_ptr)->tainted = 1;
    mpi_errno = MPIR_Comm_commit((*new_intracomm_ptr));
    MPIR_ERR_CHECK(mpi_errno);

    /* printf("About to get context id \n"); fflush(stdout); */
    /* In the multi-threaded case, MPIR_Get_contextid_sparse assumes that the
     * calling routine already holds the single critical section */
    new_context_id = 0;
    mpi_errno = MPIR_Get_contextid_sparse((*new_intracomm_ptr), &new_context_id, FALSE);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(new_context_id != 0);

    /* We release this communicator that was involved just to
     * get valid context id and create true one
     */
    mpi_errno = MPIR_Comm_release(*new_intracomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Comm_create(new_intracomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    (*new_intracomm_ptr)->remote_size = (*new_intracomm_ptr)->local_size = new_size;
    (*new_intracomm_ptr)->rank = -1;
    (*new_intracomm_ptr)->comm_kind = MPIR_COMM_KIND__INTRACOMM;
    (*new_intracomm_ptr)->context_id = new_context_id;
    (*new_intracomm_ptr)->recvcontext_id = new_context_id;

    mpi_errno = create_and_map(comm_ptr, local_high, (*new_intracomm_ptr));
    MPIR_ERR_CHECK(mpi_errno);

    (*new_intracomm_ptr)->tainted = 1;
    mpi_errno = MPIR_Comm_commit((*new_intracomm_ptr));
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
