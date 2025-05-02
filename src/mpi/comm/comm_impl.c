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
    MPID_THREAD_CS_ENTER(VCI, comm_ptr->mutex);
    MPL_strncpy(comm_ptr->name, comm_name, MPI_MAX_OBJECT_NAME);
    MPID_THREAD_CS_EXIT(VCI, comm_ptr->mutex);
    return MPI_SUCCESS;
}

int MPIR_Comm_test_inter_impl(MPIR_Comm * comm_ptr, int *flag)
{
    *flag = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM);
    return MPI_SUCCESS;
}

int MPIR_Comm_test_threadcomm_impl(MPIR_Comm * comm_ptr, int *flag)
{
    *flag = (comm_ptr->threadcomm != NULL);
    return MPI_SUCCESS;
}

/* used in MPIR_Comm_group_impl and MPIR_Comm_create_group_impl */
static int comm_create_local_group(MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    int n = comm_ptr->local_size;
    MPIR_Lpid *map = MPL_malloc(n * sizeof(MPIR_Lpid), MPL_MEM_GROUP);

    for (int i = 0; i < n; i++) {
        map[i] = MPIR_Group_rank_to_lpid(comm_ptr->local_group, i);
    }

    mpi_errno = MPIR_Group_create_map(n, comm_ptr->rank, comm_ptr->session_ptr, map,
                                      &comm_ptr->local_group);
    MPIR_ERR_CHECK(mpi_errno);

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

/* comm create impl for intracommunicators, assumes that the standard error
 * checking has already taken place in the calling function */
int MPIR_Comm_create_intra(MPIR_Comm * comm_ptr, MPIR_Group * group_ptr, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int new_context_id = 0;
    int *mapping = NULL;
    int n;

    MPIR_FUNC_ENTER;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Group_check_subset(group_ptr, comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);
#endif

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

        (*newcomm_ptr)->remote_group = NULL;
        (*newcomm_ptr)->context_id = (*newcomm_ptr)->recvcontext_id;
        (*newcomm_ptr)->remote_size = (*newcomm_ptr)->local_size = n;

        MPIR_Comm_set_session_ptr(*newcomm_ptr, comm_ptr->session_ptr);

        (*newcomm_ptr)->vcis_enabled = comm_ptr->vcis_enabled;
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
    MPIR_CHKLMEM_DECL();
    MPIR_FUNC_ENTER;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM);
    MPIR_Session *session_ptr = comm_ptr->session_ptr;

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
    int new_context_id;
    mpi_errno = MPIR_Get_contextid_sparse(comm_ptr->local_comm, &new_context_id, FALSE);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(new_context_id != 0);
    MPIR_Assert(new_context_id != comm_ptr->recvcontext_id);

    /* There is an additional step.  We must communicate the
     * information on the local context id and the group members,
     * given by the ranks so that the remote process can construct the
     * appropriate network address mapping.
     * First we exchange group sizes and context ids.  Then the ranks
     * in the remote group, from which the remote network address
     * mapping can be constructed.  We need to use the "collective"
     * context in the original intercommunicator */

    int remote_size = -1;
    int context_id;
    int *remote_mapping;        /* a list of remote ranks */
    int rinfo[2];

    if (comm_ptr->rank == 0) {
        int info[2];
        info[0] = new_context_id;
        info[1] = group_ptr->size;

        mpi_errno = MPIC_Sendrecv(info, 2, MPIR_INT_INTERNAL, 0, 0,
                                  rinfo, 2, MPIR_INT_INTERNAL, 0, 0, comm_ptr, MPI_STATUS_IGNORE,
                                  MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);
        context_id = rinfo[0];
        remote_size = rinfo[1];

        int *mapping;
        MPIR_CHKLMEM_MALLOC(mapping, group_ptr->size * sizeof(int));

        /* effectively MPIR_Group_translate_ranks_impl */
        for (int i = 0; i < group_ptr->size; i++) {
            MPIR_Lpid lpid = MPIR_Group_rank_to_lpid(group_ptr, i);
            mapping[i] = MPIR_Group_lpid_to_rank(comm_ptr->local_group, lpid);
        }

        MPIR_CHKLMEM_MALLOC(remote_mapping, remote_size * sizeof(int));

        /* Populate and exchange the ranks */
        mpi_errno = MPIC_Sendrecv(mapping, group_ptr->size, MPIR_INT_INTERNAL, 0, 0,
                                  remote_mapping, remote_size, MPIR_INT_INTERNAL, 0, 0,
                                  comm_ptr, MPI_STATUS_IGNORE, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);

        /* Broadcast to the other members of the local group */
        mpi_errno = MPIR_Bcast(rinfo, 2, MPIR_INT_INTERNAL, 0, comm_ptr->local_comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Bcast(remote_mapping, remote_size, MPIR_INT_INTERNAL, 0,
                               comm_ptr->local_comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* The other processes */
        /* Broadcast to the other members of the local group */
        mpi_errno = MPIR_Bcast(rinfo, 2, MPIR_INT_INTERNAL, 0, comm_ptr->local_comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);

        context_id = rinfo[0];
        remote_size = rinfo[1];
        MPIR_CHKLMEM_MALLOC(remote_mapping, remote_size * sizeof(int));
        mpi_errno = MPIR_Bcast(remote_mapping, remote_size, MPIR_INT_INTERNAL, 0,
                               comm_ptr->local_comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIR_Assert(remote_size >= 0);
    if (group_ptr->rank == MPI_UNDEFINED || remote_size <= 0) {
        /* If we are not part of the group, or -
         * It's possible that no members of the other side of comm were
         * members of the group that they passed, which we only know after
         * receiving/bcasting the remote_size above.  We must return
         * MPI_COMM_NULL in this case.
         */
        MPIR_Free_contextid(new_context_id);
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    /* Get the new communicator structure and context id */
    mpi_errno = MPIR_Comm_create(newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    (*newcomm_ptr)->context_id = context_id;
    (*newcomm_ptr)->remote_size = remote_size;
    (*newcomm_ptr)->recvcontext_id = new_context_id;
    (*newcomm_ptr)->rank = group_ptr->rank;
    (*newcomm_ptr)->comm_kind = comm_ptr->comm_kind;
    /* Since the group has been provided, let the new communicator know
     * about the group */
    (*newcomm_ptr)->local_comm = 0;
    (*newcomm_ptr)->local_size = group_ptr->size;
    (*newcomm_ptr)->local_group = group_ptr;
    MPIR_Group_add_ref(group_ptr);

    mpi_errno = MPIR_Group_incl_impl(comm_ptr->remote_group, remote_size, remote_mapping,
                                     &(*newcomm_ptr)->remote_group);

    (*newcomm_ptr)->is_low_group = comm_ptr->is_low_group;

    MPIR_Comm_set_session_ptr(*newcomm_ptr, session_ptr);

    (*newcomm_ptr)->vcis_enabled = comm_ptr->vcis_enabled;

    mpi_errno = MPIR_Comm_commit(*newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
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
    int new_context_id = 0;
    int n;

    MPIR_FUNC_ENTER;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    n = group_ptr->size;
    *newcomm_ptr = NULL;

    /* Shift tag into the tagged coll space */
    tag |= MPIR_TAG_COLL_BIT;

    /* Create a new communicator from the specified group members */

    if (group_ptr->rank != MPI_UNDEFINED) {
        /* For this routine, creation of the id is collective over the input
         *group*, so processes not in the group do not participate. */

        mpi_errno = MPIR_Get_contextid_sparse_group(comm_ptr, group_ptr, tag, &new_context_id, 0);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(new_context_id != 0);

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

        (*newcomm_ptr)->remote_group = NULL;
        (*newcomm_ptr)->context_id = (*newcomm_ptr)->recvcontext_id;
        (*newcomm_ptr)->remote_size = (*newcomm_ptr)->local_size = n;

        MPIR_Comm_set_session_ptr(*newcomm_ptr, group_ptr->session_ptr);

        (*newcomm_ptr)->vcis_enabled = comm_ptr->vcis_enabled;
        mpi_errno = MPIR_Comm_commit(*newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* This process is not in the group */
        new_context_id = 0;
    }

  fn_exit:
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

int MPIR_Comm_create_from_group_impl(MPIR_Group * group_ptr, const char *stringtag,
                                     MPIR_Info * info_ptr, MPIR_Errhandler * errhan_ptr,
                                     MPIR_Comm ** p_newcom_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int tag = get_tag_from_stringtag(stringtag);

    MPIR_Comm *new_comm = (MPIR_Comm *) MPIR_Handle_obj_alloc(&MPIR_Comm_mem);
    MPIR_ERR_CHKANDJUMP(!new_comm, mpi_errno, MPI_ERR_OTHER, "**nomem");

    mpi_errno = MPII_Comm_init(new_comm);
    MPIR_ERR_CHECK(mpi_errno);

    new_comm->attr |= MPIR_COMM_ATTR__BOOTSTRAP;
    new_comm->context_id = MPIR_CTXID_BOOTSTRAP;
    new_comm->recvcontext_id = new_comm->context_id;
    new_comm->comm_kind = MPIR_COMM_KIND__INTRACOMM;
    new_comm->rank = group_ptr->rank;
    new_comm->local_size = group_ptr->size;
    new_comm->remote_size = new_comm->local_size;
    new_comm->local_group = group_ptr;
    MPIR_Group_add_ref(group_ptr);

    mpi_errno = MPIR_Comm_commit(new_comm);
    MPIR_ERR_CHECK(mpi_errno);

    /* allocate a new context id */
    int new_context_id;
    mpi_errno = MPIR_Get_contextid_sparse_group(new_comm, NULL, tag, &new_context_id, 0);
    MPIR_ERR_CHECK(mpi_errno);

    new_comm->context_id = new_context_id;
    new_comm->recvcontext_id = new_comm->context_id;
    if (new_comm->node_comm) {
        new_comm->node_comm->context_id = new_context_id + MPIR_CONTEXT_INTRANODE_OFFSET;
        new_comm->node_comm->recvcontext_id = new_comm->node_comm->context_id;
    }
    if (new_comm->node_roots_comm) {
        new_comm->node_roots_comm->context_id = new_context_id + MPIR_CONTEXT_INTERNODE_OFFSET;
        new_comm->node_roots_comm->recvcontext_id = new_comm->node_roots_comm->context_id;
    }

    if (info_ptr) {
        MPII_Comm_set_hints(new_comm, info_ptr, true);
    }

    if (errhan_ptr) {
        MPIR_Comm_set_errhandler_impl(new_comm, errhan_ptr);
    }

  fn_exit:
    *p_newcom_ptr = new_comm;
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int lpid_cmp(MPIR_Lpid lpid_a, MPIR_Lpid lpid_b);
static int create_peer_comm(MPIR_Lpid my_lpid, MPIR_Lpid remote_lpid,
                            MPIR_Session * session, const char *stringtag,
                            MPIR_Errhandler * errhan_ptr,
                            MPIR_Comm ** peer_comm_out, int *remote_rank_out)
{
    int mpi_errno = MPI_SUCCESS;

    bool is_low_group = (lpid_cmp(my_lpid, remote_lpid) < 0);

    MPIR_Lpid peer_map[2];
    int my_rank, remote_rank;
    if (is_low_group) {
        peer_map[0] = my_lpid;
        peer_map[1] = remote_lpid;
        my_rank = 0;
        remote_rank = 1;
    } else {
        peer_map[0] = remote_lpid;
        peer_map[1] = my_lpid;
        my_rank = 1;
        remote_rank = 0;
    }

    MPIR_Group *peer_group;
    mpi_errno = MPIR_Group_create_map(2, my_rank, session, peer_map, &peer_group);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Comm *peer_comm;
    mpi_errno = MPIR_Comm_create_from_group_impl(peer_group, stringtag, NULL, errhan_ptr,
                                                 &peer_comm);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Group_free_impl(peer_group);
    *peer_comm_out = peer_comm;
    *remote_rank_out = remote_rank;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Intercomm_create_from_groups_impl(MPIR_Group * local_group_ptr, int local_leader,
                                           MPIR_Group * remote_group_ptr, int remote_leader,
                                           const char *stringtag,
                                           MPIR_Info * info_ptr, MPIR_Errhandler * errhan_ptr,
                                           MPIR_Comm ** p_newintercom_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Session *session = local_group_ptr->session_ptr;
    MPIR_ERR_CHKANDJUMP(session != remote_group_ptr->session_ptr, mpi_errno, MPI_ERR_OTHER,
                        "**session_mixed");

    bool is_leader = (local_group_ptr->rank == local_leader);

    /* first, create local comm */
    MPIR_Comm *local_comm;
    mpi_errno = MPIR_Comm_create_from_group_impl(local_group_ptr, stringtag, info_ptr, errhan_ptr,
                                                 &local_comm);
    MPIR_ERR_CHECK(mpi_errno);

    /* next, create peer_comm between the leaders */
    MPIR_Comm *peer_comm = NULL;
    int remote_rank = -1;
    if (is_leader) {
        MPIR_Lpid my_lpid = MPIR_Group_rank_to_lpid(local_group_ptr, local_leader);
        MPIR_Lpid remote_lpid = MPIR_Group_rank_to_lpid(remote_group_ptr, remote_leader);

        mpi_errno = create_peer_comm(my_lpid, remote_lpid, session, stringtag, errhan_ptr,
                                     &peer_comm, &remote_rank);
    }

    /* synchronize mpi_errno */
    int tmp_err = mpi_errno;
    mpi_errno = MPIR_Bcast_impl(&tmp_err, 1, MPIR_INT_INTERNAL, local_leader, local_comm,
                                MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = tmp_err;
    MPIR_ERR_CHECK(mpi_errno);

    int tag = get_tag_from_stringtag(stringtag);
    mpi_errno = MPIR_Intercomm_create_impl(local_comm, local_leader, peer_comm, remote_rank,
                                           tag, p_newintercom_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Comm_release(local_comm);
    if (is_leader) {
        MPIR_Comm_release(peer_comm);
    }

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
    if (comm_ptr == NULL) {
        MPL_strncpy(comm_name, "MPI_COMM_NULL", MPI_MAX_OBJECT_NAME);
    } else {
        MPL_strncpy(comm_name, comm_ptr->name, MPI_MAX_OBJECT_NAME);
    }
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
    MPIR_FUNC_ENTER;

    /* FIXME: remove the following remote_group creation once this assertion passes */
    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM && comm_ptr->remote_group);

    /* Create a group and populate it with the local process ids */
    if (!comm_ptr->remote_group) {
        int n = comm_ptr->remote_size;
        MPIR_Lpid *map = MPL_malloc(n * sizeof(MPIR_Lpid), MPL_MEM_GROUP);

        for (int i = 0; i < n; i++) {
            map[i] = MPIR_Group_rank_to_lpid(comm_ptr->remote_group, i);
        }
        mpi_errno = MPIR_Group_create_map(n, MPI_UNDEFINED, comm_ptr->session_ptr, map,
                                          &comm_ptr->remote_group);
        MPIR_ERR_CHECK(mpi_errno);
    }
    *group_ptr = comm_ptr->remote_group;
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

/* arbitrarily determine which group is the low_group by comparing
 * world namespaces and world ranks */
static int lpid_cmp(MPIR_Lpid lpid_a, MPIR_Lpid lpid_b)
{
    int indx_a = MPIR_LPID_WORLD_INDEX(lpid_a);;
    int rank_a = MPIR_LPID_WORLD_RANK(lpid_a);
    int indx_b = MPIR_LPID_WORLD_INDEX(lpid_b);
    int rank_b = MPIR_LPID_WORLD_RANK(lpid_b);

    if (lpid_a == lpid_b) {
        return 0;
    } else if (indx_a == indx_b) {
        /* same world, just compare world ranks */
        if (rank_a < rank_b) {
            return -1;
        } else {
            return 1;
        }
    } else {
        /* different world, compare namespace */
        return strncmp(MPIR_Worlds[indx_a].namespace, MPIR_Worlds[indx_b].namespace,
                       MPIR_NAMESPACE_MAX);
    }

}

static int determine_low_group(MPIR_Lpid remote_lpid, bool * is_low_group_out)
{
    int mpi_errno = MPI_SUCCESS;

    int cmp_result = lpid_cmp(MPIR_Process.rank, remote_lpid);
    MPIR_Assert(cmp_result != 0);

    *is_low_group_out = (cmp_result < 0);
    return mpi_errno;
}

int MPIR_Intercomm_create_impl(MPIR_Comm * local_comm_ptr, int local_leader,
                               MPIR_Comm * peer_comm_ptr, int remote_leader, int tag,
                               MPIR_Comm ** new_intercomm_ptr)
{
    return MPIR_Intercomm_create_timeout(local_comm_ptr, local_leader,
                                         peer_comm_ptr, remote_leader, tag, 0, new_intercomm_ptr);
}

int MPIR_Intercomm_create_timeout(MPIR_Comm * local_comm_ptr, int local_leader,
                                  MPIR_Comm * peer_comm_ptr, int remote_leader,
                                  int tag, int timeout, MPIR_Comm ** new_intercomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int remote_size = 0;
    MPIR_Lpid *remote_lpids = NULL;
    MPIR_Session *session_ptr = local_comm_ptr->session_ptr;

    MPIR_FUNC_ENTER;

    /*
     * Create the contexts.  Each group will have a context for sending
     * to the other group. All processes must be involved.  Because
     * we know that the local and remote groups are disjoint, this
     * step will complete
     */
    /* In the multi-threaded case, MPIR_Get_contextid_sparse assumes that the
     * calling routine already holds the single critical section */
    /* TODO: Make sure this is tag-safe */
    int recvcontext_id = MPIR_INVALID_CONTEXT_ID;
    mpi_errno = MPIR_Get_contextid_sparse(local_comm_ptr, &recvcontext_id, FALSE);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(recvcontext_id != 0);

    /* Shift tag into the tagged coll space */
    tag |= MPIR_TAG_COLL_BIT;

    int remote_context_id;
    mpi_errno = MPID_Intercomm_exchange(local_comm_ptr, local_leader,
                                        peer_comm_ptr, remote_leader, tag,
                                        recvcontext_id, &remote_context_id,
                                        &remote_size, &remote_lpids, timeout);
    MPIR_ERR_CHECK(mpi_errno);

    bool is_low_group;
    mpi_errno = determine_low_group(remote_lpids[0], &is_low_group);
    MPIR_ERR_CHECK(mpi_errno);

    /* At last, we now have the information that we need to build the
     * intercommunicator */

    /* All processes in the local_comm now build the communicator */

    mpi_errno = MPIR_Comm_create(new_intercomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    (*new_intercomm_ptr)->context_id = remote_context_id;
    (*new_intercomm_ptr)->recvcontext_id = recvcontext_id;
    (*new_intercomm_ptr)->remote_size = remote_size;
    (*new_intercomm_ptr)->local_size = local_comm_ptr->local_size;
    (*new_intercomm_ptr)->rank = local_comm_ptr->rank;
    (*new_intercomm_ptr)->comm_kind = MPIR_COMM_KIND__INTERCOMM;
    (*new_intercomm_ptr)->local_comm = 0;
    (*new_intercomm_ptr)->is_low_group = is_low_group;

    (*new_intercomm_ptr)->local_group = local_comm_ptr->local_group;
    MPIR_Group_add_ref(local_comm_ptr->local_group);

    /* NOTE: create map before MPIR_Group_create_map because remote_lpids may
     * get deallocated in MPIR_Group_create_map (e.g. strided map) */
    mpi_errno = MPID_Create_intercomm_from_lpids(*new_intercomm_ptr, remote_size, remote_lpids);
    MPIR_ERR_CHECK(mpi_errno);

    /* construct remote_group */
    mpi_errno = MPIR_Group_create_map(remote_size, MPI_UNDEFINED, session_ptr, remote_lpids,
                                      &(*new_intercomm_ptr)->remote_group);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Comm_set_session_ptr(*new_intercomm_ptr, session_ptr);

    /* Inherit the error handler (if any) */
    MPID_THREAD_CS_ENTER(VCI, local_comm_ptr->mutex);
    (*new_intercomm_ptr)->errhandler = local_comm_ptr->errhandler;
    if (local_comm_ptr->errhandler) {
        MPIR_Errhandler_add_ref(local_comm_ptr->errhandler);
    }
    MPID_THREAD_CS_EXIT(VCI, local_comm_ptr->mutex);

    mpi_errno = MPIR_Comm_commit(*new_intercomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    if (recvcontext_id != MPIR_INVALID_CONTEXT_ID) {
        MPIR_Free_contextid(recvcontext_id);
    }
    goto fn_exit;
}

int MPIR_Intercomm_merge_impl(MPIR_Comm * comm_ptr, int high, MPIR_Comm ** new_intracomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int local_high, remote_high, new_size;
    int new_context_id;

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
        mpi_errno = MPIC_Sendrecv(&local_high, 1, MPIR_INT_INTERNAL, 0, 0,
                                  &remote_high, 1, MPIR_INT_INTERNAL, 0, 0, comm_ptr,
                                  MPI_STATUS_IGNORE, MPIR_ERR_NONE);
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
    mpi_errno = MPIR_Bcast(&local_high, 1, MPIR_INT_INTERNAL, 0, comm_ptr->local_comm,
                           MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

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
    (*new_intracomm_ptr)->comm_kind = MPIR_COMM_KIND__INTRACOMM;
    (*new_intracomm_ptr)->remote_group = NULL;

    MPIR_Comm_set_session_ptr(*new_intracomm_ptr, comm_ptr->session_ptr);

    /* construct local_group */
    MPIR_Group *new_local_group;

    MPIR_Lpid *map;
    map = MPL_malloc(new_size * sizeof(MPIR_Lpid), MPL_MEM_GROUP);
    MPIR_ERR_CHKANDJUMP(!map, mpi_errno, MPI_ERR_OTHER, "**nomem");

    int myrank;
    MPIR_Group *group1, *group2;
    if (local_high) {
        group1 = comm_ptr->remote_group;
        group2 = comm_ptr->local_group;
        myrank = group1->size + group2->rank;
    } else {
        group1 = comm_ptr->local_group;
        group2 = comm_ptr->remote_group;
        myrank = group1->rank;
    }
    for (int i = 0; i < group1->size; i++) {
        map[i] = MPIR_Group_rank_to_lpid(group1, i);
    }
    for (int i = 0; i < group2->size; i++) {
        map[group1->size + i] = MPIR_Group_rank_to_lpid(group2, i);
    }

    mpi_errno = MPIR_Group_create_map(new_size, myrank, comm_ptr->session_ptr, map,
                                      &new_local_group);

    (*new_intracomm_ptr)->local_group = new_local_group;
    MPIR_Group_add_ref(new_local_group);

    (*new_intracomm_ptr)->rank = myrank;

    /* We've setup a temporary context id, based on the context id
     * used by the intercomm.  This allows us to perform the allreduce
     * operations within the context id algorithm, since we already
     * have a valid (almost - see comm_create_hook) communicator.
     */
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
    (*new_intracomm_ptr)->rank = myrank;
    (*new_intracomm_ptr)->comm_kind = MPIR_COMM_KIND__INTRACOMM;
    (*new_intracomm_ptr)->context_id = new_context_id;
    (*new_intracomm_ptr)->recvcontext_id = new_context_id;
    (*new_intracomm_ptr)->remote_group = NULL;

    MPIR_Comm_set_session_ptr(*new_intracomm_ptr, comm_ptr->session_ptr);
    (*new_intracomm_ptr)->local_group = new_local_group;

    mpi_errno = MPIR_Comm_commit((*new_intracomm_ptr));
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
