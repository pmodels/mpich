/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpicomm.h"
#include "mpir_info.h"  /* MPIR_Info_free */

#include "utlist.h"
#include "uthash.h"

/* This is the utility file for comm that contains the basic comm items
   and storage management */

/* Preallocated comm objects */
/* initialized in initthread.c */
MPIR_Comm MPIR_Comm_builtin[MPIR_COMM_N_BUILTIN];
MPIR_Comm MPIR_Comm_direct[MPIR_COMM_PREALLOC];

MPIR_Object_alloc_t MPIR_Comm_mem = { 0, 0, 0, 0, 0, 0, 0, MPIR_COMM,
    sizeof(MPIR_Comm),
    MPIR_Comm_direct,
    MPIR_COMM_PREALLOC,
    {0}
};

/* Communicator creation functions */
struct MPIR_Commops *MPIR_Comm_fns = NULL;
static int MPIR_Comm_commit_internal(MPIR_Comm * comm);

/* Communicator hint functions */
/* For balance of simplicity and feature, we'll internally use integers for both keys
 * and values, and provide facilities to translate from and to string-based infos.
 */

struct MPIR_HINT {
    const char *key;
    MPIR_Comm_hint_fn_t fn;
    int type;
    int attr;                   /* e.g. whether this key is local */
    int default_val;
};
static struct MPIR_HINT MPIR_comm_hint_list[MPIR_COMM_HINT_MAX];
static int next_comm_hint_index = MPIR_COMM_HINT_PREDEFINED_COUNT;

int MPIR_Comm_register_hint(int idx, const char *hint_key, MPIR_Comm_hint_fn_t fn,
                            int type, int attr, int default_val)
{
    if (idx == 0) {
        idx = next_comm_hint_index;
        next_comm_hint_index++;
        MPIR_Assert(idx < MPIR_COMM_HINT_MAX);
    } else {
        MPIR_Assert(idx > 0 && idx < MPIR_COMM_HINT_PREDEFINED_COUNT);
    }
    MPIR_comm_hint_list[idx] = (struct MPIR_HINT) {
    hint_key, fn, type, attr, default_val};
    return idx;
}

static int parse_string_value(const char *s, int type, int *val)
{
    if (type == MPIR_COMM_HINT_TYPE_BOOL) {
        if (strcmp(s, "true") == 0) {
            *val = 1;
        } else if (strcmp(s, "false") == 0) {
            *val = 0;
        } else {
            *val = atoi(s);
        }
    } else if (type == MPIR_COMM_HINT_TYPE_INT) {
        *val = atoi(s);
    } else {
        return -1;
    }
    return 0;
}

static int get_string_value(char *s, int type, int val)
{
    if (type == MPIR_COMM_HINT_TYPE_BOOL) {
        strncpy(s, val ? "true" : "false", MPI_MAX_INFO_VAL);
    } else if (type == MPIR_COMM_HINT_TYPE_INT) {
        snprintf(s, MPI_MAX_INFO_VAL, "%d", val);
    } else {
        return -1;
    }
    return 0;
}

/* Hints are stored as hints array inside MPIR_Comm.
 * All hints are initialized to zero. Communitcator creation hook can be used to
 * to customize initialization value (make sure only do that when the value is zero
 * or risk resetting user hints).
 * If the hint is registered with callback function, it can be used for customization
 * at both creation time and run-time.
 */
