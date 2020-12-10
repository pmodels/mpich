/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


int MPIR_Comm_agree_impl(MPIR_Comm * comm_ptr, int *flag)
{
    int mpi_errno = MPI_SUCCESS, mpi_errno_tmp = MPI_SUCCESS;
    MPIR_Group *comm_grp = NULL, *failed_grp = NULL, *new_group_ptr = NULL, *global_failed = NULL;
    int result, success = 1;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int values[2];

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_COMM_AGREE);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_COMM_AGREE);

    MPIR_Comm_group_impl(comm_ptr, &comm_grp);

    /* Get the locally known (not acknowledged) group of failed procs */
    mpi_errno = MPID_Comm_failure_get_acked(comm_ptr, &failed_grp);
    MPIR_ERR_CHECK(mpi_errno);

    /* First decide on the group of failed procs. */
    mpi_errno = MPID_Comm_get_all_failed_procs(comm_ptr, &global_failed, MPIR_AGREE_TAG);
    if (mpi_errno)
        errflag = MPIR_ERR_PROC_FAILED;

    mpi_errno = MPIR_Group_compare_impl(failed_grp, global_failed, &result);
    MPIR_ERR_CHECK(mpi_errno);

    /* Create a subgroup without the failed procs */
    mpi_errno = MPIR_Group_difference_impl(comm_grp, global_failed, &new_group_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* If that group isn't the same as what we think is failed locally, then
     * mark it as such. */
    if (result == MPI_UNEQUAL || errflag)
        success = 0;

    /* Do an allreduce to decide whether or not anyone thinks the group
     * has changed */
    mpi_errno_tmp = MPII_Allreduce_group(MPI_IN_PLACE, &success, 1, MPI_INT, MPI_MIN, comm_ptr,
                                         new_group_ptr, MPIR_AGREE_TAG, &errflag);
    if (!success || errflag || mpi_errno_tmp)
        success = 0;

    values[0] = success;
    values[1] = *flag;

    /* Determine both the result of this function (mpi_errno) and the result
     * of flag that will be returned to the user. */
    MPII_Allreduce_group(MPI_IN_PLACE, values, 2, MPI_INT, MPI_BAND, comm_ptr,
                         new_group_ptr, MPIR_AGREE_TAG, &errflag);
    /* Ignore the result of the operation this time. Everyone will either
     * return a failure because of !success earlier or they will return
     * something useful for flag because of this operation. If there was a new
     * failure in between the first allreduce and the second one, it's ignored
     * here. */

    if (failed_grp != MPIR_Group_empty)
        MPIR_Group_release(failed_grp);
    MPIR_Group_release(new_group_ptr);
    MPIR_Group_release(comm_grp);
    if (global_failed != MPIR_Group_empty)
        MPIR_Group_release(global_failed);

    success = values[0];
    *flag = values[1];

    if (!success) {
        MPIR_ERR_SET(mpi_errno_tmp, MPIX_ERR_PROC_FAILED, "**mpix_comm_agree");
        MPIR_ERR_ADD(mpi_errno, mpi_errno_tmp);
    }

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_COMM_AGREE);
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
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_COMM_CREATE_CALCULATE_MAPPING);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_COMM_CREATE_CALCULATE_MAPPING);

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
        wsize = MPIR_Process.comm_world->local_size;
        for (i = 0; i < n; i++) {
            int g_lpid = group_ptr->lrank_to_lpid[i].lpid;

            /* This mapping is relative to comm world */
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                            (MPL_DBG_FDEST,
                             "comm-create - mapping into world[%d] = %d", i, g_lpid));
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
                int comm_lpid;
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
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_COMM_CREATE_CALCULATE_MAPPING);
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
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_COMM_CREATE_INTRA);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_COMM_CREATE_INTRA);

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    n = group_ptr->size;
    *newcomm_ptr = NULL;

    /* Create a new communicator from the specified group members */

    /* Creating the context id is collective over the *input* communicator,
     * so it must be created before we decide if this process is a
     * member of the group */
    /* In the multi-threaded case, MPIR_Get_contextid_sparse assumes that the
     * calling routine already holds the single criticial section */
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

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_COMM_CREATE_INTRA);
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
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_COMM_CREATE_INTER);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_COMM_CREATE_INTER);

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM);

    /* Create a new communicator from the specified group members */

    /* If there is a context id cache in oldcomm, use it here.  Otherwise,
     * use the appropriate algorithm to get a new context id.
     * Creating the context id is collective over the *input* communicator,
     * so it must be created before we decide if this process is a
     * member of the group */
    /* In the multi-threaded case, MPIR_Get_contextid_sparse assumes that the
     * calling routine already holds the single criticial section */
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

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_COMM_CREATE_INTER);
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

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_COMM_CREATE_GROUP);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_COMM_CREATE_GROUP);

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

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_COMM_CREATE_GROUP);
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
    return MPIR_Comm_dup_with_info_impl(comm_ptr, NULL, newcomm_ptr);
}

