/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"
#include "mpiinfo.h"    /* MPIU_Info_free */

#include "mpl_utlist.h"
#include "mpiu_uthash.h"

/* This is the utility file for comm that contains the basic comm items
   and storage management */
#ifndef MPID_COMM_PREALLOC
#define MPID_COMM_PREALLOC 8
#endif

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CTXID_EAGER_SIZE
      category    : THREADS
      type        : int
      default     : 2
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The MPIR_CVAR_CTXID_EAGER_SIZE environment variable allows you to
        specify how many words in the context ID mask will be set aside
        for the eager allocation protocol.  If the application is running
        out of context IDs, reducing this value may help.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/


/* Preallocated comm objects */
/* initialized in initthread.c */
MPID_Comm MPID_Comm_builtin[MPID_COMM_N_BUILTIN] = { {0} };
MPID_Comm MPID_Comm_direct[MPID_COMM_PREALLOC]   = { {0} };
MPIU_Object_alloc_t MPID_Comm_mem = { 0, 0, 0, 0, MPID_COMM, 
				      sizeof(MPID_Comm), MPID_Comm_direct,
                                      MPID_COMM_PREALLOC};

/* Communicator creation functions */
struct MPID_CommOps *MPID_Comm_fns = NULL;
struct MPIR_Comm_hint_fn_elt {
    char name[MPI_MAX_INFO_KEY];
    MPIR_Comm_hint_fn_t fn;
    void *state;
    UT_hash_handle hh;
};
static struct MPIR_Comm_hint_fn_elt *MPID_hint_fns = NULL;

/* utility function to pretty print a context ID for debugging purposes, see
 * mpiimpl.h for more info on the various fields */
#ifdef USE_DBG_LOGGING
static void MPIR_Comm_dump_context_id(MPIR_Context_id_t context_id, char *out_str, int len)
{
    int subcomm_type = MPID_CONTEXT_READ_FIELD(SUBCOMM,context_id);
    const char *subcomm_type_name = NULL;

    switch (subcomm_type) {
        case 0: subcomm_type_name = "parent"; break;
        case 1: subcomm_type_name = "intranode"; break;
        case 2: subcomm_type_name = "internode"; break;
        default: MPIU_Assert(FALSE); break;
    }
    MPIU_Snprintf(out_str, len,
                  "context_id=%d (%#x): DYNAMIC_PROC=%d PREFIX=%#x IS_LOCALCOMM=%d SUBCOMM=%s SUFFIX=%s",
                  context_id,
                  context_id,
                  MPID_CONTEXT_READ_FIELD(DYNAMIC_PROC,context_id),
                  MPID_CONTEXT_READ_FIELD(PREFIX,context_id),
                  MPID_CONTEXT_READ_FIELD(IS_LOCALCOMM,context_id),
                  subcomm_type_name,
                  (MPID_CONTEXT_READ_FIELD(SUFFIX,context_id) ? "coll" : "pt2pt"));
}
#endif

/* FIXME :
   Reusing context ids can lead to a race condition if (as is desirable)
   MPI_Comm_free does not include a barrier.  Consider the following:
   Process A frees the communicator.
   Process A creates a new communicator, reusing the just released id
   Process B sends a message to A on the old communicator.
   Process A receives the message, and believes that it belongs to the
   new communicator.
   Process B then cancels the message, and frees the communicator.

   The likelyhood of this happening can be reduced by introducing a gap
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
int MPIR_Comm_init(MPID_Comm *comm_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Object_set_ref(comm_p, 1);

    MPIU_THREAD_MPI_OBJ_INIT(comm_p);

    /* Clear many items (empty means to use the default; some of these
       may be overridden within the upper-level communicator initialization) */
    comm_p->errhandler   = NULL;
    comm_p->attributes   = NULL;
    comm_p->remote_group = NULL;
    comm_p->local_group  = NULL;
    comm_p->coll_fns     = NULL;
    comm_p->topo_fns     = NULL;
    comm_p->name[0]      = '\0';
    comm_p->info         = NULL;

    comm_p->hierarchy_kind  = MPID_HIERARCHY_FLAT;
    comm_p->node_comm       = NULL;
    comm_p->node_roots_comm = NULL;
    comm_p->intranode_table = NULL;
    comm_p->internode_table = NULL;

    /* abstractions bleed a bit here... :( */
    comm_p->next_sched_tag = MPIR_FIRST_NBC_TAG;

    /* Initialize the revoked flag as false */
    comm_p->revoked = 0;

    /* Fields not set include context_id, remote and local size, and
       kind, since different communicator construction routines need
       different values */
fn_fail:
    return mpi_errno;
}


/*
    Create a communicator structure and perform basic initialization 
    (mostly clearing fields and updating the reference count).  
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_create
#undef FCNAME
#define FCNAME "MPIR_Comm_create"
int MPIR_Comm_create( MPID_Comm **newcomm_ptr )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *newptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_CREATE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_CREATE);

    newptr = (MPID_Comm *)MPIU_Handle_obj_alloc( &MPID_Comm_mem );
    MPIU_ERR_CHKANDJUMP(!newptr, mpi_errno, MPI_ERR_OTHER, "**nomem");

    *newcomm_ptr = newptr;

    mpi_errno = MPIR_Comm_init(newptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* Insert this new communicator into the list of known communicators.
       Make this conditional on debugger support to match the test in 
       MPIR_Comm_release . */
    MPIR_COMML_REMEMBER( newptr );

 fn_fail:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_CREATE);

    return mpi_errno;
}

/* Create a local intra communicator from the local group of the
   specified intercomm. */
/* FIXME this is an alternative constructor that doesn't use MPIR_Comm_create! */
#undef FUNCNAME
#define FUNCNAME MPIR_Setup_intercomm_localcomm
#undef FCNAME
#define FCNAME "MPIR_Setup_intercomm_localcomm"
int MPIR_Setup_intercomm_localcomm( MPID_Comm *intercomm_ptr )
{
    MPID_Comm *localcomm_ptr;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_SETUP_INTERCOMM_LOCALCOMM);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_SETUP_INTERCOMM_LOCALCOMM);

    localcomm_ptr = (MPID_Comm *)MPIU_Handle_obj_alloc( &MPID_Comm_mem );
    MPIU_ERR_CHKANDJUMP(!localcomm_ptr,mpi_errno,MPI_ERR_OTHER,"**nomem");

    /* get sensible default values for most fields (usually zeros) */
    mpi_errno = MPIR_Comm_init(localcomm_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* use the parent intercomm's recv ctx as the basis for our ctx */
    localcomm_ptr->recvcontext_id = MPID_CONTEXT_SET_FIELD(IS_LOCALCOMM, intercomm_ptr->recvcontext_id, 1);
    localcomm_ptr->context_id = localcomm_ptr->recvcontext_id;

    MPIU_DBG_MSG_FMT(COMM,TYPICAL,(MPIU_DBG_FDEST, "setup_intercomm_localcomm ic=%p ic->context_id=%d ic->recvcontext_id=%d lc->recvcontext_id=%d", intercomm_ptr, intercomm_ptr->context_id, intercomm_ptr->recvcontext_id, localcomm_ptr->recvcontext_id));

    /* Duplicate the VCRT references */
    MPID_VCRT_Add_ref( intercomm_ptr->local_vcrt );
    localcomm_ptr->vcrt = intercomm_ptr->local_vcrt;
    localcomm_ptr->vcr  = intercomm_ptr->local_vcr;

    /* Save the kind of the communicator */
    localcomm_ptr->comm_kind   = MPID_INTRACOMM;

    /* Set the sizes and ranks */
    localcomm_ptr->remote_size = intercomm_ptr->local_size;
    localcomm_ptr->local_size  = intercomm_ptr->local_size;
    localcomm_ptr->rank        = intercomm_ptr->rank;

    /* TODO More advanced version: if the group is available, dup it by 
       increasing the reference count instead of recreating it later */
    /* FIXME  : No coll_fns functions for the collectives */
    /* FIXME  : No local functions for the topology routines */

    intercomm_ptr->local_comm = localcomm_ptr;

    /* sets up the SMP-aware sub-communicators and tables */
    mpi_errno = MPIR_Comm_commit(localcomm_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_fail:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_SETUP_INTERCOMM_LOCALCOMM);

    return mpi_errno;
}

/* holds default collop "vtables" for _intracomms_, where
 * default[hierarchy_kind] is the pointer to the collop struct for that
 * hierarchy kind */
static struct MPID_Collops *default_collops[MPID_HIERARCHY_SIZE] = {NULL};
/* default for intercomms */
static struct MPID_Collops *ic_default_collops = NULL;