int MPII_Comm_set_hints(MPIR_Comm * comm_ptr, MPIR_Info * info, bool in_comm_creation)
{
    int mpi_errno = MPI_SUCCESS;

    for (int i = 0; i < next_comm_hint_index; i++) {
        if (!MPIR_comm_hint_list[i].key) {
            continue;
        }

        const char *str_val;
        str_val = MPIR_Info_lookup(info, MPIR_comm_hint_list[i].key);
        if (str_val) {
            int val;
            int rc = parse_string_value(str_val, MPIR_comm_hint_list[i].type, &val);
            if (rc == 0) {
                if (MPIR_comm_hint_list[i].fn) {
                    MPIR_comm_hint_list[i].fn(comm_ptr, i, val);
                } else {
                    comm_ptr->hints[i] = val;
                }
            }
        }
    }

    /* Device can process hints during commit stage if in_comm_creation */
    if (!in_comm_creation) {
        mpi_errno = MPID_Comm_set_hints(comm_ptr, info);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* FIXME: run collective to ensure hints consistency */
  fn_exit:
    return MPI_SUCCESS;
  fn_fail:
    goto fn_exit;
}

int MPII_Comm_get_hints(MPIR_Comm * comm_ptr, MPIR_Info * info)
{
    int mpi_errno = MPI_SUCCESS;

    char hint_val_str[MPI_MAX_INFO_VAL];
    for (int i = 0; i < next_comm_hint_index; i++) {
        if (MPIR_comm_hint_list[i].key) {
            get_string_value(hint_val_str, MPIR_comm_hint_list[i].type, comm_ptr->hints[i]);
            mpi_errno = MPIR_Info_set_impl(info, MPIR_comm_hint_list[i].key, hint_val_str);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    char *memory_alloc_kinds;
    MPIR_get_memory_kinds_from_comm(comm_ptr, &memory_alloc_kinds);
    MPIR_Info_set_impl(info, "mpi_memory_alloc_kinds", memory_alloc_kinds);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_Comm_check_hints(MPIR_Comm * comm)
{
    /* for all non-local hints and non-zero hint values, run collective
     * to check whether they are equal across the communicator */
    /* TODO */
    return MPI_SUCCESS;
}

/*
=== BEGIN_INFO_HINT_BLOCK ===
hints:
    - name        : mpi_assert_no_any_tag
      functions   : MPI_Comm_dup_with_info, MPI_Comm_idup_with_info, MPI_Comm_set_info
      type        : boolean
      default     : false
      description : >-
        If set to true, user promises that MPI_ANY_TAG will not be used with
        the communicator. This potentially allows MPICH to treat messages with
        different tags independent and seek to improve performance, e.g. by
        employ multiple network context.

    - name        : mpi_assert_no_any_source
      functions   : MPI_Comm_dup_with_info, MPI_Comm_idup_with_info, MPI_Comm_set_info
      type        : boolean
      default     : false
      description : >-
        If set to true, user promises that MPI_ANY_SOURCE will not be used with
        the communicator. This potentially allows MPICH to treat messages send
        to different ranks or receive from different ranks independent and
        seek to improve performance, e.g. by employ multiple network context.

    - name        : mpi_assert_exact_length
      functions   : MPI_Comm_dup_with_info, MPI_Comm_idup_with_info, MPI_Comm_set_info
      type        : boolean
      default     : false
      description : >-
        If set to true, user promises that the lengths of messages received
        by the process will always equal to the size of the corresponding
        receive buffers.

    - name        : mpi_assert_allow_overtaking
      functions   : MPI_Comm_dup_with_info, MPI_Comm_idup_with_info, MPI_Comm_set_info
      type        : boolean
      default     : false
      description : >-
        If set to true, user asserts that send operations are not required
        to be matched at the receiver in the order in which the send operations
        were performed by the sender, and receive operations are not required
        to be matched in the order in which they were performed by the
        receivers.

    - name        : mpi_assert_strict_persistent_collective_ordering
      functions   : MPI_Comm_dup_with_info, MPI_Comm_idup_with_info, MPI_Comm_set_info
      type        : boolean
      default     : false
      description : >-
      If set to true, then the implementation may assume that all the persistent
      collective operations are started in the same order across all MPI processes
      in the group of the communicator. It is required that if this assertion is made
      on one member of the communicator's group, then it must be made on all members
      of that communicator's group with the same value.

=== END_INFO_HINT_BLOCK ===
*/

void MPIR_Comm_hint_init(void)
{
    MPIR_Comm_register_hint(MPIR_COMM_HINT_NO_ANY_TAG, "mpi_assert_no_any_tag",
                            NULL, MPIR_COMM_HINT_TYPE_BOOL, 0, 0);
    MPIR_Comm_register_hint(MPIR_COMM_HINT_NO_ANY_SOURCE, "mpi_assert_no_any_source",
                            NULL, MPIR_COMM_HINT_TYPE_BOOL, 0, 0);
    MPIR_Comm_register_hint(MPIR_COMM_HINT_EXACT_LENGTH, "mpi_assert_exact_length",
                            NULL, MPIR_COMM_HINT_TYPE_BOOL, 0, 0);
    MPIR_Comm_register_hint(MPIR_COMM_HINT_ALLOW_OVERTAKING, "mpi_assert_allow_overtaking",
                            NULL, MPIR_COMM_HINT_TYPE_BOOL, 0, 0);
    MPIR_Comm_register_hint(MPIR_COMM_HINT_STRICT_PCOLL_ORDERING,
                            "mpi_assert_strict_persistent_collective_ordering", NULL,
                            MPIR_COMM_HINT_TYPE_BOOL, 0, 0);
    /* Used by ch4:ofi, but needs to be initialized early to get the default value. */
    MPIR_Comm_register_hint(MPIR_COMM_HINT_ENABLE_MULTI_NIC_STRIPING, "enable_multi_nic_striping",
                            NULL, MPIR_COMM_HINT_TYPE_BOOL, 0, -1);
    MPIR_Comm_register_hint(MPIR_COMM_HINT_ENABLE_MULTI_NIC_HASHING, "enable_multi_nic_hashing",
                            NULL, MPIR_COMM_HINT_TYPE_BOOL, 0, -1);
    MPIR_Comm_register_hint(MPIR_COMM_HINT_MULTI_NIC_PREF_NIC, "multi_nic_pref_nic", NULL,
                            MPIR_COMM_HINT_TYPE_INT, 0, -1);
}

/* FIXME :
   Reusing context ids can lead to a race condition if (as is desirable)
   MPI_Comm_free does not include a barrier.  Consider the following:
   Process A frees the communicator.
   Process A creates a new communicator, reusing the just released id
   Process B sends a message to A on the old communicator.
   Process A receives the message, and believes that it belongs to the
   new communicator.
   Process B then cancels the message, and frees the communicator.

   The likelihood of this happening can be reduced by introducing a gap
   between when a context id is released and when it is reused.  An alternative
   is to use an explicit message (in the implementation of MPI_Comm_free)
   to indicate that a communicator is being freed; this will often require
   less communication than a barrier in MPI_Comm_free, and will ensure that
   no messages are later sent to the same communicator (we may also want to
   have a similar check when building fault-tolerant versions of MPI).
 */

/* Zeroes most non-handle fields in a communicator, as well as initializing any
 * other special fields, such as a per-object mutex.  Also defaults the
 * reference count to 1, under the assumption that the caller holds a reference
 * to it.
 *
 * !!! The resulting struct is _not_ ready for communication !!! */
int MPII_Comm_init(MPIR_Comm * comm_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Object_set_ref(comm_p, 1);

    /* initialize local and remote sizes to -1 to allow other parts of
     * the stack to detect errors more easily */
    comm_p->local_size = -1;
    comm_p->remote_size = -1;

    /* Clear many items (empty means to use the default; some of these
     * may be overridden within the upper-level communicator initialization) */
    comm_p->errhandler = NULL;
    comm_p->attributes = NULL;
    comm_p->remote_group = NULL;
    comm_p->local_group = NULL;
    comm_p->topo_fns = NULL;
    comm_p->bsendbuffer = NULL;
    comm_p->name[0] = '\0';
    comm_p->seq = 0;    /* default to 0, to be updated at Comm_commit */
    comm_p->vcis_enabled = false;
    memset(comm_p->hints, 0, sizeof(comm_p->hints));
    for (int i = 0; i < next_comm_hint_index; i++) {
        if (MPIR_comm_hint_list[i].key) {
            comm_p->hints[i] = MPIR_comm_hint_list[i].default_val;
        }
    }

    comm_p->hierarchy_kind = MPIR_COMM_HIERARCHY_KIND__FLAT;
    comm_p->node_comm = NULL;
    comm_p->node_roots_comm = NULL;
    comm_p->intranode_table = NULL;
    comm_p->internode_table = NULL;

    /* abstractions bleed a bit here... :(*/
    comm_p->next_sched_tag = MPIR_FIRST_NBC_TAG;
    comm_p->next_am_tag = 0;

    /* Initialize the revoked flag as false */
    comm_p->revoked = 0;

    comm_p->threadcomm = NULL;
    MPIR_stream_comm_init(comm_p);

    comm_p->persistent_requests = NULL;

    /* mutex is only used in VCI granularity. But the overhead of
     * creation is low, so we always create it. */
    {
        int thr_err;
        MPID_Thread_mutex_create(&comm_p->mutex, &thr_err);
        MPIR_Assert(thr_err == 0);
    }

    /* Initialize the session_ptr to NULL; for non-session-related communicators it will remain NULL */
    comm_p->session_ptr = NULL;

    /* Fields not set include context_id, remote and local size, and
     * kind, since different communicator construction routines need
     * different values */
    return mpi_errno;
}


/*
    Create a communicator structure and perform basic initialization
    (mostly clearing fields and updating the reference count).
 */
int MPIR_Comm_create(MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *newptr;

    MPIR_FUNC_ENTER;

    newptr = (MPIR_Comm *) MPIR_Handle_obj_alloc(&MPIR_Comm_mem);
    MPIR_ERR_CHKANDJUMP(!newptr, mpi_errno, MPI_ERR_OTHER, "**nomem");

    *newcomm_ptr = newptr;

    mpi_errno = MPII_Comm_init(newptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* Insert this new communicator into the list of known communicators.
     * Make this conditional on debugger support to match the test in
     * MPIR_Comm_release . */
    MPII_COMML_REMEMBER(newptr);

  fn_fail:
    MPIR_FUNC_EXIT;

    return mpi_errno;
}

/* Create a local intra communicator from the local group of the
   specified intercomm. */
/* FIXME this is an alternative constructor that doesn't use MPIR_Comm_create! */
int MPII_Setup_intercomm_localcomm(MPIR_Comm * intercomm_ptr)
{
    MPIR_Comm *localcomm_ptr;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    localcomm_ptr = (MPIR_Comm *) MPIR_Handle_obj_alloc(&MPIR_Comm_mem);
    MPIR_ERR_CHKANDJUMP(!localcomm_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem");

    /* get sensible default values for most fields (usually zeros) */
    mpi_errno = MPII_Comm_init(localcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Assert(intercomm_ptr->local_group);
    localcomm_ptr->local_group = intercomm_ptr->local_group;
    MPIR_Group_add_ref(intercomm_ptr->local_group);

    MPIR_Comm_set_session_ptr(localcomm_ptr, intercomm_ptr->session_ptr);

    /* use the parent intercomm's recv ctx as the basis for our ctx */
    localcomm_ptr->recvcontext_id =
        MPIR_CONTEXT_SET_FIELD(IS_LOCALCOMM, intercomm_ptr->recvcontext_id, 1);
    localcomm_ptr->context_id = localcomm_ptr->recvcontext_id;

    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, TYPICAL,
                    (MPL_DBG_FDEST,
                     "setup_intercomm_localcomm ic=%p ic->context_id=%d ic->recvcontext_id=%d lc->recvcontext_id=%d",
                     intercomm_ptr, intercomm_ptr->context_id, intercomm_ptr->recvcontext_id,
                     localcomm_ptr->recvcontext_id));

    /* Save the kind of the communicator */
    localcomm_ptr->comm_kind = MPIR_COMM_KIND__INTRACOMM;

    /* Set the sizes and ranks */
    localcomm_ptr->remote_size = intercomm_ptr->local_size;
    localcomm_ptr->local_size = intercomm_ptr->local_size;
    localcomm_ptr->rank = intercomm_ptr->rank;

    /* FIXME  : No local functions for the topology routines */

    intercomm_ptr->local_comm = localcomm_ptr;

    /* sets up the SMP-aware sub-communicators and tables */
    mpi_errno = MPIR_Comm_commit(localcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_fail:
    MPIR_FUNC_EXIT;

    return mpi_errno;
}

static int get_node_count(MPIR_Comm * comm, int *node_count)
{
    int mpi_errno = MPI_SUCCESS;
    struct uniq_nodes {
        int id;
        UT_hash_handle hh;
    } *node_list = NULL;
    struct uniq_nodes *s, *tmp;

    if (comm->comm_kind != MPIR_COMM_KIND__INTRACOMM) {
        *node_count = comm->local_size;
        goto fn_exit;
    } else if (comm->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__NODE) {
        *node_count = 1;
        goto fn_exit;
    } else if (comm->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__NODE_ROOTS) {
        *node_count = comm->local_size;
        goto fn_exit;
    }

    /* go through the list of ranks and add the unique ones to the
     * node_list array */
    for (int i = 0; i < comm->local_size; i++) {
        int node;

        mpi_errno = MPID_Get_node_id(comm, i, &node);
        MPIR_ERR_CHECK(mpi_errno);

        HASH_FIND_INT(node_list, &node, s);
        if (s == NULL) {
            s = (struct uniq_nodes *) MPL_malloc(sizeof(struct uniq_nodes), MPL_MEM_COLL);
            MPIR_Assert(s);
            s->id = node;
            HASH_ADD_INT(node_list, id, s, MPL_MEM_COLL);
        }
    }

    /* the final size of our hash table is our node count */
    *node_count = HASH_COUNT(node_list);

    /* free up everything */
    HASH_ITER(hh, node_list, s, tmp) {
        HASH_DEL(node_list, s);
        MPL_free(s);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIR_Comm_commit_internal(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* Notify device of communicator creation */
    mpi_errno = MPID_Comm_commit_pre_hook(comm);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = get_node_count(comm, &comm->node_count);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Copy relevant hints to a given subcomm */
static void propagate_hints_to_subcomm(MPIR_Comm * comm, MPIR_Comm * subcomm)
{
    /* Copy vci hints */
    subcomm->hints[MPIR_COMM_HINT_SENDER_VCI] = comm->hints[MPIR_COMM_HINT_SENDER_VCI];
    subcomm->hints[MPIR_COMM_HINT_RECEIVER_VCI] = comm->hints[MPIR_COMM_HINT_RECEIVER_VCI];
    subcomm->hints[MPIR_COMM_HINT_VCI] = comm->hints[MPIR_COMM_HINT_VCI];
}

static void propagate_vcis_enabled(MPIR_Comm * comm)
{
    if (comm->node_comm != NULL)
        comm->node_comm->vcis_enabled = comm->vcis_enabled;

    if (comm->node_roots_comm != NULL)
        comm->node_roots_comm->vcis_enabled = comm->vcis_enabled;
}

int MPIR_Comm_create_subcomms(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    int num_local = -1, num_external = -1;
    int local_rank = -1, external_rank = -1;
    int *local_procs = NULL, *external_procs = NULL;

    MPIR_FUNC_ENTER;

    MPIR_Assert(comm->node_comm == NULL);
    MPIR_Assert(comm->node_roots_comm == NULL);

    mpi_errno = MPIR_Find_local(comm, &num_local, &local_rank, &local_procs,
                                &comm->intranode_table);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno) {
        if (MPIR_Err_is_fatal(mpi_errno))
            MPIR_ERR_POP(mpi_errno);

        /* Non-fatal errors simply mean that this communicator will not have
         * any node awareness.  Node-aware collectives are an optimization. */
        MPL_DBG_MSG_P(MPIR_DBG_COMM, VERBOSE, "MPIR_Find_local failed for comm_ptr=%p", comm);
        MPL_free(comm->intranode_table);

        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    mpi_errno = MPIR_Find_external(comm, &num_external, &external_rank, &external_procs,
                                   &comm->internode_table);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno) {
        if (MPIR_Err_is_fatal(mpi_errno))
            MPIR_ERR_POP(mpi_errno);

        /* Non-fatal errors simply mean that this communicator will not have
         * any node awareness.  Node-aware collectives are an optimization. */
        MPL_DBG_MSG_P(MPIR_DBG_COMM, VERBOSE, "MPIR_Find_external failed for comm_ptr=%p", comm);
        MPL_free(comm->internode_table);

        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    /* defensive checks */
    MPIR_Assert(num_local > 0);
    MPIR_Assert(num_local > 1 || external_rank >= 0);
    MPIR_Assert(external_rank < 0 || external_procs != NULL);

    /* if the node_roots_comm and comm would be the same size, then creating
     * the second communicator is useless and wasteful. */
    if (num_external == comm->remote_size) {
        MPIR_Assert(num_local == 1);
        goto fn_exit;
    }

    /* we don't need a local comm if this process is the only one on this node */
    if (num_local > 1) {
        mpi_errno = MPIR_Comm_create(&comm->node_comm);
        MPIR_ERR_CHECK(mpi_errno);

        comm->node_comm->context_id = comm->context_id + MPIR_CONTEXT_INTRANODE_OFFSET;
        comm->node_comm->recvcontext_id = comm->node_comm->context_id;
        comm->node_comm->rank = local_rank;
        comm->node_comm->comm_kind = MPIR_COMM_KIND__INTRACOMM;
        comm->node_comm->hierarchy_kind = MPIR_COMM_HIERARCHY_KIND__NODE;
        comm->node_comm->local_comm = NULL;
        MPL_DBG_MSG_D(MPIR_DBG_COMM, VERBOSE, "Create node_comm=%p\n", comm->node_comm);

        comm->node_comm->local_size = num_local;
        comm->node_comm->remote_size = num_local;

        MPIR_Comm_set_session_ptr(comm->node_comm, comm->session_ptr);

        /* Copy relevant hints to node_comm */
        propagate_hints_to_subcomm(comm, comm->node_comm);

        /* construct local_group */
        MPIR_Group *parent_group = comm->local_group;
        MPIR_Assert(parent_group);
        mpi_errno = MPIR_Group_incl_impl(parent_group, num_local, local_procs,
                                         &comm->node_comm->local_group);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Comm_commit_internal(comm->node_comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* this process may not be a member of the node_roots_comm */
    if (local_rank == 0) {
        mpi_errno = MPIR_Comm_create(&comm->node_roots_comm);
        MPIR_ERR_CHECK(mpi_errno);

        comm->node_roots_comm->context_id = comm->context_id + MPIR_CONTEXT_INTERNODE_OFFSET;
        comm->node_roots_comm->recvcontext_id = comm->node_roots_comm->context_id;
        comm->node_roots_comm->rank = external_rank;
        comm->node_roots_comm->comm_kind = MPIR_COMM_KIND__INTRACOMM;
        comm->node_roots_comm->hierarchy_kind = MPIR_COMM_HIERARCHY_KIND__NODE_ROOTS;
        comm->node_roots_comm->local_comm = NULL;
        MPL_DBG_MSG_D(MPIR_DBG_COMM, VERBOSE, "Create node_roots_comm=%p\n", comm->node_roots_comm);

        comm->node_roots_comm->local_size = num_external;
        comm->node_roots_comm->remote_size = num_external;

        MPIR_Comm_set_session_ptr(comm->node_roots_comm, comm->session_ptr);

        /* Copy relevant hints to node_roots_comm */
        propagate_hints_to_subcomm(comm, comm->node_roots_comm);

        /* construct local_group */
        MPIR_Group *parent_group = comm->local_group;
        MPIR_Assert(parent_group);
        mpi_errno = MPIR_Group_incl_impl(parent_group, num_external, external_procs,
                                         &comm->node_roots_comm->local_group);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Comm_commit_internal(comm->node_roots_comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

    propagate_vcis_enabled(comm);

    comm->hierarchy_kind = MPIR_COMM_HIERARCHY_KIND__PARENT;

  fn_exit:
    MPL_free(local_procs);
    MPL_free(external_procs);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* static routines for MPIR_Comm_commit */
static int init_comm_seq(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    /* Every user-level communicator gets a sequence number, which can be
     * used, for example, to hash vci.
     * Builtin-comm, e.g. MPI_COMM_WORLD, always have seq at 0 */
    if (!HANDLE_IS_BUILTIN(comm->handle)) {
        static int vci_seq = 0;
        vci_seq++;

        int tmp = vci_seq;
        /* Bcast seq over vci 0 */
        MPIR_Assert(comm->seq == 0);

        /* Every rank need share the same seq from root. NOTE: it is possible for
         * different communicators to have the same seq. It is only used as an
         * opportunistic optimization */
        mpi_errno = MPIR_Bcast_allcomm_auto(&tmp, 1, MPIR_INT_INTERNAL, 0, comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);

        comm->seq = tmp;
    }

    if (comm->node_comm) {
        comm->node_comm->seq = comm->seq;
    }

    if (comm->node_roots_comm) {
        comm->node_roots_comm->seq = comm->seq;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Provides a hook for the top level functions to perform some manipulation on a
   communicator just before it is given to the application level.

   For example, we create sub-communicators for SMP-aware collectives at this
   step. */
int MPIR_Comm_commit(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* It's OK to relax these assertions, but we should do so very
     * intentionally.  For now this function is the only place that we create
     * our hierarchy of communicators */
    MPIR_Assert(comm->node_comm == NULL);
    MPIR_Assert(comm->node_roots_comm == NULL);

    /* Notify device of communicator creation */
    mpi_errno = MPIR_Comm_commit_internal(comm);
    MPIR_ERR_CHECK(mpi_errno);

    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM && !MPIR_CONTEXT_READ_FIELD(SUBCOMM, comm->context_id)) {  /*make sure this is not a subcomm */
        mpi_errno = MPIR_Comm_create_subcomms(comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Create collectives-specific infrastructure */
    mpi_errno = MPIR_Coll_comm_init(comm);
    MPIR_ERR_CHECK(mpi_errno);

    if (comm->node_comm) {
        mpi_errno = MPIR_Coll_comm_init(comm->node_comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (comm->node_roots_comm) {
        mpi_errno = MPIR_Coll_comm_init(comm->node_roots_comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* call post commit hooks */
    mpi_errno = MPID_Comm_commit_post_hook(comm);
    MPIR_ERR_CHECK(mpi_errno);

    if (comm->node_comm) {
        mpi_errno = MPID_Comm_commit_post_hook(comm->node_comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (comm->node_roots_comm) {
        mpi_errno = MPID_Comm_commit_post_hook(comm->node_roots_comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM && comm->vcis_enabled) {
        mpi_errno = init_comm_seq(comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Returns true if the given communicator is aware of node topology information,
   false otherwise.  Such information could be used to implement more efficient
   collective communication, for example. */
int MPIR_Comm_is_parent_comm(MPIR_Comm * comm)
{
    return (comm->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__PARENT);
}

/* Returns true if the communicator is node-aware and processes in all the nodes
   are consecutive. For example, if node 0 contains "0, 1, 2, 3", node 1
   contains "4, 5, 6", and node 2 contains "7", we shall return true. */
int MPII_Comm_is_node_consecutive(MPIR_Comm * comm)
{
    int i = 0, curr_nodeidx = 0;
    int *internode_table = comm->internode_table;

    if (!MPIR_Comm_is_parent_comm(comm))
        return 0;

    for (; i < comm->local_size; i++) {
        if (internode_table[i] == curr_nodeidx + 1)
            curr_nodeidx++;
        else if (internode_table[i] != curr_nodeidx)
            return 0;
    }

    return 1;
}

/* Duplicate a communicator without copying the streams. This is the common
 * part called by both MPIR_Comm_dup_with_info_impl and MPIR_Stream_comm_create_impl.
 */
int MPII_Comm_dup(MPIR_Comm * comm_ptr, MPIR_Info * info, MPIR_Comm ** newcomm_ptr)
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

/*
 * Copy a communicator, including creating a new context and copying the
 * virtual connection tables and clearing the various fields.
 * Does *not* copy attributes.  If size is < the size of the local group
 * in the input communicator, copy only the first size elements.
 * If this process is not a member, return a null pointer in outcomm_ptr.
 * This is only supported in the case where the communicator is in
 * Intracomm (not an Intercomm).  Note that this is all that is required
 * for cart_create and graph_create.
 *
 * Used by cart_create, graph_create, and dup_create
 */
int MPII_Comm_copy(MPIR_Comm * comm_ptr, int size, MPIR_Info * info, MPIR_Comm ** outcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int new_context_id, new_recvcontext_id;
    MPIR_Comm *newcomm_ptr = NULL;

    MPIR_FUNC_ENTER;

    /* Get a new context first.  We need this to be collective over the
     * input communicator */
    /* If there is a context id cache in oldcomm, use it here.  Otherwise,
     * use the appropriate algorithm to get a new context id.  Be careful
     * of intercomms here */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        mpi_errno = MPIR_Get_intercomm_contextid(comm_ptr, &new_context_id, &new_recvcontext_id);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno = MPIR_Get_contextid_sparse(comm_ptr, &new_context_id, FALSE);
        new_recvcontext_id = new_context_id;
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(new_context_id != 0);
    }

    /* This is the local size, not the remote size, in the case of
     * an intercomm */
    if (comm_ptr->rank >= size) {
        *outcomm_ptr = 0;
        /* always free the recvcontext ID, never the "send" ID */
        MPIR_Free_contextid(new_recvcontext_id);
        goto fn_exit;
    }

    /* We're left with the processes that will have a non-null communicator.
     * Create the object, initialize the data, and return the result */

    mpi_errno = MPIR_Comm_create(&newcomm_ptr);
    if (mpi_errno)
        goto fn_fail;

    newcomm_ptr->context_id = new_context_id;
    newcomm_ptr->recvcontext_id = new_recvcontext_id;

    /* Save the kind of the communicator */
    newcomm_ptr->comm_kind = comm_ptr->comm_kind;
    newcomm_ptr->local_comm = 0;

    newcomm_ptr->local_group = comm_ptr->local_group;
    MPIR_Group_add_ref(comm_ptr->local_group);
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        newcomm_ptr->remote_group = comm_ptr->remote_group;
        MPIR_Group_add_ref(comm_ptr->remote_group);
    }

    MPIR_Comm_set_session_ptr(newcomm_ptr, comm_ptr->session_ptr);

    /* Set the sizes and ranks */
    newcomm_ptr->rank = comm_ptr->rank;
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        newcomm_ptr->local_size = comm_ptr->local_size;
        newcomm_ptr->remote_size = comm_ptr->remote_size;
        newcomm_ptr->is_low_group = comm_ptr->is_low_group;
    } else {
        newcomm_ptr->local_size = size;
        newcomm_ptr->remote_size = size;
    }

    /* Inherit the error handler (if any) */
    MPID_THREAD_CS_ENTER(VCI, comm_ptr->mutex);
    newcomm_ptr->errhandler = comm_ptr->errhandler;
    if (comm_ptr->errhandler) {
        MPIR_Errhandler_add_ref(comm_ptr->errhandler);
    }
    MPID_THREAD_CS_EXIT(VCI, comm_ptr->mutex);

    if (info) {
        MPII_Comm_set_hints(newcomm_ptr, info, true);
    }

    newcomm_ptr->vcis_enabled = comm_ptr->vcis_enabled;
    mpi_errno = MPIR_Comm_commit(newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* Start with no attributes on this communicator */
    newcomm_ptr->attributes = 0;

    *outcomm_ptr = newcomm_ptr;

  fn_fail:
  fn_exit:

    MPIR_FUNC_EXIT;

    return mpi_errno;
}

/* Copy a communicator, including copying the virtual connection tables and
 * clearing the various fields.  Does *not* allocate a context ID or commit the
 * communicator.  Does *not* copy attributes.
 *
 * Used by comm_idup.
 */
int MPII_Comm_copy_data(MPIR_Comm * comm_ptr, MPIR_Info * info, MPIR_Comm ** outcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *newcomm_ptr = NULL;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Comm_create(&newcomm_ptr);
    if (mpi_errno)
        goto fn_fail;

    /* use a large garbage value to ensure errors are caught more easily */
    newcomm_ptr->context_id = 32767;
    newcomm_ptr->recvcontext_id = 32767;

    /* Save the kind of the communicator */
    newcomm_ptr->comm_kind = comm_ptr->comm_kind;
    newcomm_ptr->local_comm = 0;

    newcomm_ptr->local_group = comm_ptr->local_group;
    MPIR_Group_add_ref(comm_ptr->local_group);
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        newcomm_ptr->remote_group = comm_ptr->remote_group;
        MPIR_Group_add_ref(comm_ptr->remote_group);
    }

    /* Set the sizes and ranks */
    newcomm_ptr->rank = comm_ptr->rank;
    newcomm_ptr->local_size = comm_ptr->local_size;
    newcomm_ptr->remote_size = comm_ptr->remote_size;
    newcomm_ptr->is_low_group = comm_ptr->is_low_group; /* only relevant for intercomms */

    MPIR_Comm_set_session_ptr(newcomm_ptr, comm_ptr->session_ptr);

    /* Inherit the error handler (if any) */
    MPID_THREAD_CS_ENTER(VCI, comm_ptr->mutex);
    newcomm_ptr->errhandler = comm_ptr->errhandler;
    if (comm_ptr->errhandler) {
        MPIR_Errhandler_add_ref(comm_ptr->errhandler);
    }
    MPID_THREAD_CS_EXIT(VCI, comm_ptr->mutex);

    if (info) {
        MPII_Comm_set_hints(newcomm_ptr, info, true);
    }

    /* Start with no attributes on this communicator */
    newcomm_ptr->attributes = 0;
    *outcomm_ptr = newcomm_ptr;

    newcomm_ptr->vcis_enabled = comm_ptr->vcis_enabled;
    propagate_vcis_enabled(newcomm_ptr);

  fn_fail:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

/* Common body between MPIR_Comm_release and MPIR_comm_release_always.  This
 * helper function frees the actual MPIR_Comm structure and any associated
 * storage.  It also releases any references to other objects.
 * This function should only be called when the communicator's reference count
 * has dropped to 0.
 *
 * !!! This routine should *never* be called outside of MPIR_Comm_release{,_always} !!!
 */
int MPIR_Comm_delete_internal(MPIR_Comm * comm_ptr)
{
    int in_use;
    int mpi_errno = MPI_SUCCESS;
    int unmatched_messages = 0;
    MPI_Comm comm_handle ATTRIBUTE((unused)) = comm_ptr->handle;

    MPIR_FUNC_ENTER;

    MPIR_Assert(MPIR_Object_get_ref(comm_ptr) == 0);    /* sanity check */

    /* Remove the attributes, executing the attribute delete routine.
     * Do this only if the attribute functions are defined.
     * This must be done first, because if freeing the attributes
     * returns an error, the communicator is not freed */
    if (MPIR_Process.attr_free && comm_ptr->attributes) {
        /* Temporarily add a reference to this communicator because
         * the attr_free code requires a valid communicator */
        MPIR_Object_add_ref(comm_ptr);
        mpi_errno = MPIR_Process.attr_free(comm_ptr->handle, &comm_ptr->attributes);
        /* Release the temporary reference added before the call to
         * attr_free */
        MPIR_Object_release_ref(comm_ptr, &in_use);
        MPIR_Assertp(in_use == 0);
    }

    /* If the attribute delete functions return failure, the
     * communicator must not be freed.  That is the reason for the
     * test on mpi_errno here. */
    if (mpi_errno == MPI_SUCCESS) {
        bool do_message_check = false;
#if defined(HAVE_ERROR_CHECKING)
        do_message_check = true;
#endif
#if defined(MPICH_IS_THREADED) && MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
        /* anysrc/anytag probe is extremely messy with pervci thread-cs. Avoid for for now. */
        if (MPIR_ThreadInfo.isThreaded) {
            do_message_check = false;
        }
#endif
        if (do_message_check) {
            /* receive any unmatched messages to clear the queue and avoid context_id
             * reuse issues. unmatched messages could be the result of user error,
             * or send operations that were unable to be canceled by the device layer */
            MPIR_Object_add_ref(comm_ptr);
            int flag;
            MPI_Status status;
            do {
                mpi_errno = MPID_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm_ptr, 0, &flag, &status);
                MPIR_ERR_CHECK(mpi_errno);
                if (flag) {
                    /* receive the message, ignore truncation errors */
                    MPIR_Request *request;
                    MPID_Recv(NULL, 0, MPI_DATATYPE_NULL, status.MPI_SOURCE, status.MPI_TAG,
                              comm_ptr, 0, MPI_STATUS_IGNORE, &request);
                    if (request != NULL) {
                        MPID_Wait(request, MPI_STATUS_IGNORE);
                        MPIR_Request_free(request);
                    }
                    unmatched_messages++;
                }
            } while (flag);
            MPIR_Object_release_ref(comm_ptr, &in_use);
        }

        /* If this communicator is our parent, and we're disconnecting
         * from the parent, mark that fact */
        if (MPIR_Process.comm_parent == comm_ptr)
            MPIR_Process.comm_parent = NULL;

        /* Cleanup collectives-specific infrastructure */
        mpi_errno = MPII_Coll_comm_cleanup(comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        /* Notify the device that the communicator is about to be
         * destroyed */
        mpi_errno = MPID_Comm_free_hook(comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Comm_bsend_finalize(comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        if (comm_ptr->session_ptr != NULL) {
            /* Release session */
            MPIR_Session_release(comm_ptr->session_ptr);
        }

        if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM && comm_ptr->local_comm)
            MPIR_Comm_release(comm_ptr->local_comm);

        /* Free the local and remote groups, if they exist */
        if (comm_ptr->local_group)
            MPIR_Group_release(comm_ptr->local_group);
        if (comm_ptr->remote_group)
            MPIR_Group_release(comm_ptr->remote_group);

        /* free the intra/inter-node communicators, if they exist */
        if (comm_ptr->node_comm)
            MPIR_Comm_release(comm_ptr->node_comm);
        if (comm_ptr->node_roots_comm)
            MPIR_Comm_release(comm_ptr->node_roots_comm);
        MPL_free(comm_ptr->intranode_table);
        MPL_free(comm_ptr->internode_table);

        MPIR_stream_comm_free(comm_ptr);

        /* Free the context value.  This should come after freeing the
         * intra/inter-node communicators since those free calls won't
         * release this context ID and releasing this before then could lead
         * to races once we make threading finer grained. */
        /* This must be the recvcontext_id (i.e. not the (send)context_id)
         * because in the case of intercommunicators the send context ID is
         * allocated out of the remote group's bit vector, not ours. */
        MPIR_Free_contextid(comm_ptr->recvcontext_id);

        {
            int thr_err;
            MPID_Thread_mutex_destroy(&comm_ptr->mutex, &thr_err);
            MPIR_Assert(thr_err == 0);
        }

        /* We need to release the error handler */
        if (comm_ptr->errhandler && !(HANDLE_IS_BUILTIN(comm_ptr->errhandler->handle))) {
            (void) MPIR_Errhandler_free_impl(comm_ptr->errhandler);
        }

        /* Remove from the list of active communicators if
         * we are supporting message-queue debugging.  We make this
         * conditional on having debugger support since the
         * operation is not constant-time */
        MPII_COMML_FORGET(comm_ptr);

        /* Check for predefined communicators - these should not
         * be freed */
        if (!(HANDLE_IS_BUILTIN(comm_ptr->handle)))
            MPIR_Handle_obj_free(&MPIR_Comm_mem, comm_ptr);
    } else {
        /* If the user attribute free function returns an error,
         * then do not free the communicator */
        MPIR_Comm_add_ref(comm_ptr);
    }

  fn_exit:
    if (unmatched_messages > 0) {
        MPIR_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**commhasunmatched", "**commhasunmatched %x %d",
                      comm_handle, unmatched_messages);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This function retrieves info key and collectively compares hint_str to see
 * hether all processes are having the same string.
 */
int MPII_collect_info_key(MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, const char *key,
                          const char **value_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    const char *hint_str = NULL;
    if (info_ptr) {
        hint_str = MPIR_Info_lookup(info_ptr, key);
    }

    int hint_str_size;
    if (!hint_str) {
        hint_str_size = 0;
    } else {
        hint_str_size = strlen(hint_str);
        if (hint_str_size == 0) {
            hint_str = NULL;
        }
    }

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        /* just ensure consistency on the local group */
        /* TODO: check consistency between local and remote processes */
        if (!comm_ptr->local_comm) {
            MPII_Setup_intercomm_localcomm(comm_ptr);
        }
        comm_ptr = comm_ptr->local_comm;
    }

    int is_equal;
    mpi_errno = MPIR_Allreduce_equal(&hint_str_size, 1, MPIR_INT_INTERNAL, &is_equal, comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    if (is_equal && hint_str_size > 0) {
        mpi_errno = MPIR_Allreduce_equal(hint_str, hint_str_size, MPIR_CHAR_INTERNAL,
                                         &is_equal, comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIR_ERR_CHKANDJUMP1(!is_equal, mpi_errno, MPI_ERR_OTHER, "**infonoteq", "**infonoteq %s", key);

    *value_ptr = hint_str;

  fn_exit:
    return MPI_SUCCESS;
  fn_fail:
    /* inconsistent info keys are ignored */
    *value_ptr = NULL;
    goto fn_exit;
}

/* Returns true if the communicator is node-aware and the number of processes in all the nodes are
 * same */
int MPII_Comm_is_node_balanced(MPIR_Comm * comm, int *num_nodes, bool * node_balanced)
{
    int i = 0;
    int mpi_errno = MPI_SUCCESS;
    int *ranks_per_node;
    *num_nodes = 0;

    MPIR_CHKPMEM_DECL();

    if (!MPIR_Comm_is_parent_comm(comm)) {
        *node_balanced = false;
        goto fn_exit;
    }

    /* Find maximum value in the internode_table */
    for (i = 0; i < comm->local_size; i++) {
        if (comm->internode_table[i] > *num_nodes) {
            *num_nodes = comm->internode_table[i];
        }
    }
    /* number of nodes is max_node_id + 1 */
    (*num_nodes)++;

    MPIR_CHKPMEM_CALLOC(ranks_per_node, *num_nodes * sizeof(int), MPL_MEM_OTHER);

    for (i = 0; i < comm->local_size; i++) {
        ranks_per_node[comm->internode_table[i]]++;
    }

    for (i = 1; i < *num_nodes; i++) {
        if (ranks_per_node[i - 1] != ranks_per_node[i]) {
            *node_balanced = false;
            goto fn_exit;
        }
    }

    *node_balanced = true;
  fn_exit:
    MPIR_CHKPMEM_REAP();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_save_inactive_request(MPIR_Comm * comm, MPIR_Request * request)
{
    MPID_THREAD_CS_ENTER(VCI, comm->mutex);
    HASH_ADD_INT(comm->persistent_requests, handle, request, MPL_MEM_COMM);
    MPID_THREAD_CS_EXIT(VCI, comm->mutex);

    return MPI_SUCCESS;
}

int MPIR_Comm_delete_inactive_request(MPIR_Comm * comm, MPIR_Request * request)
{
    MPID_THREAD_CS_ENTER(VCI, comm->mutex);
    HASH_DEL(comm->persistent_requests, request);
    MPID_THREAD_CS_EXIT(VCI, comm->mutex);

    return MPI_SUCCESS;
}

int MPIR_Comm_free_inactive_requests(MPIR_Comm * comm)
{
    MPIR_Request *request, *tmp;
    MPID_THREAD_CS_ENTER(VCI, comm->mutex);
    HASH_ITER(hh, comm->persistent_requests, request, tmp) {
        if (!MPIR_Request_is_active(request)) {
            HASH_DEL(comm->persistent_requests, request);
            /* reset request->comm so it won't trigger MPIR_Comm_delete_inactive_request,
             * which will recursively entering the lock. */
            if (request->comm) {
                MPIR_Comm_release(request->comm);
                request->comm = NULL;
            }
            MPL_internal_error_printf
                ("MPICH: freeing inactive persistent request %x on communicator %x.\n",
                 request->handle, comm->handle);
            MPIR_Request_free_impl(request);
        }
    }
    MPID_THREAD_CS_EXIT(VCI, comm->mutex);

    return MPI_SUCCESS;
}

void MPIR_Comm_set_session_ptr(MPIR_Comm * comm_ptr, MPIR_Session * session_ptr)
{
    if (session_ptr != NULL) {
        comm_ptr->session_ptr = session_ptr;
        /* Add ref counter of session */
        MPIR_Session_add_ref(session_ptr);
    }
}

void MPIR_get_memory_kinds_from_comm(MPIR_Comm * comm_ptr, char **kinds)
{
    if (comm_ptr->session_ptr) {
        *kinds = comm_ptr->session_ptr->memory_alloc_kinds;
    } else {
        *kinds = MPIR_Process.memory_alloc_kinds;
    }
}