int MPIR_Comm_dup_with_info_impl(MPIR_Comm * comm_ptr, MPIR_Info * info, MPIR_Comm ** newcomm_ptr)
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


    /* Generate a new context value and a new communicator structure */
    /* We must use the local size, because this is compared to the
     * rank of the process in the communicator.  For intercomms,
     * this must be the local size */
    mpi_errno = MPII_Comm_copy(comm_ptr, comm_ptr->local_size, info, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    (*newcomm_ptr)->attributes = new_attributes;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_create_from_group_impl(MPIR_Group * group_ptr, const char *stringtag,
                                     MPIR_Info * info_ptr, MPIR_Errhandler * errhan_ptr,
                                     MPIR_Comm ** p_newcom_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    /* This implementation assumes an internal MPI_COMM_WORLD already exist.
     *
     * Refer to TODOs below inside the branches:
     * The next implementation will relax but assume the first call to this function will
     * either from a "world" pset or "self" pset, during former a comm world will be created.
     *
     * The final implementation will further relax and allow partial initializations.
     */

    /* Need convert stringtag to tag, use macro from uthash.h to calculate hash value */
    unsigned hash;
    int n = strlen(stringtag);
    HASH_VALUE(stringtag, n, hash);

    /* NOTE: tag will be used with MPIR_TAG_COLL_BIT on, ref. MPIR_Get_contextid_sparse_group */
    int tag = hash % (MPIR_Process.attrs.tag_ub);

    if (MPIR_Process.comm_world) {
        if (!MPIR_Process.comm_world->local_group) {
            /* need create local_group for MPIR_Comm_create_group_impl to work */
            MPIR_Group *world_group_ptr;
            mpi_errno = MPIR_Comm_group_impl(MPIR_Process.comm_world, &world_group_ptr);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_Group_release(world_group_ptr);
        }
        MPIR_Comm_create_group_impl(MPIR_Process.comm_world, group_ptr, tag, p_newcom_ptr);
    } else if (group_ptr->pset_name && strcmp(group_ptr->pset_name, "mpi://WORLD") == 0) {
        /* TODO: once we init process is split into local init and world init, we need call
         * world-init in this branch and then call MPIR_Comm_dup_impl(MPIR_Process.comm_world, ...)
         */
        MPIR_Assert(0 && "not implemented");
        goto fn_fail;
    } else if (group_ptr->pset_name && strcmp(group_ptr->pset_name, "mpi://SELF") == 0) {
        /* TODO: We need refactor a function to create a self-comm as local-only operation,
         * then just call the self-comm-creation here. */
        /* TODO: Ideally, a single process application never need world-init and we should
         * be able to do on-demand dynamic connections afterwards. Essentially world-init will
         * be replaced with dynamic init. This is needed in next branch. */
        MPIR_Assert(0 && "not implemented");
        goto fn_fail;
    } else {
        /* TODO: dynamically check and establish connections */
        MPIR_Assert(0 && "not implemented");
        goto fn_fail;
    }

    if (info_ptr) {
        MPII_Comm_set_hints(*p_newcom_ptr, info_ptr);
    }

    if (errhan_ptr) {
        MPIR_Comm_set_errhandler_impl(*p_newcom_ptr, errhan_ptr);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_free_impl(MPIR_Comm * comm_ptr)
{
    return MPIR_Comm_release(comm_ptr);
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
    int i, lpid, n;
    int comm_world_size = MPIR_Process.comm_world->local_size;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_COMM_GROUP_IMPL);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_COMM_GROUP_IMPL);
    /* Create a group if necessary and populate it with the
     * local process ids */
    if (!comm_ptr->local_group) {
        n = comm_ptr->local_size;
        mpi_errno = MPIR_Group_create(n, group_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        (*group_ptr)->is_local_dense_monotonic = TRUE;
        for (i = 0; i < n; i++) {
            (void) MPID_Comm_get_lpid(comm_ptr, i, &lpid, FALSE);
            (*group_ptr)->lrank_to_lpid[i].lpid = lpid;
            if (lpid > comm_world_size ||
                (i > 0 && (*group_ptr)->lrank_to_lpid[i - 1].lpid != (lpid - 1))) {
                (*group_ptr)->is_local_dense_monotonic = FALSE;
            }
        }

        (*group_ptr)->size = n;
        (*group_ptr)->rank = comm_ptr->rank;
        (*group_ptr)->idx_of_first_lpid = -1;

        comm_ptr->local_group = *group_ptr;
    } else {
        *group_ptr = comm_ptr->local_group;
    }

    /* FIXME : Add a sanity check that the size of the group is the same as
     * the size of the communicator.  This helps catch corrupted
     * communicators */

    MPIR_Group_add_ref(comm_ptr->local_group);

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_COMM_GROUP_IMPL);
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
    int i, lpid, n;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_COMM_REMOTE_GROUP_IMPL);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_COMM_REMOTE_GROUP_IMPL);
    /* Create a group and populate it with the local process ids */
    if (!comm_ptr->remote_group) {
        n = comm_ptr->remote_size;
        mpi_errno = MPIR_Group_create(n, group_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        for (i = 0; i < n; i++) {
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
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_COMM_REMOTE_GROUP_IMPL);
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

int MPIR_Comm_set_info_impl(MPIR_Comm * comm_ptr, MPIR_Info * info_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_COMM_SET_INFO_IMPL);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_COMM_SET_INFO_IMPL);

    mpi_errno = MPII_Comm_set_hints(comm_ptr, info_ptr);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_COMM_SET_INFO_IMPL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* comm shrink impl; assumes that standard error checking has already taken
 * place in the calling function */
int MPIR_Comm_shrink_impl(MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *global_failed = NULL, *comm_grp = NULL, *new_group_ptr = NULL;
    int attempts = 0;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_COMM_SHRINK);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_COMM_SHRINK);

    /* TODO - Implement this function for intercommunicators */
    MPIR_Comm_group_impl(comm_ptr, &comm_grp);

    do {
        errflag = MPIR_ERR_NONE;

        MPID_Comm_get_all_failed_procs(comm_ptr, &global_failed, MPIR_SHRINK_TAG);
        /* Ignore the mpi_errno value here as it will definitely communicate
         * with failed procs */

        mpi_errno = MPIR_Group_difference_impl(comm_grp, global_failed, &new_group_ptr);
        MPIR_ERR_CHECK(mpi_errno);
        if (MPIR_Group_empty != global_failed)
            MPIR_Group_release(global_failed);

        mpi_errno = MPIR_Comm_create_group_impl(comm_ptr, new_group_ptr, MPIR_SHRINK_TAG,
                                                newcomm_ptr);
        if (*newcomm_ptr == NULL) {
            errflag = MPIR_ERR_PROC_FAILED;
        } else if (mpi_errno) {
            errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_Comm_release(*newcomm_ptr);
        }

        mpi_errno = MPII_Allreduce_group(MPI_IN_PLACE, &errflag, 1, MPI_INT, MPI_MAX, comm_ptr,
                                         new_group_ptr, MPIR_SHRINK_TAG, &errflag);
        MPIR_Group_release(new_group_ptr);

        if (errflag) {
            if (*newcomm_ptr != NULL && MPIR_Object_get_ref(*newcomm_ptr) > 0) {
                MPIR_Object_set_ref(*newcomm_ptr, 1);
                MPIR_Comm_release(*newcomm_ptr);
            }
            if (MPIR_Object_get_ref(new_group_ptr) > 0) {
                MPIR_Object_set_ref(new_group_ptr, 1);
                MPIR_Group_release(new_group_ptr);
            }
        }
    } while (errflag && ++attempts < 5);

    if (errflag && attempts >= 5)
        goto fn_fail;
    else
        mpi_errno = MPI_SUCCESS;

  fn_exit:
    MPIR_Group_release(comm_grp);
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_COMM_SHRINK);
    return mpi_errno;
  fn_fail:
    if (*newcomm_ptr)
        MPIR_Object_set_ref(*newcomm_ptr, 0);
    MPIR_Object_set_ref(global_failed, 0);
    MPIR_Object_set_ref(new_group_ptr, 0);
    goto fn_exit;
}

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