#undef FUNCNAME
#define FUNCNAME cleanup_default_collops
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int cleanup_default_collops(void *unused) {
    int i;
    for (i = 0; i < MPID_HIERARCHY_SIZE; ++i) {
        if (default_collops[i]) {
            MPIU_Assert(default_collops[i]->ref_count >= 1);
            if (--default_collops[i]->ref_count == 0)
                MPIU_Free(default_collops[i]);
            default_collops[i] = NULL;
        }
    }
    if (ic_default_collops) {
        MPIU_Assert(ic_default_collops->ref_count >= 1);
        if (--ic_default_collops->ref_count == 0)
            MPIU_Free(ic_default_collops);
    }
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME init_default_collops
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int init_default_collops(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    struct MPID_Collops *ops = NULL;
    MPIU_CHKPMEM_DECL(MPID_HIERARCHY_SIZE+1);

    /* first initialize the intracomms */
    for (i = 0; i < MPID_HIERARCHY_SIZE; ++i) {
        MPIU_CHKPMEM_CALLOC(ops, struct MPID_Collops *, sizeof(struct MPID_Collops), mpi_errno, "default intracomm collops");
        ops->ref_count = 1; /* force existence until finalize time */

        /* intracomm default defaults... */
        ops->Ibcast_sched = &MPIR_Ibcast_intra;
        ops->Ibarrier_sched = &MPIR_Ibarrier_intra;
        ops->Ireduce_sched = &MPIR_Ireduce_intra;
        ops->Ialltoall_sched = &MPIR_Ialltoall_intra;
        ops->Ialltoallv_sched = &MPIR_Ialltoallv_intra;
        ops->Ialltoallw_sched = &MPIR_Ialltoallw_intra;
        ops->Iallreduce_sched = &MPIR_Iallreduce_intra;
        ops->Igather_sched = &MPIR_Igather_intra;
        ops->Igatherv_sched = &MPIR_Igatherv;
        ops->Iscatter_sched = &MPIR_Iscatter_intra;
        ops->Iscatterv_sched = &MPIR_Iscatterv;
        ops->Ireduce_scatter_sched = &MPIR_Ireduce_scatter_intra;
        ops->Ireduce_scatter_block_sched = &MPIR_Ireduce_scatter_block_intra;
        ops->Iallgather_sched = &MPIR_Iallgather_intra;
        ops->Iallgatherv_sched = &MPIR_Iallgatherv_intra;
        ops->Iscan_sched = &MPIR_Iscan_rec_dbl;
        ops->Iexscan_sched = &MPIR_Iexscan;
        ops->Neighbor_allgather   = &MPIR_Neighbor_allgather_default;
        ops->Neighbor_allgatherv  = &MPIR_Neighbor_allgatherv_default;
        ops->Neighbor_alltoall    = &MPIR_Neighbor_alltoall_default;
        ops->Neighbor_alltoallv   = &MPIR_Neighbor_alltoallv_default;
        ops->Neighbor_alltoallw   = &MPIR_Neighbor_alltoallw_default;
        ops->Ineighbor_allgather  = &MPIR_Ineighbor_allgather_default;
        ops->Ineighbor_allgatherv = &MPIR_Ineighbor_allgatherv_default;
        ops->Ineighbor_alltoall   = &MPIR_Ineighbor_alltoall_default;
        ops->Ineighbor_alltoallv  = &MPIR_Ineighbor_alltoallv_default;
        ops->Ineighbor_alltoallw  = &MPIR_Ineighbor_alltoallw_default;

        /* override defaults, such as for SMP */
        switch (i) {
            case MPID_HIERARCHY_FLAT:
                break;
            case MPID_HIERARCHY_PARENT:
                ops->Ibcast_sched = &MPIR_Ibcast_SMP;
                ops->Iscan_sched = &MPIR_Iscan_SMP;
                ops->Iallreduce_sched = &MPIR_Iallreduce_SMP;
                ops->Ireduce_sched = &MPIR_Ireduce_SMP;
                break;
            case MPID_HIERARCHY_NODE:
                break;
            case MPID_HIERARCHY_NODE_ROOTS:
                break;

                /* --BEGIN ERROR HANDLING-- */
            default:
                MPIU_Assertp(FALSE);
                break;
                /* --END ERROR HANDLING-- */
        }

        /* this is a default table, it's not overriding another table */
        ops->prev_coll_fns = NULL;

        default_collops[i] = ops;
    }

    /* now the intercomm table */
    {
        MPIU_CHKPMEM_CALLOC(ops, struct MPID_Collops *, sizeof(struct MPID_Collops), mpi_errno, "default intercomm collops");
        ops->ref_count = 1; /* force existence until finalize time */

        /* intercomm defaults */
        ops->Ibcast_sched = &MPIR_Ibcast_inter;
        ops->Ibarrier_sched = &MPIR_Ibarrier_inter;
        ops->Ireduce_sched = &MPIR_Ireduce_inter;
        ops->Ialltoall_sched = &MPIR_Ialltoall_inter;
        ops->Ialltoallv_sched = &MPIR_Ialltoallv_inter;
        ops->Ialltoallw_sched = &MPIR_Ialltoallw_inter;
        ops->Iallreduce_sched = &MPIR_Iallreduce_inter;
        ops->Igather_sched = &MPIR_Igather_inter;
        ops->Igatherv_sched = &MPIR_Igatherv;
        ops->Iscatter_sched = &MPIR_Iscatter_inter;
        ops->Iscatterv_sched = &MPIR_Iscatterv;
        ops->Ireduce_scatter_sched = &MPIR_Ireduce_scatter_inter;
        ops->Ireduce_scatter_block_sched = &MPIR_Ireduce_scatter_block_inter;
        ops->Iallgather_sched = &MPIR_Iallgather_inter;
        ops->Iallgatherv_sched = &MPIR_Iallgatherv_inter;
        /* scan and exscan are not valid for intercommunicators, leave them NULL */
        /* Ineighbor_all* routines are not valid for intercommunicators, leave
         * them NULL */

        /* this is a default table, it's not overriding another table */
        ops->prev_coll_fns = NULL;

        ic_default_collops = ops;
    }


    /* run after MPID_Finalize to permit collective usage during finalize */
    MPIR_Add_finalize(cleanup_default_collops, NULL, MPIR_FINALIZE_CALLBACK_PRIO - 1);

    MPIU_CHKPMEM_COMMIT();
fn_exit:
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* Initializes the coll_fns field of comm to a sensible default.  It may re-use
 * an existing structure, so any override by a lower level should _not_ change
 * any of the fields but replace the coll_fns object instead.
 *
 * NOTE: for now we only initialize nonblocking collective routines, since the
 * blocking collectives all contain fallback logic that correctly handles NULL
 * override functions. */
#undef FUNCNAME
#define FUNCNAME set_collops
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int set_collops(MPID_Comm *comm)
{
    int mpi_errno = MPI_SUCCESS;
    static int initialized = FALSE;

    if (comm->coll_fns != NULL)
        goto fn_exit;

    if (unlikely(!initialized)) {
        mpi_errno = init_default_collops();
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        initialized = TRUE;
    }

    if (comm->comm_kind == MPID_INTRACOMM) {
        /* FIXME MT what protects access to this structure and ic_default_collops? */
        comm->coll_fns = default_collops[comm->hierarchy_kind];
    }
    else { /* intercomm */
        comm->coll_fns = ic_default_collops;
    }

    comm->coll_fns->ref_count++;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Provides a hook for the top level functions to perform some manipulation on a
   communicator just before it is given to the application level.
  
   For example, we create sub-communicators for SMP-aware collectives at this
   step. */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_commit
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Comm_commit(MPID_Comm *comm)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int num_local = -1, num_external = -1;
    int local_rank = -1, external_rank = -1;
    int *local_procs = NULL, *external_procs = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_COMMIT);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_COMMIT);

    /* It's OK to relax these assertions, but we should do so very
       intentionally.  For now this function is the only place that we create
       our hierarchy of communicators */
    MPIU_Assert(comm->node_comm == NULL);
    MPIU_Assert(comm->node_roots_comm == NULL);

    if (comm->comm_kind == MPID_INTRACOMM) {

        mpi_errno = MPIU_Find_local_and_external(comm,
                                                 &num_local,    &local_rank,    &local_procs,
                                                 &num_external, &external_rank, &external_procs,
                                                 &comm->intranode_table, &comm->internode_table);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno) {
            if (MPIR_Err_is_fatal(mpi_errno)) MPIU_ERR_POP(mpi_errno);

            /* Non-fatal errors simply mean that this communicator will not have
               any node awareness.  Node-aware collectives are an optimization. */
            MPIU_DBG_MSG_P(COMM,VERBOSE,"MPIU_Find_local_and_external failed for comm_ptr=%p", comm);
            if (comm->intranode_table)
                MPIU_Free(comm->intranode_table);
            if (comm->internode_table)
                MPIU_Free(comm->internode_table);

            mpi_errno = MPI_SUCCESS;
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */

        /* defensive checks */
        MPIU_Assert(num_local > 0);
        MPIU_Assert(num_local > 1 || external_rank >= 0);
        MPIU_Assert(external_rank < 0 || external_procs != NULL);

        /* if the node_roots_comm and comm would be the same size, then creating
           the second communicator is useless and wasteful. */
        if (num_external == comm->remote_size) {
            MPIU_Assert(num_local == 1);
            goto fn_exit;
        }

        /* we don't need a local comm if this process is the only one on this node */
        if (num_local > 1) {
            mpi_errno = MPIR_Comm_create(&comm->node_comm);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            comm->node_comm->context_id = comm->context_id + MPID_CONTEXT_INTRANODE_OFFSET;
            comm->node_comm->recvcontext_id = comm->node_comm->context_id;
            comm->node_comm->rank = local_rank;
            comm->node_comm->comm_kind = MPID_INTRACOMM;
            comm->node_comm->hierarchy_kind = MPID_HIERARCHY_NODE;
            comm->node_comm->local_comm = NULL;
            MPIU_DBG_MSG_D(CH3_OTHER,VERBOSE,"Create node_comm=%p\n", comm->node_comm);

            comm->node_comm->local_size  = num_local;
            comm->node_comm->remote_size = num_local;

            MPID_VCRT_Create( num_local, &comm->node_comm->vcrt );
            MPID_VCRT_Get_ptr( comm->node_comm->vcrt, &comm->node_comm->vcr );
            for (i = 0; i < num_local; ++i) {
                /* For rank i in the new communicator, find the corresponding
                   rank in the input communicator */
                MPID_VCR_Dup( comm->vcr[local_procs[i]], 
                              &comm->node_comm->vcr[i] );
            }

            mpi_errno = set_collops(comm->node_comm);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            /* Notify device of communicator creation */
            mpi_errno = MPID_Dev_comm_create_hook( comm->node_comm );
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            /* don't call MPIR_Comm_commit here */
        }


        /* this process may not be a member of the node_roots_comm */
        if (local_rank == 0) {
            mpi_errno = MPIR_Comm_create(&comm->node_roots_comm);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            comm->node_roots_comm->context_id = comm->context_id + MPID_CONTEXT_INTERNODE_OFFSET;
            comm->node_roots_comm->recvcontext_id = comm->node_roots_comm->context_id;
            comm->node_roots_comm->rank = external_rank;
            comm->node_roots_comm->comm_kind = MPID_INTRACOMM;
            comm->node_roots_comm->hierarchy_kind = MPID_HIERARCHY_NODE_ROOTS;
            comm->node_roots_comm->local_comm = NULL;

            comm->node_roots_comm->local_size  = num_external;
            comm->node_roots_comm->remote_size = num_external;

            MPID_VCRT_Create( num_external, &comm->node_roots_comm->vcrt );
            MPID_VCRT_Get_ptr( comm->node_roots_comm->vcrt, &comm->node_roots_comm->vcr );
            for (i = 0; i < num_external; ++i) {
                /* For rank i in the new communicator, find the corresponding
                   rank in the input communicator */
                MPID_VCR_Dup( comm->vcr[external_procs[i]], 
                              &comm->node_roots_comm->vcr[i] );
            }

            mpi_errno = set_collops(comm->node_roots_comm);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            /* Notify device of communicator creation */
            mpi_errno = MPID_Dev_comm_create_hook( comm->node_roots_comm );
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            /* don't call MPIR_Comm_commit here */
        }

        comm->hierarchy_kind = MPID_HIERARCHY_PARENT;
    }

fn_exit:
    if (!mpi_errno) {
        /* catch all of the early-bail, non-error cases */

        mpi_errno = set_collops(comm);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        /* Notify device of communicator creation */
        mpi_errno = MPID_Dev_comm_create_hook(comm);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    if (external_procs != NULL)
        MPIU_Free(external_procs);
    if (local_procs != NULL)
        MPIU_Free(local_procs);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_COMMIT);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Returns true if the given communicator is aware of node topology information,
   false otherwise.  Such information could be used to implement more efficient
   collective communication, for example. */
int MPIR_Comm_is_node_aware(MPID_Comm * comm)
{
    return (comm->hierarchy_kind == MPID_HIERARCHY_PARENT);
}

/* Returns true if the communicator is node-aware and processes in all the nodes
   are consecutive. For example, if node 0 contains "0, 1, 2, 3", node 1
   contains "4, 5, 6", and node 2 contains "7", we shall return true. */
int MPIR_Comm_is_node_consecutive(MPID_Comm * comm)
{
    int i = 0, curr_nodeidx = 0;
    int *internode_table = comm->internode_table;

    if (!MPIR_Comm_is_node_aware(comm))
        return 0;

    for (; i < comm->local_size; i++)
    {
        if (internode_table[i] == curr_nodeidx + 1)
            curr_nodeidx++;
        else if (internode_table[i] != curr_nodeidx)
            return 0;
    }

    return 1;
}

/*
 * Here are the routines to find a new context id.  The algorithm is discussed 
 * in detail in the mpich coding document.  There are versions for
 * single threaded and multithreaded MPI.
 *
 * Both the threaded and non-threaded routines use the same mask of
 * available context id values.
 */
static uint32_t context_mask[MPIR_MAX_CONTEXT_MASK];
static int initialize_context_mask = 1;

/* Create a string that contains the context mask.  This is
   used only with the logging interface, and must be used by one thread at 
   a time (should this be enforced by the logging interface?).
   Converts the mask to hex and returns a pointer to that string.

   Callers should own the context ID critical section, or should be prepared to
   suffer data races in any fine-grained locking configuration.

   This routine is no longer static in order to allow advanced users and
   developers to debug context ID problems "in the field".  We provide a
   prototype here to keep the compiler happy, but users will need to put a
   (possibly "extern") copy of the prototype in their own code in order to call
   this routine.
 */
char *MPIR_ContextMaskToStr( void );
char *MPIR_ContextMaskToStr( void )
{
    static char bufstr[MPIR_MAX_CONTEXT_MASK*8+1];
    int i;
    int maxset=0;

    for (maxset=MPIR_MAX_CONTEXT_MASK-1; maxset>=0; maxset--) {
	if (context_mask[maxset] != 0) break;
    }

    for (i=0; i<maxset; i++) {
	MPIU_Snprintf( &bufstr[i*8], 9, "%.8x", context_mask[i] );
    }
    return bufstr;
}

/* Returns useful debugging information about the context ID mask bit-vector.
 * This includes the total number of possibly valid IDs (the size of the ID
 * space) and the number of free IDs remaining in the mask.  NULL arguments are
 * fine, they will be ignored.
 *
 * This routine is for debugging in very particular situations and does not
 * attempt to control concurrent access to the mask vector.
 *
 * Callers should own the context ID critical section, or should be prepared to
 * suffer data races in any fine-grained locking configuration.
 *
 * The routine is non-static in order to permit "in the field debugging".  We
 * provide a prototype here to keep the compiler happy. */
void MPIR_ContextMaskStats(int *free_ids, int *total_ids);
void MPIR_ContextMaskStats(int *free_ids, int *total_ids)
{
    if (free_ids) {
        int i, j;
        *free_ids = 0;

        /* if this ever needs to be fast, use a lookup table to do a per-nibble
         * or per-byte lookup of the popcount instead of checking each bit at a
         * time (or just track the count when manipulating the mask and keep
         * that count stored in a variable) */
        for (i = 0; i < MPIR_MAX_CONTEXT_MASK; ++i) {
            for (j = 0; j < sizeof(context_mask[0])*8; ++j) {
                *free_ids += (context_mask[i] & (0x1 << j)) >> j;
            }
        }
    }
    if (total_ids) {
        *total_ids = MPIR_MAX_CONTEXT_MASK*sizeof(context_mask[0])*8;
    }
}

#ifdef MPICH_DEBUG_HANDLEALLOC
static int MPIU_CheckContextIDsOnFinalize(void *context_mask_ptr)
{
    int i;
    uint32_t *mask = context_mask_ptr;
    /* the predefined communicators should be freed by this point, so we don't
     * need to special case bits 0,1, and 2 */
    for (i = 0; i < MPIR_MAX_CONTEXT_MASK; ++i) {
        if (~mask[i]) {
            /* some bits were still cleared */
            printf("leaked context IDs detected: mask=%p mask[%d]=%#x\n", mask, i, (int)mask[i]);
        }
    }
    return MPI_SUCCESS;
}
#endif

static void MPIR_Init_contextid(void)
{
    int i;

    for (i=1; i<MPIR_MAX_CONTEXT_MASK; i++) {
	context_mask[i] = 0xFFFFFFFF;
    }
    /* The first two values are already used (comm_world, comm_self).
       The third value is also used for the internal-only copy of
       comm_world, if needed by mpid. */
#ifdef MPID_NEEDS_ICOMM_WORLD
    context_mask[0] = 0xFFFFFFF8;
#else
    context_mask[0] = 0xFFFFFFFC;
#endif
    initialize_context_mask = 0;

#ifdef MPICH_DEBUG_HANDLEALLOC
    /* check for context ID leaks in MPI_Finalize.  Use (_PRIO-1) to make sure
     * that we run after MPID_Finalize. */
    MPIR_Add_finalize(MPIU_CheckContextIDsOnFinalize, context_mask,
                      MPIR_FINALIZE_CALLBACK_PRIO - 1);
#endif
}

/* Return the context id corresponding to the first set bit in the mask.
   Return 0 if no bit found.  This function does _not_ alter local_mask. */
static int MPIR_Locate_context_bit(uint32_t local_mask[])
{
    int i, j, context_id = 0;
    for (i=0; i<MPIR_MAX_CONTEXT_MASK; i++) {
	if (local_mask[i]) {
	    /* There is a bit set in this word. */
	    register uint32_t     val, nval;
	    /* The following code finds the highest set bit by recursively
	       checking the top half of a subword for a bit, and incrementing
	       the bit location by the number of bit of the lower sub word if 
	       the high subword contains a set bit.  The assumption is that
	       full-word bitwise operations and compares against zero are 
	       fast */
	    val = local_mask[i];
	    j   = 0;
	    nval = val & 0xFFFF0000;
	    if (nval) {
		j += 16;
		val = nval;
	    }
	    nval = val & 0xFF00FF00;
	    if (nval) {
		j += 8;
		val = nval;
	    }
	    nval = val & 0xF0F0F0F0;
	    if (nval) {
		j += 4;
		val = nval;
	    }
	    nval = val & 0xCCCCCCCC;
	    if (nval) {
		j += 2;
		val = nval;
	    }
	    if (val & 0xAAAAAAAA) {
		j += 1;
	    }
	    context_id = (MPIR_CONTEXT_INT_BITS * i + j) << MPID_CONTEXT_PREFIX_SHIFT;
	    return context_id;
	}
    }
    return 0;
}

/* Allocates a context ID from the given mask by clearing the bit
 * corresponding to the the given id.  Returns 0 on failure, id on
 * success. */
static int MPIR_Allocate_context_bit(uint32_t mask[], MPIR_Context_id_t id)
{
    int raw_prefix, idx, bitpos;
    raw_prefix = MPID_CONTEXT_READ_FIELD(PREFIX,id);
    idx    = raw_prefix / MPIR_CONTEXT_INT_BITS;
    bitpos = raw_prefix % MPIR_CONTEXT_INT_BITS;

    /* the bit should not already be cleared (allocated) */
    MPIU_Assert(mask[idx] & (1<<bitpos));

    /* clear the bit */
    mask[idx] &= ~(1<<bitpos);

    MPIU_DBG_MSG_FMT(COMM,VERBOSE,(MPIU_DBG_FDEST,
            "allocating contextid = %d, (mask=%p, mask[%d], bit %d)",
            id, mask, idx, bitpos));
    return id;
}

/* Allocates the first available context ID from context_mask based on the available
 * bits given in local_mask.  This function will clear the corresponding bit in
 * context_mask if allocation was successful.
 *
 * Returns 0 on failure.  Returns the allocated context ID on success. */
static int MPIR_Find_and_allocate_context_id(uint32_t local_mask[])
{
    MPIR_Context_id_t context_id;
    context_id = MPIR_Locate_context_bit(local_mask);
    if (context_id != 0) {
        context_id = MPIR_Allocate_context_bit(context_mask, context_id);
    }
    return context_id;
}

/* Older, simpler interface.  Allocates a context ID collectively over the given
 * communicator. */
#undef FUNCNAME
#define FUNCNAME MPIR_Get_contextid
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Get_contextid(MPID_Comm *comm_ptr, MPIR_Context_id_t *context_id)
{
    int mpi_errno = MPI_SUCCESS;
    mpi_errno = MPIR_Get_contextid_sparse(comm_ptr, context_id, FALSE);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_Assert(*context_id != MPIR_INVALID_CONTEXT_ID);
fn_fail:
    return mpi_errno;
}


/* EAGER CONTEXT ID ALLOCATION: Attempt to allocate the context ID during the
 * initial synchronization step.  If eager protocol fails, threads fall back to
 * the base algorithm.
 *
 * They are used to avoid deadlock in multi-threaded case. In single-threaded
 * case, they are not used.
 */
static volatile int eager_nelem     = -1;
static volatile int eager_in_use    = 0;

/* In multi-threaded case, mask_in_use is used to maintain thread safety. In
 * single-threaded case, it is always 0. */
static volatile int mask_in_use     = 0;

/* In multi-threaded case, lowestContextId is used to prioritize access when
 * multiple threads are contending for the mask, lowestTag is used to break
 * ties when MPI_Comm_create_group is invoked my multiple threads on the same
 * parent communicator.  In single-threaded case, lowestContextId is always
 * set to parent context id in sched_cb_gcn_copy_mask and lowestTag is not
 * used.
 */
#define MPIR_MAXID (1 << 30)
static volatile int lowestContextId = MPIR_MAXID;
static volatile int lowestTag       = -1;

#ifndef MPICH_IS_THREADED
/* Unthreaded (only one MPI call active at any time) */

#undef FUNCNAME
#define FUNCNAME MPIR_Get_contextid_sparse
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Get_contextid_sparse(MPID_Comm *comm_ptr, MPIR_Context_id_t *context_id, int ignore_id)
{
    return MPIR_Get_contextid_sparse_group(comm_ptr, NULL /* group_ptr */, MPIR_Process.attrs.tag_ub /* tag */, context_id, ignore_id);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Get_contextid_sparse_group
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Get_contextid_sparse_group(MPID_Comm *comm_ptr, MPID_Group *group_ptr, int tag, MPIR_Context_id_t *context_id, int ignore_id)
{
    int mpi_errno = MPI_SUCCESS;
    uint32_t     local_mask[MPIR_MAX_CONTEXT_MASK];
    int errflag = FALSE;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_GET_CONTEXTID);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_GET_CONTEXTID);

    *context_id = 0;

    if (initialize_context_mask) {
	MPIR_Init_contextid();
    }

    if (ignore_id) {
        /* We are not participating in the resulting communicator, so our
         * context ID space doesn't matter.  Set the mask to "all available". */
        memset(local_mask, 0xff, MPIR_MAX_CONTEXT_MASK * sizeof(int));
    }
    else {
        MPIU_Memcpy( local_mask, context_mask, MPIR_MAX_CONTEXT_MASK * sizeof(int) );
    }

    /* Note that this is the unthreaded version */
    /* Comm must be an intracommunicator */
    if (group_ptr != NULL) {
        int coll_tag = tag | MPIR_Process.tagged_coll_mask; /* Shift tag into the tagged coll space */
        mpi_errno = MPIR_Allreduce_group( MPI_IN_PLACE, local_mask, MPIR_MAX_CONTEXT_MASK,
                                            MPI_INT, MPI_BAND, comm_ptr, group_ptr, coll_tag, &errflag );
    } else {
        mpi_errno = MPIR_Allreduce_impl( MPI_IN_PLACE, local_mask, MPIR_MAX_CONTEXT_MASK,
                                            MPI_INT, MPI_BAND, comm_ptr, &errflag );
    }

    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    if (ignore_id) {
        *context_id = MPIR_Locate_context_bit(local_mask);
        if (*context_id == 0) {
            int nfree = -1;
            int ntotal = -1;
            MPIR_ContextMaskStats(&nfree, &ntotal);
            MPIU_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER,
                                 "**toomanycomm", "**toomanycomm %d %d %d",
                                 nfree, ntotal, ignore_id);
        }
    }
    else {
        *context_id = MPIR_Find_and_allocate_context_id(local_mask);
        if (*context_id == 0) {
            int nfree = -1;
            int ntotal = -1;
            MPIR_ContextMaskStats(&nfree, &ntotal);
            MPIU_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER,
                                 "**toomanycomm", "**toomanycomm %d %d %d",
                                 nfree, ntotal, ignore_id);
        }
    }