typedef struct splittype {
    int color, key;
} splittype;

/* Same as splittype but with an additional field to stabilize the qsort.  We
 * could just use one combined type, but using separate types simplifies the
 * allgather step. */
typedef struct sorttype {
    int color, key;
    int orig_idx;
} sorttype;

#if defined(HAVE_QSORT)
static int sorttype_compare(const void *v1, const void *v2)
{
    const sorttype *s1 = v1;
    const sorttype *s2 = v2;

    if (s1->key > s2->key)
        return 1;
    if (s1->key < s2->key)
        return -1;

    /* (s1->key == s2->key), maintain original order */
    if (s1->orig_idx > s2->orig_idx)
        return 1;
    else if (s1->orig_idx < s2->orig_idx)
        return -1;

    /* --BEGIN ERROR HANDLING-- */
    return 0;   /* should never happen */
    /* --END ERROR HANDLING-- */
}
#endif

/* Sort the entries in keytable into increasing order by key.  A stable
   sort should be used incase the key values are not unique. */
static void MPIU_Sort_inttable(sorttype * keytable, int size)
{
    sorttype tmp;
    int i, j;

#if defined(HAVE_QSORT)
    /* temporary switch for profiling performance differences */
    if (MPIR_CVAR_COMM_SPLIT_USE_QSORT) {
        /* qsort isn't a stable sort, so we have to enforce stability by keeping
         * track of the original indices */
        for (i = 0; i < size; ++i)
            keytable[i].orig_idx = i;
        qsort(keytable, size, sizeof(sorttype), &sorttype_compare);
    } else
#endif
    {
        /* --BEGIN USEREXTENSION-- */
        /* fall through to insertion sort if qsort is unavailable/disabled */
        for (i = 1; i < size; ++i) {
            tmp = keytable[i];
            j = i - 1;
            while (1) {
                if (keytable[j].key > tmp.key) {
                    keytable[j + 1] = keytable[j];
                    j = j - 1;
                    if (j < 0)
                        break;
                } else {
                    break;
                }
            }
            keytable[j + 1] = tmp;
        }
        /* --END USEREXTENSION-- */
    }
}