fn_exit:
    if (ignore_id)
        *context_id = MPIR_INVALID_CONTEXT_ID;
    MPIU_DBG_MSG_S(COMM,VERBOSE,"Context mask = %s",MPIR_ContextMaskToStr());

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_GET_CONTEXTID);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#else /* MPICH_IS_THREADED is set and true */

#undef FUNCNAME
#define FUNCNAME MPIR_Get_contextid_sparse
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Get_contextid_sparse(MPID_Comm *comm_ptr, MPIR_Context_id_t *context_id, int ignore_id)
{
    return MPIR_Get_contextid_sparse_group(comm_ptr, NULL /*group_ptr*/,
                                           MPIR_Process.attrs.tag_ub /*tag*/,
                                           context_id, ignore_id);
}

/* Allocates a new context ID collectively over the given communicator.  This
 * routine is "sparse" in the sense that while it is collective, some processes
 * may not care about the value selected context ID.
 *
 * One example of this case is processes who pass MPI_UNDEFINED as the color
 * value to MPI_Comm_split.  Such processes should pass ignore_id==TRUE to
 * obtain the best performance and utilization of the context ID space.
 *
 * Processes that pass ignore_id==TRUE will receive
 * (*context_id==MPIR_INVALID_CONTEXT_ID) and should not attempt to use it.
 *
 * If a group pointer is given, the call is _not_ sparse, and only processes
 * in the group should call this routine.  That is, it is collective only over
 * the given group.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Get_contextid_sparse_group
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Get_contextid_sparse_group(MPID_Comm *comm_ptr, MPID_Group *group_ptr, int tag, MPIR_Context_id_t *context_id, int ignore_id)
{
    int mpi_errno = MPI_SUCCESS;
    const int ALL_OWN_MASK_FLAG = MPIR_MAX_CONTEXT_MASK;
    uint32_t local_mask[MPIR_MAX_CONTEXT_MASK+1];
    int own_mask = 0;
    int own_eager_mask = 0;
    int errflag = FALSE;
    int first_iter = 1;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_GET_CONTEXTID);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_GET_CONTEXTID);

    /* Group-collective and ignore_id should never be combined */
    MPIU_Assert(! (group_ptr != NULL && ignore_id) );

    *context_id = 0;

    MPIU_DBG_MSG_FMT(COMM, VERBOSE, (MPIU_DBG_FDEST,
         "Entering; shared state is %d:%d:%d, my ctx id is %d, tag=%d",
         mask_in_use, lowestContextId, lowestTag, comm_ptr->context_id, tag));

    while (*context_id == 0) {
        /* We lock only around access to the mask (except in the global locking
         * case).  If another thread is using the mask, we take a mask of zero. */
        MPIU_THREAD_CS_ENTER(CONTEXTID,);

        if (initialize_context_mask) {
            MPIR_Init_contextid();
        }

        if (eager_nelem < 0) {
            /* Ensure that at least one word of deadlock-free context IDs is
               always set aside for the base protocol */
            MPIU_Assert( MPIR_CVAR_CTXID_EAGER_SIZE >= 0 && MPIR_CVAR_CTXID_EAGER_SIZE < MPIR_MAX_CONTEXT_MASK-1 );
            eager_nelem = MPIR_CVAR_CTXID_EAGER_SIZE;
        }

        if (ignore_id) {
            /* We are not participating in the resulting communicator, so our
             * context ID space doesn't matter.  Set the mask to "all available". */
            memset(local_mask, 0xff, MPIR_MAX_CONTEXT_MASK * sizeof(int));
            own_mask = 0;
            /* don't need to touch mask_in_use/lowestContextId b/c our thread
             * doesn't ever need to "win" the mask */
        }

        /* Deadlock avoidance: Only participate in context id loop when all
         * processes have called this routine.  On the first iteration, use the
         * "eager" allocation protocol.
         */
        else if (first_iter) {
            memset(local_mask, 0, MPIR_MAX_CONTEXT_MASK * sizeof(int));
            own_eager_mask = 0;

            /* Attempt to reserve the eager mask segment */
            if (!eager_in_use && eager_nelem > 0) {
                int i;
                for (i = 0; i < eager_nelem; i++)
                    local_mask[i] = context_mask[i];

                eager_in_use   = 1;
                own_eager_mask = 1;
            }
        }

        else {
            /* lowestTag breaks ties when contextIds are the same (happens only
               in calls to MPI_Comm_create_group. */
            if (comm_ptr->context_id < lowestContextId ||
                    (comm_ptr->context_id == lowestContextId && tag < lowestTag)) {
                lowestContextId = comm_ptr->context_id;
                lowestTag       = tag;
            }

            if (mask_in_use || ! (comm_ptr->context_id == lowestContextId && tag == lowestTag)) {
                memset(local_mask, 0, MPIR_MAX_CONTEXT_MASK * sizeof(int));
                own_mask = 0;
                MPIU_DBG_MSG_D(COMM, VERBOSE, "In in-use, set lowestContextId to %d", lowestContextId);
            }
            else {
                int i;

                /* Copy safe mask segment to local_mask */
                for (i = 0; i < eager_nelem; i++)
                    local_mask[i] = 0;
                for (i = eager_nelem; i < MPIR_MAX_CONTEXT_MASK; i++)
                    local_mask[i] = context_mask[i];

                mask_in_use     = 1;
                own_mask        = 1;
                MPIU_DBG_MSG(COMM, VERBOSE, "Copied local_mask");
            }
        }
        MPIU_THREAD_CS_EXIT(CONTEXTID,);

        /* Note: MPIR_MAX_CONTEXT_MASK elements of local_mask are used by the
         * context ID allocation algorithm.  The additional element is ignored
         * by the context ID mask access routines and is used as a flag for
         * detecting context ID exhaustion (explained below). */
        if (own_mask || ignore_id)
            local_mask[ALL_OWN_MASK_FLAG] = 1;
        else
            local_mask[ALL_OWN_MASK_FLAG] = 0;

        /* Now, try to get a context id */
        MPIU_Assert(comm_ptr->comm_kind == MPID_INTRACOMM);
        /* In the global and brief-global cases, note that this routine will
           release that global lock when it needs to wait.  That will allow
           other processes to enter the global or brief global critical section.
         */
        if (group_ptr != NULL) {
            int coll_tag = tag | MPIR_Process.tagged_coll_mask; /* Shift tag into the tagged coll space */
            mpi_errno = MPIR_Allreduce_group(MPI_IN_PLACE, local_mask, MPIR_MAX_CONTEXT_MASK+1,
                                             MPI_INT, MPI_BAND, comm_ptr, group_ptr, coll_tag, &errflag);
        } else {
            mpi_errno = MPIR_Allreduce_impl(MPI_IN_PLACE, local_mask, MPIR_MAX_CONTEXT_MASK+1,
                                            MPI_INT, MPI_BAND, comm_ptr, &errflag);
        }
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* MT FIXME 2/3 cases don't seem to need the CONTEXTID CS, check and
         * narrow this region */
        MPIU_THREAD_CS_ENTER(CONTEXTID,);
        if (ignore_id) {
            /* we don't care what the value was, but make sure that everyone
             * who did care agreed on a value */
            *context_id = MPIR_Locate_context_bit(local_mask);
            /* used later in out-of-context ids check and outer while loop condition */
        }
        else if (own_eager_mask) {
            /* There is a chance that we've found a context id */
            /* Find_and_allocate_context_id updates the context_mask if it finds a match */
            *context_id = MPIR_Find_and_allocate_context_id(local_mask);
            MPIU_DBG_MSG_D(COMM, VERBOSE, "Context id is now %hd", *context_id);

            own_eager_mask = 0;
            eager_in_use   = 0;

            if (*context_id <= 0) {
                /* else we did not find a context id. Give up the mask in case
                 * there is another thread (with a lower input context id)
                 * waiting for it.  We need to ensure that any other threads
                 * have the opportunity to run, hence yielding */
                MPIU_THREAD_CS_YIELD(CONTEXTID,);
            }
        }
        else if (own_mask) {
            /* There is a chance that we've found a context id */
            /* Find_and_allocate_context_id updates the context_mask if it finds a match */
            *context_id = MPIR_Find_and_allocate_context_id(local_mask);
            MPIU_DBG_MSG_D(COMM, VERBOSE, "Context id is now %hd", *context_id);

            mask_in_use = 0;

            if (*context_id > 0) {
                /* If we were the lowest context id, reset the value to
                   allow the other threads to compete for the mask */
                if (lowestContextId == comm_ptr->context_id && lowestTag == tag) {
                    lowestContextId = MPIR_MAXID;
                    lowestTag       = -1;
                    /* Else leave it alone; there is another thread waiting */
                }
            }
            else {
                /* else we did not find a context id. Give up the mask in case
                 * there is another thread (with a lower input context id)
                 * waiting for it.  We need to ensure that any other threads
                 * have the opportunity to run, hence yielding */
                MPIU_THREAD_CS_YIELD(CONTEXTID,);
            }
        }
        else {
            /* As above, force this thread to yield */
            MPIU_THREAD_CS_YIELD(CONTEXTID,);
        }
        MPIU_THREAD_CS_EXIT(CONTEXTID,);

        /* Test for context ID exhaustion: All threads that will participate in
         * the new communicator owned the mask and could not allocate a context
         * ID.  This indicates that either some process has no context IDs
         * available, or that some are available, but the allocation cannot
         * succeed because there is no common context ID. */
        if (*context_id == 0 && local_mask[ALL_OWN_MASK_FLAG] == 1) {
            /* --BEGIN ERROR HANDLING-- */
            int nfree = 0;
            int ntotal = 0;
            int minfree;

            if (own_mask) {
                MPIU_THREAD_CS_ENTER(CONTEXTID,);
                mask_in_use = 0;
                if (lowestContextId == comm_ptr->context_id && lowestTag == tag) {
                    lowestContextId = MPIR_MAXID;
                    lowestTag       = -1;
                }
                MPIU_THREAD_CS_EXIT(CONTEXTID,);
            }

            MPIR_ContextMaskStats(&nfree, &ntotal);
            if (ignore_id)
                minfree = INT_MAX;
            else
                minfree = nfree;

            if (group_ptr != NULL) {
                int coll_tag = tag | MPIR_Process.tagged_coll_mask; /* Shift tag into the tagged coll space */
                mpi_errno = MPIR_Allreduce_group(MPI_IN_PLACE, &minfree, 1, MPI_INT, MPI_MIN,
                                                 comm_ptr, group_ptr, coll_tag, &errflag);
            } else {
                mpi_errno = MPIR_Allreduce_impl(MPI_IN_PLACE, &minfree, 1, MPI_INT,
                                                 MPI_MIN, comm_ptr, &errflag);
            }

            if (minfree > 0) {
                MPIU_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER,
                                     "**toomanycommfrag", "**toomanycommfrag %d %d %d",
                                     nfree, ntotal, ignore_id);
            } else {
                MPIU_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER,
                                     "**toomanycomm", "**toomanycomm %d %d %d",
                                     nfree, ntotal, ignore_id);
            }
            /* --END ERROR HANDLING-- */
        }

        first_iter = 0;
    }


fn_exit:
    if (ignore_id)
        *context_id = MPIR_INVALID_CONTEXT_ID;
    MPIU_DBG_MSG_S(COMM,VERBOSE,"Context mask = %s",MPIR_ContextMaskToStr());
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_GET_CONTEXTID);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    /* Release the masks */
    if (own_mask) {
        /* is it safe to access this without holding the CS? */
        mask_in_use = 0;
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#endif

struct gcn_state {
    MPIR_Context_id_t *ctx0;
    MPIR_Context_id_t *ctx1;
    int own_mask;
    int own_eager_mask;
    int first_iter;
    MPID_Comm *comm_ptr;
    MPID_Comm *comm_ptr_inter;
    MPID_Sched_t s;
    MPID_Comm_kind_t gcn_cid_kind;
    uint32_t local_mask[MPIR_MAX_CONTEXT_MASK];
};

static int sched_cb_gcn_copy_mask(MPID_Comm *comm, int tag, void *state);
static int sched_cb_gcn_allocate_cid(MPID_Comm *comm, int tag, void *state);
static int sched_cb_gcn_bcast(MPID_Comm *comm, int tag, void *state);

#undef FUNCNAME
#define FUNCNAME sched_cb_gcn_bcast
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int sched_cb_gcn_bcast(MPID_Comm *comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    struct gcn_state *st = state;

    if (st->gcn_cid_kind == MPID_INTERCOMM) {
        if (st->comm_ptr_inter->rank == 0) {
            mpi_errno = MPID_Sched_recv(st->ctx1, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, st->comm_ptr_inter, st->s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            mpi_errno = MPID_Sched_send(st->ctx0, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, st->comm_ptr_inter, st->s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            MPID_SCHED_BARRIER(st->s);
        }

        mpi_errno = st->comm_ptr->coll_fns->Ibcast_sched(st->ctx1, 1,
                MPIR_CONTEXT_ID_T_DATATYPE, 0, st->comm_ptr, st->s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPID_SCHED_BARRIER(st->s);
    }

    mpi_errno = MPID_Sched_cb(&MPIR_Sched_cb_free_buf, st, st->s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

fn_fail:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME sched_cb_gcn_allocate_cid
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int sched_cb_gcn_allocate_cid(MPID_Comm *comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    struct gcn_state *st = state;
    MPIR_Context_id_t newctxid;

    MPIU_THREAD_CS_ENTER(CONTEXTID,);
    if (st->own_eager_mask) {
        newctxid = MPIR_Find_and_allocate_context_id(st->local_mask);
        if (st->ctx0)
            *st->ctx0 = newctxid;
        if (st->ctx1)
            *st->ctx1 = newctxid;

        st->own_eager_mask = 0;
        eager_in_use = 0;

        if (newctxid <= 0) {
            /* else we did not find a context id. Give up the mask in case
             * there is another thread (with a lower input context id)
             * waiting for it.  We need to ensure that any other threads
             * have the opportunity to run, hence yielding */
            MPIU_THREAD_CS_YIELD(CONTEXTID,);
        }
    } else if (st->own_mask) {
        newctxid = MPIR_Find_and_allocate_context_id(st->local_mask);

        if (st->ctx0)
            *st->ctx0 = newctxid;
        if (st->ctx1)
            *st->ctx1 = newctxid;

        /* reset flags for the next try */
        mask_in_use = 0;

        if (newctxid > 0) {
            if (lowestContextId == st->comm_ptr->context_id)
                lowestContextId = MPIR_MAXID;
        } else {
            /* else we did not find a context id. Give up the mask in case
             * there is another thread (with a lower input context id)
             * waiting for it.  We need to ensure that any other threads
             * have the opportunity to run, hence yielding */
            MPIU_THREAD_CS_YIELD(CONTEXTID,);
        }
    } else {
        /* As above, force this thread to yield */
        MPIU_THREAD_CS_YIELD(CONTEXTID,);
    }

    if (*st->ctx0 == 0) {
        /* do not own mask, try again */
        mpi_errno = MPID_Sched_cb(&sched_cb_gcn_copy_mask, st, st->s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPID_SCHED_BARRIER(st->s);
    } else {
        /* Successfully allocated a context id */
        mpi_errno = MPID_Sched_cb(&sched_cb_gcn_bcast, st, st->s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPID_SCHED_BARRIER(st->s);
    }

    MPIU_THREAD_CS_EXIT(CONTEXTID,);

    /* --BEGIN ERROR HANDLING-- */
    /* --END ERROR HANDLING-- */
fn_fail:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME sched_cb_gcn_copy_mask
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int sched_cb_gcn_copy_mask(MPID_Comm *comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    struct gcn_state *st = state;

    MPIU_THREAD_CS_ENTER(CONTEXTID,);
    if (st->first_iter) {
        memset(st->local_mask, 0, MPIR_MAX_CONTEXT_MASK * sizeof(int));
        st->own_eager_mask = 0;

        /* Attempt to reserve the eager mask segment */
        if (!eager_in_use && eager_nelem > 0) {
            int i;
            for (i = 0; i < eager_nelem; i++)
                st->local_mask[i] = context_mask[i];

            eager_in_use   = 1;
            st->own_eager_mask = 1;
        }
        st->first_iter = 0;
    } else {
        if (st->comm_ptr->context_id < lowestContextId) {
            lowestContextId = st->comm_ptr->context_id;
        }
        if (mask_in_use || (st->comm_ptr->context_id != lowestContextId)) {
            memset(st->local_mask, 0, MPIR_MAX_CONTEXT_MASK * sizeof(int));
            st->own_mask = 0;
        } else {
            /* Copy safe mask segment to local_mask */
            int i;
            for (i = 0; i < eager_nelem; i++)
                st->local_mask[i] = 0;
            for (i = eager_nelem; i < MPIR_MAX_CONTEXT_MASK; i++)
                st->local_mask[i] = context_mask[i];

            mask_in_use = 1;
            st->own_mask = 1;
        }
    }
    MPIU_THREAD_CS_EXIT(CONTEXTID,);

    mpi_errno = st->comm_ptr->coll_fns->Iallreduce_sched(MPI_IN_PLACE, st->local_mask, MPIR_MAX_CONTEXT_MASK,
                                               MPI_UINT32_T, MPI_BAND, st->comm_ptr, st->s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPID_SCHED_BARRIER(st->s);

    mpi_errno = MPID_Sched_cb(&sched_cb_gcn_allocate_cid, st, st->s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPID_SCHED_BARRIER(st->s);

fn_fail:
    return mpi_errno;
}


/** Allocating a new context ID collectively over the given communicator in a
 * nonblocking way.
 *
 * The nonblocking mechanism is implemented by inserting MPIDU_Sched_entry to
 * the nonblocking collective progress, which is a part of the progress engine.
 * It uses a two-level linked list 'all_schedules' to manager all nonblocking
 * collective calls: the first level is a linked list of struct MPIDU_Sched;
 * and each struct MPIDU_Sched is an array of struct MPIDU_Sched_entry. The
 * following four functions are used together to implement the algorithm:
 * sched_cb_gcn_copy_mask, sched_cb_gcn_allocate_cid, sched_cb_gcn_bcast and
 * sched_get_cid_nonblock.
 *
 * The above four functions use the same algorithm as
 * MPIR_Get_contextid_sparse_group (multi-threaded version) to allocate a
 * context id. The algorithm needs to retry the allocation process in the case
 * of conflicts. In MPIR_Get_contextid_sparse_group, it is a while loop.  In
 * the nonblocking algorithm, 1) new entries are inserted to the end of
 * schedule to replace the 'while' loop in MPI_Comm_dup algorithm; 2) all
 * arguments passed to sched_get_cid_nonblock are saved to gcn_state in order
 * to be called in the future; 3) in sched_cb_gcn_allocate_cid, if the first
 * try failed, it will insert sched_cb_gcn_copy_mask to the schedule again.
 *
 * To ensure thread-safety, it shares the same global flag 'mask_in_use' with
 * other communicator functions to protect access to context_mask. And use
 * CONTEXTID lock to protect critical sections.
 *
 * There is a subtle difference between INTRACOMM and INTERCOMM when
 * duplicating a communicator.  They needed to be treated differently in
 * current algorithm. Specifically, 1) when calling sched_get_cid_nonblock, the
 * parameters are different; 2) updating newcommp->recvcontext_id in
 * MPIR_Get_intercomm_contextid_nonblock has been moved to sched_cb_gcn_bcast
 * because this should happen after sched_cb_gcn_allocate_cid has succeed.
 *
 * To avoid deadlock or livelock, it uses the same eager protocol as
 * multi-threaded MPIR_Get_contextid_sparse_group.
 */
#undef FUNCNAME
#define FUNCNAME sched_get_cid_nonblock
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int sched_get_cid_nonblock(MPID_Comm *comm_ptr, MPIR_Context_id_t *ctx0,
        MPIR_Context_id_t *ctx1, MPID_Sched_t s, MPID_Comm_kind_t gcn_cid_kind)
{
    int mpi_errno = MPI_SUCCESS;
    struct gcn_state *st = NULL;
    MPIU_CHKPMEM_DECL(1);

    MPIU_THREAD_CS_ENTER(CONTEXTID,);
    if (initialize_context_mask) {
        MPIR_Init_contextid();
    }

    MPIU_CHKPMEM_MALLOC(st, struct gcn_state *, sizeof(struct gcn_state), mpi_errno, "gcn_state");
    st->ctx0 = ctx0;
    st->ctx1 = ctx1;
    if (gcn_cid_kind == MPID_INTRACOMM) {
        st->comm_ptr = comm_ptr;
        st->comm_ptr_inter = NULL;
    } else {
        st->comm_ptr = comm_ptr->local_comm;
        st->comm_ptr_inter = comm_ptr;
    }
    st->s = s;
    st->gcn_cid_kind = gcn_cid_kind;
    *(st->ctx0) = 0;
    st->own_eager_mask = 0;
    st->first_iter = 1;
    if (eager_nelem < 0) {
        /* Ensure that at least one word of deadlock-free context IDs is
           always set aside for the base protocol */
        MPIU_Assert( MPIR_CVAR_CTXID_EAGER_SIZE >= 0 && MPIR_CVAR_CTXID_EAGER_SIZE < MPIR_MAX_CONTEXT_MASK-1 );
        eager_nelem = MPIR_CVAR_CTXID_EAGER_SIZE;
    }
    MPIU_THREAD_CS_EXIT(CONTEXTID,);

    mpi_errno = MPID_Sched_cb(&sched_cb_gcn_copy_mask, st, s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPID_SCHED_BARRIER(s);

    MPIU_CHKPMEM_COMMIT();
fn_exit:
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIR_Get_contextid_nonblock
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Get_contextid_nonblock(MPID_Comm *comm_ptr, MPID_Comm *newcommp, MPID_Request **req)
{
    int mpi_errno = MPI_SUCCESS;
    int tag;
    MPID_Sched_t s;

    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_GET_CONTEXTID_NONBLOCK);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_GET_CONTEXTID_NONBLOCK);

    /* now create a schedule */
    mpi_errno = MPID_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPID_Sched_create(&s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* add some entries to it */
    mpi_errno = sched_get_cid_nonblock(comm_ptr, &newcommp->context_id, &newcommp->recvcontext_id, s, MPID_INTRACOMM);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* finally, kick off the schedule and give the caller a request */
    mpi_errno = MPID_Sched_start(&s, comm_ptr, tag, req);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_GET_CONTEXTID_NONBLOCK);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIR_Get_intercomm_contextid_nonblock
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Get_intercomm_contextid_nonblock(MPID_Comm *comm_ptr, MPID_Comm *newcommp, MPID_Request **req)
{
    int mpi_errno = MPI_SUCCESS;
    int tag;
    MPID_Sched_t s;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID_NONBLOCK);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID_NONBLOCK);

    /* do as much local setup as possible */
    if (!comm_ptr->local_comm) {
        mpi_errno = MPIR_Setup_intercomm_localcomm(comm_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* now create a schedule */
    mpi_errno = MPID_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPID_Sched_create(&s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* add some entries to it */

    /* first get a context ID over the local comm */
    mpi_errno = sched_get_cid_nonblock(comm_ptr, &newcommp->recvcontext_id, &newcommp->context_id, s, MPID_INTERCOMM);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* finally, kick off the schedule and give the caller a request */
    mpi_errno = MPID_Sched_start(&s, comm_ptr, tag, req);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

fn_fail:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID_NONBLOCK);
    return mpi_errno;
}


/* Get a context for a new intercomm.  There are two approaches 
   here (for MPI-1 codes only)
   (a) Each local group gets a context; the groups exchange, and
       the low value is accepted and the high one returned.  This
       works because the context ids are taken from the same pool.
   (b) Form a temporary intracomm over all processes and use that
       with the regular algorithm.
   
   In some ways, (a) is the better approach because it is the one that
   extends to MPI-2 (where the last step, returning the context, is 
   not used and instead separate send and receive context id value 
   are kept).  For this reason, we'll use (a).

   Even better is to separate the local and remote context ids.  Then
   each group of processes can manage their context ids separately.
*/
/* 
 * This uses the thread-safe (if necessary) routine to get a context id
 * and does not need its own thread-safe version.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Get_intercomm_contextid
#undef FCNAME
#define FCNAME "MPIR_Get_intercomm_contextid"
int MPIR_Get_intercomm_contextid( MPID_Comm *comm_ptr, MPIR_Context_id_t *context_id, 
				  MPIR_Context_id_t *recvcontext_id )
{
    MPIR_Context_id_t mycontext_id, remote_context_id;
    int mpi_errno = MPI_SUCCESS;
    int tag = 31567; /* FIXME  - we need an internal tag or 
		        communication channel.  Can we use a different
		        context instead?.  Or can we use the tag 
		        provided in the intercomm routine? (not on a dup, 
			but in that case it can use the collective context) */
    int errflag = FALSE;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID);

    if (!comm_ptr->local_comm) {
        /* Manufacture the local communicator */
        mpi_errno = MPIR_Setup_intercomm_localcomm( comm_ptr );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    mpi_errno = MPIR_Get_contextid( comm_ptr->local_comm, &mycontext_id );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_Assert(mycontext_id != 0);

    /* MPIC routine uses an internal context id.  The local leads (process 0)
       exchange data */
    remote_context_id = -1;
    if (comm_ptr->rank == 0) {
        mpi_errno = MPIC_Sendrecv( &mycontext_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, tag,
                                      &remote_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, tag,
                                      comm_ptr->handle, MPI_STATUS_IGNORE, &errflag );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* Make sure that all of the local processes now have this
       id */
    mpi_errno = MPIR_Bcast_impl( &remote_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 
                                 0, comm_ptr->local_comm, &errflag );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    /* The recvcontext_id must be the one that was allocated out of the local
     * group, not the remote group.  Otherwise we could end up posting two
     * MPI_ANY_SOURCE,MPI_ANY_TAG recvs on the same context IDs even though we
     * are attempting to post them for two separate communicators. */
    *context_id     = remote_context_id;
    *recvcontext_id = mycontext_id;
 fn_fail:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Free_contextid
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
void MPIR_Free_contextid( MPIR_Context_id_t context_id )
{
    int idx, bitpos, raw_prefix;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_FREE_CONTEXTID);
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_FREE_CONTEXTID);

    /* Convert the context id to the bit position */
    raw_prefix = MPID_CONTEXT_READ_FIELD(PREFIX,context_id);
    idx    = raw_prefix / MPIR_CONTEXT_INT_BITS;
    bitpos = raw_prefix % MPIR_CONTEXT_INT_BITS;

    /* --BEGIN ERROR HANDLING-- */
    if (idx < 0 || idx >= MPIR_MAX_CONTEXT_MASK) {
	MPID_Abort( 0, MPI_ERR_INTERN, 1, 
		    "In MPIR_Free_contextid, idx is out of range" );
    }
    /* --END ERROR HANDLING-- */

    /* The low order bits for dynamic context IDs don't have meaning the
     * same way that low bits of non-dynamic ctx IDs do.  So we have to
     * check the dynamic case first. */
    if (MPID_CONTEXT_READ_FIELD(DYNAMIC_PROC, context_id)) {
        MPIU_DBG_MSG_D(COMM,VERBOSE,"skipping dynamic process ctx id, context_id=%d", context_id);
        goto fn_exit;
    }
    else { /* non-dynamic context ID */
        /* In terms of the context ID bit vector, intercomms and their constituent
         * localcomms have the same value.  To avoid a double-free situation we just
         * don't free the context ID for localcomms and assume it will be cleaned up
         * when the parent intercomm is itself completely freed. */
        if (MPID_CONTEXT_READ_FIELD(IS_LOCALCOMM, context_id)) {
#ifdef USE_DBG_LOGGING
            char dump_str[1024];
            MPIR_Comm_dump_context_id(context_id, dump_str, sizeof(dump_str));
            MPIU_DBG_MSG_S(COMM,VERBOSE,"skipping localcomm id: %s", dump_str);
#endif
            goto fn_exit;
        }
        else if (MPID_CONTEXT_READ_FIELD(SUBCOMM, context_id)) {
            MPIU_DBG_MSG_D(COMM,VERBOSE,"skipping non-parent communicator ctx id, context_id=%d", context_id);
            goto fn_exit;
        }
    }

    /* --BEGIN ERROR HANDLING-- */
    /* Check that this context id has been allocated */
    if ( (context_mask[idx] & (0x1 << bitpos)) != 0 ) {
#ifdef USE_DBG_LOGGING
        char dump_str[1024];
        MPIR_Comm_dump_context_id(context_id, dump_str, sizeof(dump_str));
        MPIU_DBG_MSG_S(COMM,VERBOSE,"context dump: %s", dump_str);
        MPIU_DBG_MSG_S(COMM,VERBOSE,"context mask = %s",MPIR_ContextMaskToStr());
#endif
	MPID_Abort( 0, MPI_ERR_INTERN, 1, 
		    "In MPIR_Free_contextid, the context id is not in use" );
    }
    /* --END ERROR HANDLING-- */

    MPIU_THREAD_CS_ENTER(CONTEXTID,);
    /* MT: Note that this update must be done atomically in the multithreaedd
       case.  In the "one, single lock" implementation, that lock is indeed
       held when this operation is called. */
    context_mask[idx] |= (0x1 << bitpos);
    MPIU_THREAD_CS_EXIT(CONTEXTID,);

    MPIU_DBG_MSG_FMT(COMM,VERBOSE,
                     (MPIU_DBG_FDEST,
                      "Freed context %d, mask[%d] bit %d (prefix=%#x)",
                      context_id, idx, bitpos, raw_prefix));
fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_FREE_CONTEXTID);
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
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_copy
#undef FCNAME
#define FCNAME "MPIR_Comm_copy"
int MPIR_Comm_copy( MPID_Comm *comm_ptr, int size, MPID_Comm **outcomm_ptr )
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Context_id_t new_context_id, new_recvcontext_id;
    MPID_Comm *newcomm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_COPY);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_COPY);

    /* Get a new context first.  We need this to be collective over the
       input communicator */
    /* If there is a context id cache in oldcomm, use it here.  Otherwise,
       use the appropriate algorithm to get a new context id.  Be careful
       of intercomms here */
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	mpi_errno = 
	    MPIR_Get_intercomm_contextid( 
		 comm_ptr, &new_context_id, &new_recvcontext_id );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
	mpi_errno = MPIR_Get_contextid( comm_ptr, &new_context_id );
	new_recvcontext_id = new_context_id;
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_Assert(new_context_id != 0);
    }
    /* --BEGIN ERROR HANDLING-- */
    if (new_context_id == 0) {
        int nfree = -1;
        int ntotal = -1;
        MPIR_ContextMaskStats(&nfree, &ntotal);
        MPIU_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER,
                             "**toomanycomm", "**toomanycomm %d %d %d",
                             nfree, ntotal, /*ignore_id=*/0);
    }
    /* --END ERROR HANDLING-- */

    /* This is the local size, not the remote size, in the case of
       an intercomm */
    if (comm_ptr->rank >= size) {
        *outcomm_ptr = 0;
        /* always free the recvcontext ID, never the "send" ID */
        MPIR_Free_contextid(new_recvcontext_id);
	goto fn_exit;
    }

    /* We're left with the processes that will have a non-null communicator.
       Create the object, initialize the data, and return the result */

    mpi_errno = MPIR_Comm_create( &newcomm_ptr );
    if (mpi_errno) goto fn_fail;

    newcomm_ptr->context_id     = new_context_id;
    newcomm_ptr->recvcontext_id = new_recvcontext_id;

    /* Save the kind of the communicator */
    newcomm_ptr->comm_kind   = comm_ptr->comm_kind;
    newcomm_ptr->local_comm  = 0;

    /* There are two cases here - size is the same as the old communicator,
       or it is smaller.  If the size is the same, we can just add a reference.
       Otherwise, we need to create a new VCRT.  Note that this is the
       test that matches the test on rank above. */
    if (size == comm_ptr->local_size) {
	/* Duplicate the VCRT references */
	MPID_VCRT_Add_ref( comm_ptr->vcrt );
	newcomm_ptr->vcrt = comm_ptr->vcrt;
	newcomm_ptr->vcr  = comm_ptr->vcr;
    }
    else {
	int i;
	/* The "remote" vcr gets the shortened vcrt */
	MPID_VCRT_Create( size, &newcomm_ptr->vcrt );
	MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, 
			   &newcomm_ptr->vcr );
	for (i=0; i<size; i++) {
	    /* For rank i in the new communicator, find the corresponding
	       rank in the input communicator */
	    MPID_VCR_Dup( comm_ptr->vcr[i], &newcomm_ptr->vcr[i] );
	}
    }

    /* If it is an intercomm, duplicate the local vcrt references */
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	MPID_VCRT_Add_ref( comm_ptr->local_vcrt );
	newcomm_ptr->local_vcrt = comm_ptr->local_vcrt;
	newcomm_ptr->local_vcr  = comm_ptr->local_vcr;
    }

    /* Set the sizes and ranks */
    newcomm_ptr->rank        = comm_ptr->rank;
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	newcomm_ptr->local_size   = comm_ptr->local_size;
	newcomm_ptr->remote_size  = comm_ptr->remote_size;
	newcomm_ptr->is_low_group = comm_ptr->is_low_group;
    }
    else {
	newcomm_ptr->local_size  = size;
	newcomm_ptr->remote_size = size;
    }

    /* Inherit the error handler (if any) */
    MPIU_THREAD_CS_ENTER(MPI_OBJ, comm_ptr);
    newcomm_ptr->errhandler = comm_ptr->errhandler;
    if (comm_ptr->errhandler) {
	MPIR_Errhandler_add_ref( comm_ptr->errhandler );
    }
    MPIU_THREAD_CS_EXIT(MPI_OBJ, comm_ptr);

    /* FIXME do we want to copy coll_fns here? */

    mpi_errno = MPIR_Comm_commit(newcomm_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* Start with no attributes on this communicator */
    newcomm_ptr->attributes = 0;

    /* Copy over the info hints from the original communicator. */
    mpi_errno = MPIR_Info_dup_impl(comm_ptr->info, &(newcomm_ptr->info));
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPIR_Comm_apply_hints(newcomm_ptr, newcomm_ptr->info);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    *outcomm_ptr = newcomm_ptr;

 fn_fail:
 fn_exit:

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_COPY);

    return mpi_errno;
}

/* Copy a communicator, including copying the virtual connection tables and
 * clearing the various fields.  Does *not* allocate a context ID or commit the
 * communicator.  Does *not* copy attributes.
 *
 * Used by comm_idup.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_copy_data
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Comm_copy_data(MPID_Comm *comm_ptr, MPID_Comm **outcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *newcomm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_COPY_DATA);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_COPY_DATA);

    mpi_errno = MPIR_Comm_create(&newcomm_ptr);
    if (mpi_errno) goto fn_fail;

    /* use a large garbage value to ensure errors are caught more easily */
    newcomm_ptr->context_id     = 32767;
    newcomm_ptr->recvcontext_id = 32767;

    /* Save the kind of the communicator */
    newcomm_ptr->comm_kind  = comm_ptr->comm_kind;
    newcomm_ptr->local_comm = 0;

    /* Duplicate the VCRT references */
    MPID_VCRT_Add_ref(comm_ptr->vcrt);
    newcomm_ptr->vcrt = comm_ptr->vcrt;
    newcomm_ptr->vcr  = comm_ptr->vcr;

    /* If it is an intercomm, duplicate the local vcrt references */
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
        MPID_VCRT_Add_ref(comm_ptr->local_vcrt);
        newcomm_ptr->local_vcrt = comm_ptr->local_vcrt;
        newcomm_ptr->local_vcr  = comm_ptr->local_vcr;
    }

    /* Set the sizes and ranks */
    newcomm_ptr->rank         = comm_ptr->rank;
    newcomm_ptr->local_size   = comm_ptr->local_size;
    newcomm_ptr->remote_size  = comm_ptr->remote_size;
    newcomm_ptr->is_low_group = comm_ptr->is_low_group; /* only relevant for intercomms */

    /* Inherit the error handler (if any) */
    MPIU_THREAD_CS_ENTER(MPI_OBJ, comm_ptr);
    newcomm_ptr->errhandler = comm_ptr->errhandler;
    if (comm_ptr->errhandler) {
        MPIR_Errhandler_add_ref(comm_ptr->errhandler);
    }
    MPIU_THREAD_CS_EXIT(MPI_OBJ, comm_ptr);

    /* FIXME do we want to copy coll_fns here? */

    mpi_errno = MPIR_Comm_commit(newcomm_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* Start with no attributes on this communicator */
    newcomm_ptr->attributes = 0;
    *outcomm_ptr = newcomm_ptr;

fn_fail:
fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_COPY_DATA);
    return mpi_errno;
}
/* Common body between MPIR_Comm_release and MPIR_comm_release_always.  This
 * helper function frees the actual MPID_Comm structure and any associated
 * storage.  It also releases any refernces to other objects, such as the VCRT.
 * This function should only be called when the communicator's reference count
 * has dropped to 0.
 *
 * !!! This routine should *never* be called outside of MPIR_Comm_release{,_always} !!!
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_delete_internal
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Comm_delete_internal(MPID_Comm * comm_ptr, int isDisconnect)
{
    int in_use;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_COMM_DELETE_INTERNAL);

    MPID_MPI_FUNC_ENTER(MPID_STATE_COMM_DELETE_INTERNAL);

    MPIU_Assert(MPIU_Object_get_ref(comm_ptr) == 0); /* sanity check */

    /* Remove the attributes, executing the attribute delete routine.
       Do this only if the attribute functions are defined.
       This must be done first, because if freeing the attributes
       returns an error, the communicator is not freed */
    if (MPIR_Process.attr_free && comm_ptr->attributes) {
        /* Temporarily add a reference to this communicator because
           the attr_free code requires a valid communicator */
        MPIU_Object_add_ref( comm_ptr );
        mpi_errno = MPIR_Process.attr_free( comm_ptr->handle,
                                            &comm_ptr->attributes );
        /* Release the temporary reference added before the call to
           attr_free */
        MPIU_Object_release_ref( comm_ptr, &in_use);
    }

    /* If the attribute delete functions return failure, the
       communicator must not be freed.  That is the reason for the
       test on mpi_errno here. */
    if (mpi_errno == MPI_SUCCESS) {
        /* If this communicator is our parent, and we're disconnecting
           from the parent, mark that fact */
        if (MPIR_Process.comm_parent == comm_ptr)
            MPIR_Process.comm_parent = NULL;

        /* Notify the device that the communicator is about to be
           destroyed */
        mpi_errno = MPID_Dev_comm_destroy_hook(comm_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        /* Free info hints */
        if (comm_ptr->info != NULL) {
            MPIU_Info_free(comm_ptr->info);
        }

        /* release our reference to the collops structure, comes after the
         * destroy_hook to allow the device to manage these vtables in a custom
         * fashion */
        if (comm_ptr->coll_fns && --comm_ptr->coll_fns->ref_count == 0) {
            MPIU_Free(comm_ptr->coll_fns);
            comm_ptr->coll_fns = NULL;
        }

        /* Free the VCRT */
        mpi_errno = MPID_VCRT_Release(comm_ptr->vcrt, isDisconnect);
        if (mpi_errno != MPI_SUCCESS) {
            MPIU_ERR_POP(mpi_errno);
        }
        if (comm_ptr->comm_kind == MPID_INTERCOMM) {
            mpi_errno = MPID_VCRT_Release(
                                          comm_ptr->local_vcrt, isDisconnect);
            if (mpi_errno != MPI_SUCCESS) {
                MPIU_ERR_POP(mpi_errno);
            }
            if (comm_ptr->local_comm)
                MPIR_Comm_release(comm_ptr->local_comm, isDisconnect );
        }

        /* Free the local and remote groups, if they exist */
        if (comm_ptr->local_group)
            MPIR_Group_release(comm_ptr->local_group);
        if (comm_ptr->remote_group)
            MPIR_Group_release(comm_ptr->remote_group);

        /* free the intra/inter-node communicators, if they exist */
        if (comm_ptr->node_comm)
            MPIR_Comm_release(comm_ptr->node_comm, isDisconnect);
        if (comm_ptr->node_roots_comm)
            MPIR_Comm_release(comm_ptr->node_roots_comm, isDisconnect);
        if (comm_ptr->intranode_table != NULL)
            MPIU_Free(comm_ptr->intranode_table);
        if (comm_ptr->internode_table != NULL)
            MPIU_Free(comm_ptr->internode_table);

        /* Free the context value.  This should come after freeing the
         * intra/inter-node communicators since those free calls won't
         * release this context ID and releasing this before then could lead
         * to races once we make threading finer grained. */
        /* This must be the recvcontext_id (i.e. not the (send)context_id)
         * because in the case of intercommunicators the send context ID is
         * allocated out of the remote group's bit vector, not ours. */
        MPIR_Free_contextid( comm_ptr->recvcontext_id );

        /* We need to release the error handler */
        /* no MPI_OBJ CS needed */
        if (comm_ptr->errhandler &&
            ! (HANDLE_GET_KIND(comm_ptr->errhandler->handle) ==
               HANDLE_KIND_BUILTIN) ) {
            int errhInuse;
            MPIR_Errhandler_release_ref( comm_ptr->errhandler,&errhInuse);
            if (!errhInuse) {
                MPIU_Handle_obj_free( &MPID_Errhandler_mem,
                                      comm_ptr->errhandler );
            }
        }

        /* Remove from the list of active communicators if
           we are supporting message-queue debugging.  We make this
           conditional on having debugger support since the
           operation is not constant-time */
        MPIR_COMML_FORGET( comm_ptr );

        /* Check for predefined communicators - these should not
           be freed */
        if (! (HANDLE_GET_KIND(comm_ptr->handle) == HANDLE_KIND_BUILTIN) )
            MPIU_Handle_obj_free( &MPID_Comm_mem, comm_ptr );
    }
    else {
        /* If the user attribute free function returns an error,
           then do not free the communicator */
        MPIR_Comm_add_ref( comm_ptr );
    }

 fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_COMM_DELETE_INTERNAL);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Release a reference to a communicator.  If there are no pending
   references, delete the communicator and recover all storage and
   context ids.  This version of the function always manipulates the reference
   counts, even for predefined objects. */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_release_always
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Comm_release_always(MPID_Comm *comm_ptr, int isDisconnect)
{
    int mpi_errno = MPI_SUCCESS;
    int in_use;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_RELEASE_ALWAYS);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_RELEASE_ALWAYS);

    /* we want to short-circuit any optimization that avoids reference counting
     * predefined communicators, such as MPI_COMM_WORLD or MPI_COMM_SELF. */
    MPIU_Object_release_ref_always(comm_ptr, &in_use);
    if (!in_use) {
        mpi_errno = MPIR_Comm_delete_internal(comm_ptr, isDisconnect);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_RELEASE_ALWAYS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Apply all known info hints in the specified info chain to the given
 * communicator. */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_apply_hints
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Comm_apply_hints(MPID_Comm *comm_ptr, MPID_Info *info_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Info *hint = NULL;
    char hint_name[MPI_MAX_INFO_KEY] = { 0 };
    struct MPIR_Comm_hint_fn_elt *hint_fn = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_APPLY_HINTS);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_APPLY_HINTS);

    MPL_LL_FOREACH(info_ptr, hint) {
        /* Have we hit the default, empty info hint? */
        if (hint->key == NULL) continue;

        strncpy(hint_name, hint->key, MPI_MAX_INFO_KEY);

        HASH_FIND_STR(MPID_hint_fns, hint_name, hint_fn);

        /* Skip hints that MPICH doesn't recognize. */
        if (hint_fn) {
            mpi_errno = hint_fn->fn(comm_ptr, hint, hint_fn->state);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }

 fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_APPLY_HINTS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_free_hint_handles
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIR_Comm_free_hint_handles(void *ignore)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIR_Comm_hint_fn_elt *curr_hint = NULL, *tmp = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_FREE_HINT_HANDLES);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_FREE_HINT_HANDLES);

    if (MPID_hint_fns) {
        HASH_ITER(hh, MPID_hint_fns, curr_hint, tmp) {
            HASH_DEL(MPID_hint_fns, curr_hint);
            MPIU_Free(curr_hint);
        }
    }

 fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_FREE_HINT_HANDLES);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* The hint logic is stored in a uthash, with hint name as key and
 * the function responsible for applying the hint as the value. */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_register_hint
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Comm_register_hint(const char *hint_key, MPIR_Comm_hint_fn_t fn, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIR_Comm_hint_fn_elt *hint_elt = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_REGISTER_HINT);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_REGISTER_HINT);

    if (MPID_hint_fns == NULL) {
        MPIR_Add_finalize(MPIR_Comm_free_hint_handles, NULL, MPIR_FINALIZE_CALLBACK_PRIO - 1);
    }

    hint_elt = MPIU_Malloc(sizeof(struct MPIR_Comm_hint_fn_elt));
    strncpy(hint_elt->name, hint_key, MPI_MAX_INFO_KEY);
    hint_elt->state = state;
    hint_elt->fn = fn;

    HASH_ADD_STR(MPID_hint_fns, name, hint_elt);

 fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_REGISTER_HINT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