int MPIR_Comm_split_impl(MPIR_Comm * comm_ptr, int color, int key, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *local_comm_ptr;
    splittype *table, *remotetable = 0;
    sorttype *keytable, *remotekeytable = 0;
    int rank, size, remote_size, i, new_size, new_remote_size,
        first_entry = 0, first_remote_entry = 0, *last_ptr;
    int in_newcomm;             /* TRUE iff *newcomm should be populated */
    MPIR_Context_id_t new_context_id, remote_context_id;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Comm_map_t *mapper;
    MPIR_CHKLMEM_DECL(4);

    rank = comm_ptr->rank;
    size = comm_ptr->local_size;
    remote_size = comm_ptr->remote_size;

    /* Step 1: Find out what color and keys all of the processes have */
    MPIR_CHKLMEM_MALLOC(table, splittype *, size * sizeof(splittype), mpi_errno,
                        "table", MPL_MEM_COMM);
    table[rank].color = color;
    table[rank].key = key;

    /* Get the communicator to use in collectives on the local group of
     * processes */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        if (!comm_ptr->local_comm) {
            MPII_Setup_intercomm_localcomm(comm_ptr);
        }
        local_comm_ptr = comm_ptr->local_comm;
    } else {
        local_comm_ptr = comm_ptr;
    }
    /* Gather information on the local group of processes */
    mpi_errno =
        MPIR_Allgather(MPI_IN_PLACE, 2, MPI_INT, table, 2, MPI_INT, local_comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    /* Step 2: How many processes have our same color? */
    new_size = 0;
    if (color != MPI_UNDEFINED) {
        /* Also replace the color value with the index of the *next* value
         * in this set.  The integer first_entry is the index of the
         * first element */
        last_ptr = &first_entry;
        for (i = 0; i < size; i++) {
            /* Replace color with the index in table of the next item
             * of the same color.  We use this to efficiently populate
             * the keyval table */
            if (table[i].color == color) {
                new_size++;
                *last_ptr = i;
                last_ptr = &table[i].color;
            }
        }
    }
    /* We don't need to set the last value to -1 because we loop through
     * the list for only the known size of the group */

    /* If we're an intercomm, we need to do the same thing for the remote
     * table, as we need to know the size of the remote group of the
     * same color before deciding to create the communicator */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        splittype mypair;
        /* For the remote group, the situation is more complicated.
         * We need to find the size of our "partner" group in the
         * remote comm.  The easiest way (in terms of code) is for
         * every process to essentially repeat the operation for the
         * local group - perform an (intercommunicator) all gather
         * of the color and rank information for the remote group.
         */
        MPIR_CHKLMEM_MALLOC(remotetable, splittype *,
                            remote_size * sizeof(splittype), mpi_errno,
                            "remotetable", MPL_MEM_COMM);
        /* This is an intercommunicator allgather */

        /* We must use a local splittype because we've already modified the
         * entries in table to indicate the location of the next rank of the
         * same color */
        mypair.color = color;
        mypair.key = key;
        mpi_errno = MPIR_Allgather(&mypair, 2, MPI_INT, remotetable, 2, MPI_INT,
                                   comm_ptr, &errflag);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* Each process can now match its color with the entries in the table */
        new_remote_size = 0;
        last_ptr = &first_remote_entry;
        for (i = 0; i < remote_size; i++) {
            /* Replace color with the index in table of the next item
             * of the same color.  We use this to efficiently populate
             * the keyval table */
            if (remotetable[i].color == color) {
                new_remote_size++;
                *last_ptr = i;
                last_ptr = &remotetable[i].color;
            }
        }
        /* Note that it might find that there a now processes in the remote
         * group with the same color.  In that case, COMM_SPLIT will
         * return a null communicator */
    } else {
        /* Set the size of the remote group to the size of our group.
         * This simplifies the test below for intercomms with an empty remote
         * group (must create comm_null) */
        new_remote_size = new_size;
    }

    in_newcomm = (color != MPI_UNDEFINED && new_remote_size > 0);

    /* Step 3: Create the communicator */
    /* Collectively create a new context id.  The same context id will
     * be used by each (disjoint) collections of processes.  The
     * processes whose color is MPI_UNDEFINED will not influence the
     * resulting context id (by passing ignore_id==TRUE). */
    /* In the multi-threaded case, MPIR_Get_contextid_sparse assumes that the
     * calling routine already holds the single criticial section */
    mpi_errno = MPIR_Get_contextid_sparse(local_comm_ptr, &new_context_id, !in_newcomm);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(new_context_id != 0);

    /* In the intercomm case, we need to exchange the context ids */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        if (comm_ptr->rank == 0) {
            mpi_errno = MPIC_Sendrecv(&new_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, 0,
                                      &remote_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE,
                                      0, 0, comm_ptr, MPI_STATUS_IGNORE, &errflag);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno =
                MPIR_Bcast(&remote_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, local_comm_ptr,
                           &errflag);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
        } else {
            /* Broadcast to the other members of the local group */
            mpi_errno =
                MPIR_Bcast(&remote_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, local_comm_ptr,
                           &errflag);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
        }
    }

    *newcomm_ptr = NULL;

    /* Now, create the new communicator structure if necessary */
    if (in_newcomm) {

        mpi_errno = MPIR_Comm_create(newcomm_ptr);
        if (mpi_errno)
            goto fn_fail;

        (*newcomm_ptr)->recvcontext_id = new_context_id;
        (*newcomm_ptr)->local_size = new_size;
        (*newcomm_ptr)->comm_kind = comm_ptr->comm_kind;
        /* Other fields depend on whether this is an intercomm or intracomm */

        /* Step 4: Order the processes by their key values.  Sort the
         * list that is stored in table.  To simplify the sort, we
         * extract the table into a smaller array and sort that.
         * Also, store in the "color" entry the rank in the input communicator
         * of the entry. */
        MPIR_CHKLMEM_MALLOC(keytable, sorttype *, new_size * sizeof(sorttype),
                            mpi_errno, "keytable", MPL_MEM_COMM);
        for (i = 0; i < new_size; i++) {
            keytable[i].key = table[first_entry].key;
            keytable[i].color = first_entry;
            first_entry = table[first_entry].color;
        }

        /* sort key table.  The "color" entry is the rank of the corresponding
         * process in the input communicator */
        MPIU_Sort_inttable(keytable, new_size);

        if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
            MPIR_CHKLMEM_MALLOC(remotekeytable, sorttype *,
                                new_remote_size * sizeof(sorttype),
                                mpi_errno, "remote keytable", MPL_MEM_COMM);
            for (i = 0; i < new_remote_size; i++) {
                remotekeytable[i].key = remotetable[first_remote_entry].key;
                remotekeytable[i].color = first_remote_entry;
                first_remote_entry = remotetable[first_remote_entry].color;
            }

            /* sort key table.  The "color" entry is the rank of the
             * corresponding process in the input communicator */
            MPIU_Sort_inttable(remotekeytable, new_remote_size);

            MPIR_Comm_map_irregular(*newcomm_ptr, comm_ptr, NULL,
                                    new_size, MPIR_COMM_MAP_DIR__L2L, &mapper);

            for (i = 0; i < new_size; i++) {
                mapper->src_mapping[i] = keytable[i].color;
                if (keytable[i].color == comm_ptr->rank)
                    (*newcomm_ptr)->rank = i;
            }

            /* For the remote group, the situation is more complicated.
             * We need to find the size of our "partner" group in the
             * remote comm.  The easiest way (in terms of code) is for
             * every process to essentially repeat the operation for the
             * local group - perform an (intercommunicator) all gather
             * of the color and rank information for the remote group.
             */
            /* We apply the same sorting algorithm to the entries that we've
             * found to get the correct order of the entries.
             *
             * Note that if new_remote_size is 0 (no matching processes with
             * the same color in the remote group), then MPI_COMM_SPLIT
             * is required to return MPI_COMM_NULL instead of an intercomm
             * with an empty remote group. */

            MPIR_Comm_map_irregular(*newcomm_ptr, comm_ptr, NULL,
                                    new_remote_size, MPIR_COMM_MAP_DIR__R2R, &mapper);

            for (i = 0; i < new_remote_size; i++)
                mapper->src_mapping[i] = remotekeytable[i].color;

            (*newcomm_ptr)->context_id = remote_context_id;
            (*newcomm_ptr)->remote_size = new_remote_size;
            (*newcomm_ptr)->local_comm = 0;
            (*newcomm_ptr)->is_low_group = comm_ptr->is_low_group;

        } else {
            /* INTRA Communicator */
            (*newcomm_ptr)->context_id = (*newcomm_ptr)->recvcontext_id;
            (*newcomm_ptr)->remote_size = new_size;

            MPIR_Comm_map_irregular(*newcomm_ptr, comm_ptr, NULL,
                                    new_size, MPIR_COMM_MAP_DIR__L2L, &mapper);

            for (i = 0; i < new_size; i++) {
                mapper->src_mapping[i] = keytable[i].color;
                if (keytable[i].color == comm_ptr->rank)
                    (*newcomm_ptr)->rank = i;
            }
        }

        /* Inherit the error handler (if any) */
        MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));
        (*newcomm_ptr)->errhandler = comm_ptr->errhandler;
        if (comm_ptr->errhandler) {
            MPIR_Errhandler_add_ref(comm_ptr->errhandler);
        }
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));

        (*newcomm_ptr)->tainted = comm_ptr->tainted;
        mpi_errno = MPIR_Comm_commit(*newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static const char *SHMEM_INFO_KEY = "shmem_topo";

static int node_split(MPIR_Comm * comm_ptr, int key, const char *hint_str,
                      MPIR_Comm ** newcomm_ptr);
static int compare_info_hint(const char *hint_str, MPIR_Comm * comm_ptr, int *info_args_are_equal);

int MPIR_Comm_split_type(MPIR_Comm * user_comm_ptr, int split_type, int key,
                         MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;

    /* split out the undefined processes */
    mpi_errno = MPIR_Comm_split_impl(user_comm_ptr, split_type == MPI_UNDEFINED ? MPI_UNDEFINED : 0,
                                     key, &comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    if (split_type == MPI_UNDEFINED) {
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    if (split_type == MPI_COMM_TYPE_SHARED) {
        mpi_errno = MPIR_Comm_split_type_self(comm_ptr, split_type, key, newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (split_type == MPIX_COMM_TYPE_NEIGHBORHOOD) {
        mpi_errno =
            MPIR_Comm_split_type_neighborhood(comm_ptr, split_type, key, info_ptr, newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_ARG, "**arg");
    }

  fn_exit:
    if (comm_ptr)
        MPIR_Comm_free_impl(comm_ptr);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_split_type_impl(MPIR_Comm * comm_ptr, int split_type, int key,
                              MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    /* Only MPI_COMM_TYPE_SHARED, MPI_UNDEFINED, and
     * NEIGHBORHOOD are supported */
    MPIR_Assert(split_type == MPI_COMM_TYPE_SHARED ||
                split_type == MPI_UNDEFINED || split_type == MPIX_COMM_TYPE_NEIGHBORHOOD);

    if (MPIR_Comm_fns != NULL && MPIR_Comm_fns->split_type != NULL) {
        mpi_errno = MPIR_Comm_fns->split_type(comm_ptr, split_type, key, info_ptr, newcomm_ptr);
    } else {
        mpi_errno = MPIR_Comm_split_type(comm_ptr, split_type, key, info_ptr, newcomm_ptr);
    }
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Comm_set_info_impl(*newcomm_ptr, info_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_split_type_self(MPIR_Comm * user_comm_ptr, int split_type, int key,
                              MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Comm *comm_self_ptr;
    int mpi_errno = MPI_SUCCESS;

    /* split out the undefined processes */
    mpi_errno = MPIR_Comm_split_impl(user_comm_ptr, split_type == MPI_UNDEFINED ? MPI_UNDEFINED : 0,
                                     key, &comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    if (split_type == MPI_UNDEFINED) {
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    MPIR_Comm_get_ptr(MPI_COMM_SELF, comm_self_ptr);
    mpi_errno = MPIR_Comm_dup_impl(comm_self_ptr, newcomm_ptr);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (comm_ptr)
        MPIR_Comm_free_impl(comm_ptr);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_split_type_by_node(MPIR_Comm * user_comm_ptr, int split_type, int key,
                                 MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    int color;

    /* split out the undefined processes */
    mpi_errno = MPIR_Comm_split_impl(user_comm_ptr, split_type == MPI_UNDEFINED ? MPI_UNDEFINED : 0,
                                     key, &comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    if (split_type == MPI_UNDEFINED) {
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    mpi_errno = MPID_Get_node_id(comm_ptr, comm_ptr->rank, &color);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (comm_ptr)
        MPIR_Comm_free_impl(comm_ptr);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_split_type_node_topo(MPIR_Comm * user_comm_ptr, int split_type, int key,
                                   MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr;
    int mpi_errno = MPI_SUCCESS;
    int flag = 0;
    char hint_str[MPI_MAX_INFO_VAL + 1];
    int info_args_are_equal;
    *newcomm_ptr = NULL;

    mpi_errno = MPIR_Comm_split_type_by_node(user_comm_ptr, split_type, key, &comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    if (comm_ptr == NULL) {
        MPIR_Assert(split_type == MPI_UNDEFINED);
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    if (info_ptr) {
        MPIR_Info_get_impl(info_ptr, SHMEM_INFO_KEY, MPI_MAX_INFO_VAL, hint_str, &flag);
    }

    if (!flag) {
        hint_str[0] = '\0';
    }

    mpi_errno = compare_info_hint(hint_str, comm_ptr, &info_args_are_equal);

    MPIR_ERR_CHECK(mpi_errno);

    /* if all processes do not have the same hint_str, skip
     * topology-aware comm split */
    if (!info_args_are_equal)
        goto use_node_comm;

    /* if no info key is given, skip topology-aware comm split */
    if (!info_ptr)
        goto use_node_comm;

    /* if hw topology is not initialized, skip topology-aware comm split */
    if (!MPIR_hwtopo_is_initialized())
        goto use_node_comm;

    if (flag) {
        mpi_errno = node_split(comm_ptr, key, hint_str, newcomm_ptr);

        MPIR_ERR_CHECK(mpi_errno);

        MPIR_Comm_free_impl(comm_ptr);

        goto fn_exit;
    }

  use_node_comm:
    *newcomm_ptr = comm_ptr;

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int node_split(MPIR_Comm * comm_ptr, int key, const char *hint_str, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_hwtopo_gid_t gid;

    gid = MPIR_hwtopo_get_obj_by_name(hint_str);

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, gid, key, newcomm_ptr);

    return mpi_errno;
}

static int compare_info_hint(const char *hint_str, MPIR_Comm * comm_ptr, int *info_args_are_equal)
{
    int hint_str_size = strlen(hint_str);
    int hint_str_size_max;
    int hint_str_equal;
    int hint_str_equal_global = 0;
    char *hint_str_global = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    /* Find the maximum hint_str size.  Each process locally compares
     * its hint_str size to the global max, and makes sure that this
     * comparison is successful on all processes. */
    mpi_errno =
        MPID_Allreduce(&hint_str_size, &hint_str_size_max, 1, MPI_INT, MPI_MAX, comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    hint_str_equal = (hint_str_size == hint_str_size_max);

    mpi_errno =
        MPID_Allreduce(&hint_str_equal, &hint_str_equal_global, 1, MPI_INT, MPI_LAND,
                       comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    if (!hint_str_equal_global)
        goto fn_exit;


    /* Now that the sizes of the hint_strs match, check to make sure
     * the actual hint_strs themselves are the equal */
    hint_str_global = (char *) MPL_malloc(strlen(hint_str), MPL_MEM_OTHER);

    mpi_errno =
        MPID_Allreduce(hint_str, hint_str_global, strlen(hint_str), MPI_CHAR,
                       MPI_MAX, comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    hint_str_equal = !memcmp(hint_str, hint_str_global, strlen(hint_str));

    mpi_errno =
        MPID_Allreduce(&hint_str_equal, &hint_str_equal_global, 1, MPI_INT, MPI_LAND,
                       comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPL_free(hint_str_global);

    *info_args_are_equal = hint_str_equal_global;
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
    int remote_size = 0, *remote_lpids = NULL;
    int comm_info[3];
    int is_low_group = 0;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_COMM_KIND__INTERCOMM_CREATE_IMPL);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_COMM_KIND__INTERCOMM_CREATE_IMPL);

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
     * calling routine already holds the single criticial section */
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

        /* Extract the context and group sign informatin */
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
    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(local_comm_ptr));
    (*new_intercomm_ptr)->errhandler = local_comm_ptr->errhandler;
    if (local_comm_ptr->errhandler) {
        MPIR_Errhandler_add_ref(local_comm_ptr->errhandler);
    }
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(local_comm_ptr));

    (*new_intercomm_ptr)->tainted = 1;
    mpi_errno = MPIR_Comm_commit(*new_intercomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);


  fn_exit:
    MPL_free(remote_lpids);
    remote_lpids = NULL;
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_COMM_KIND__INTERCOMM_CREATE_IMPL);
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
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_COMM_KIND__INTERCOMM_MERGE_IMPL);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_COMM_KIND__INTERCOMM_MERGE_IMPL);
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
     * calling routine already holds the single criticial section */
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
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_COMM_KIND__INTERCOMM_MERGE_IMPL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
