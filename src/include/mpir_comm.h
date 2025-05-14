/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_COMM_H_INCLUDED
#define MPIR_COMM_H_INCLUDED

#if defined HAVE_HCOLL
#include "../mpid/common/hcoll/hcollpre.h"
#endif

/* Maximum radix up to which the nbrs from
 * recursive exchange algorithm will be stored in the communicator */
#define MAX_RADIX 8
/*E
  MPIR_Comm_kind_t - Name the two types of communicators
  E*/
typedef enum MPIR_Comm_kind_t {
    MPIR_COMM_KIND__INTRACOMM = 0,
    MPIR_COMM_KIND__INTERCOMM = 1
} MPIR_Comm_kind_t;

/* Communicator info hint */
#define MPIR_COMM_HINT_TYPE_BOOL 0
#define MPIR_COMM_HINT_TYPE_INT  1

/* Communicator attr (bitmask)
 * If local bit is set, the hint is local. Default (0) will require the hint value be
 * the same across communicator.
 */
#define MPIR_COMM_HINT_ATTR_LOCAL 0x1

#define MPIR_COMM_HINT_MAX 100

enum MPIR_COMM_HINT_PREDEFINED_t {
    MPIR_COMM_HINT_INVALID = 0,
    MPIR_COMM_HINT_NO_ANY_TAG,
    MPIR_COMM_HINT_NO_ANY_SOURCE,
    MPIR_COMM_HINT_EXACT_LENGTH,
    MPIR_COMM_HINT_ALLOW_OVERTAKING,
    MPIR_COMM_HINT_STRICT_PCOLL_ORDERING,
    /* device specific hints.
     * Potentially, we can use macros and configure to hide them */
    MPIR_COMM_HINT_EAGER_THRESH,        /* ch3 */
    MPIR_COMM_HINT_ENABLE_MULTI_NIC_STRIPING,   /* ch4:ofi */
    MPIR_COMM_HINT_ENABLE_MULTI_NIC_HASHING,    /* ch4:ofi */
    MPIR_COMM_HINT_MULTI_NIC_PREF_NIC,  /* ch4:ofi */
    MPIR_COMM_HINT_SENDER_VCI,  /* ch4 */
    MPIR_COMM_HINT_RECEIVER_VCI,        /* ch4 */
    MPIR_COMM_HINT_VCI, /* ch4. Generic hint, useful to select vci_idx, progress thread etc. */
    /* dynamic hints starts here */
    MPIR_COMM_HINT_PREDEFINED_COUNT
};

/*S
  MPIR_Comm - Description of the Communicator data structure

  Notes:
  Note that the size and rank duplicate data in the groups that
  make up this communicator.  These are used often enough that this
  optimization is valuable.

  This definition provides only a 16-bit integer for context id''s .
  This should be sufficient for most applications.  However, extending
  this to a 32-bit (or longer) integer should be easy.

  There are two context ids.  One is used for sending and one for
  receiving.  In the case of an Intracommunicator, they are the same
  context id.  They differ in the case of intercommunicators, where
  they may come from processes in different comm worlds (in the
  case of MPI-2 dynamic process intercomms). With intercomms, we are
  sending with context_id, which is the same as peer's recvcontext_id.

  The virtual connection table is an explicit member of this structure.
  This contains the information used to contact a particular process,
  indexed by the rank relative to this communicator.

  Groups are allocated lazily.  That is, the group pointers may be
  null, created only when needed by a routine such as 'MPI_Comm_group'.
  The local process ids needed to form the group are available within
  the virtual connection table.
  For intercommunicators, we may want to always have the groups.  If not,
  we either need the 'local_group' or we need a virtual connection table
  corresponding to the 'local_group' (we may want this anyway to simplify
  the implementation of the intercommunicator collective routines).

  The pointer to the structure 'MPIR_Collops' containing pointers to the
  collective
  routines allows an implementation to replace each routine on a
  routine-by-routine basis.  By default, this pointer is null, as are the
  pointers within the structure.  If either pointer is null, the implementation
  uses the generic provided implementation.  This choice, rather than
  initializing the table with pointers to all of the collective routines,
  is made to reduce the space used in the communicators and to eliminate the
  need to include the implementation of all collective routines in all MPI
  executables, even if the routines are not used.

  Please note that the local_size and remote_size fields can be confusing.  For
  intracommunicators both fields are always equal to the size of the
  communicator.  For intercommunicators local_size is equal to the size of
  local_group while remote_size is equal to the size of remote_group.

  Module:
  Communicator-DS

  Question:
  For fault tolerance, do we want to have a standard field for communicator
  health?  For example, ok, failure detected, all (live) members of failed
  communicator have acked.
  S*/
struct MPIR_Comm {
    MPIR_OBJECT_HEADER;         /* adds handle and ref_count fields */

    /* -- core fields, required by subcomm as well -- */
    int attr;                   /* if attr is 0, only the core set of fields are set.
                                 * Other fields are set in Comm_commit along with corresponding attr bits */
    int context_id;             /* Send context id.  See notes */
    int recvcontext_id;         /* Recv context id (locally allocated).  See notes */
    int rank;                   /* Value of MPI_Comm_rank */
    int local_size;             /* Value of MPI_Comm_size for local group */
    MPIR_Group *local_group;    /* Groups in communicator. */
    MPIR_Comm_kind_t comm_kind; /* MPIR_COMM_KIND__INTRACOMM or MPIR_COMM_KIND__INTERCOMM */
    MPID_Thread_mutex_t mutex;
    struct MPIR_CCLcomm *cclcomm;       /* Not NULL only if CCL subcommunication is enabled */

    /* -- unset unless (attr | MPIR_COMM_ATTR__HIERARCHY) -- */
    int hierarchy_flags;        /* bit flags for hierarchy charateristics. See bit definitions below. */
    int local_rank;
    int num_local;
    int external_rank;
    int num_external;
    struct MPIR_Comm *node_comm;        /* Comm of processes in this comm that are on
                                         * the same node as this process. */
    struct MPIR_Comm *node_roots_comm;  /* Comm of root processes for other nodes. */
    int *intranode_table;       /* intranode_table[i] gives the rank in
                                 * node_comm of rank i in this comm or -1 if i
                                 * is not in this process' node_comm.
                                 * It is of size 'local_size'. */
    int *internode_table;       /* internode_table[i] gives the rank in
                                 * node_roots_comm of rank i in this comm.
                                 * It is of size 'local_size'. */

    int is_low_group;           /* For intercomms only, this boolean is
                                 * M* set for all members of one of the
                                 * * two groups of processes and clear for
                                 * * the other.  It enables certain
                                 * * intercommunicator collective operations
                                 * * that wish to use half-duplex operations
                                 * * to implement a full-duplex operation */
    int remote_size;            /* Value of MPI_Comm_(remote)_size */
    MPIR_Group *remote_group;   /* The remote group in a inter communicator.
                                 * Must be NULL in a intra communicator. */
    struct MPIR_Comm *local_comm;       /* Defined only for intercomms, holds
                                         * an intracomm for the local group */

    /* -- user-level comm required -- */
    char name[MPI_MAX_OBJECT_NAME];     /* Required for MPI-2 */
    MPIR_Session *session_ptr;  /* Pointer to MPI session to which the communicator belongs */
    MPIR_Errhandler *errhandler;        /* Pointer to the error handler structure */
    MPIR_Attribute *attributes; /* List of attributes */
    struct MPIR_Comm *comm_next;        /* Provides a chain through all active
                                         * communicators */
    struct MPII_Topo_ops *topo_fns;     /* Pointer to a table of functions
                                         * implementting the topology routines */
    struct MPII_BsendBuffer *bsendbuffer;       /* for MPI_Comm_attach_buffer */

    int next_sched_tag;         /* used by the NBC schedule code to allocate tags */
    int next_am_tag;            /* for ch4 am_tag_send and am_tag_recv */

    int revoked;                /* Flag to track whether the communicator
                                 * has been revoked */

    /* -- extended features -- */
    struct MPIR_Threadcomm *threadcomm; /* Not NULL only if it's associated with a threadcomm */

    enum { MPIR_STREAM_COMM_NONE, MPIR_STREAM_COMM_SINGLE, MPIR_STREAM_COMM_MULTIPLEX }
        stream_comm_type;
    union {
        struct {
            struct MPIR_Stream *stream;
            int *vci_table;
        } single;
        struct {
            struct MPIR_Stream **local_streams;
            MPI_Aint *vci_displs;       /* comm size + 1 */
            int *vci_table;     /* comm size */
        } multiplex;
    } stream_comm;

    /* -- optimization fields -- */
    /* A sequence number used for e.g. vci hashing. We can't directly use context_id
     * because context_id is non-sequential and can't be used to identify user-level
     * communicators (due to sub-comms). */
    int seq;
    /* Whether multiple-vci is enabled. This is ONLY inherited in Comm_dup and Comm_split */
    bool vcis_enabled;


    int hints[MPIR_COMM_HINT_MAX];      /* Hints to the communicator
                                         * use int array for fast access */

    struct {
        int pofk[MAX_RADIX - 1];
        int k[MAX_RADIX - 1];
        int step1_sendto[MAX_RADIX - 1];
        int step1_nrecvs[MAX_RADIX - 1];
        int *step1_recvfrom[MAX_RADIX - 1];
        int step2_nphases[MAX_RADIX - 1];
        int **step2_nbrs[MAX_RADIX - 1];
        int nbrs_defined[MAX_RADIX - 1];
        void **recexch_allreduce_nbr_buffer;
        int topo_aware_tree_root;
        int topo_aware_tree_k;
        MPIR_Treealgo_tree_t *topo_aware_tree;
        int topo_aware_k_tree_root;
        int topo_aware_k_tree_k;
        MPIR_Treealgo_tree_t *topo_aware_k_tree;
        int topo_wave_tree_root;
        int topo_wave_tree_overhead;
        int topo_wave_tree_lat_diff_groups;
        int topo_wave_tree_lat_diff_switches;
        int topo_wave_tree_lat_same_switches;
        MPIR_Treealgo_tree_t *topo_wave_tree;
    } coll;

    void *csel_comm;            /* collective selector handle */

#if defined HAVE_HCOLL
    hcoll_comm_priv_t hcoll_priv;
#endif                          /* HAVE_HCOLL */

    MPIR_Request *persistent_requests;

    /* Other, device-specific information */
#ifdef MPID_DEV_COMM_DECL
     MPID_DEV_COMM_DECL
#endif
};

/* Bit flags for comm->attr */
#define MPIR_COMM_ATTR__SUBCOMM   0x1
#define MPIR_COMM_ATTR__HIERARCHY 0x2

#define MPIR_COMM_HIERARCHY__NO_LOCAL    0x1
#define MPIR_COMM_HIERARCHY__SINGLE_NODE 0x2
#define MPIR_COMM_HIERARCHY__NODE_CONSECUTIVE 0x4       /* ranks are ordered by nodes */
#define MPIR_COMM_HIERARCHY__NODE_BALANCED    0x8       /* same number of ranks on every node */
#define MPIR_COMM_HIERARCHY__PARENT      0x10   /* has node_comm and node_roots_comm */

#define MPIR_is_self_comm(comm) \
    ((comm)->remote_size == 1 && (comm)->comm_kind == MPIR_COMM_KIND__INTRACOMM && \
     (!(comm)->threadcomm || (comm)->threadcomm->num_threads == 1))

extern MPIR_Object_alloc_t MPIR_Comm_mem;

/* this function should not be called by normal code! */
int MPIR_Comm_delete_internal(MPIR_Comm * comm_ptr);
void MPIR_stream_comm_init(MPIR_Comm * comm_ptr);
void MPIR_stream_comm_free(MPIR_Comm * comm_ptr);
int MPIR_Comm_copy_stream(MPIR_Comm * oldcomm, MPIR_Comm * newcomm);
int MPIR_get_local_gpu_stream(MPIR_Comm * comm_ptr, MPL_gpu_stream_t * gpu_stream);

MPL_STATIC_INLINE_PREFIX MPIR_Lpid MPIR_comm_rank_to_lpid(MPIR_Comm * comm_ptr, int rank)
{
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        return MPIR_Group_rank_to_lpid(comm_ptr->local_group, rank);
    } else {
        return MPIR_Group_rank_to_lpid(comm_ptr->remote_group, rank);
    }
}

MPL_STATIC_INLINE_PREFIX MPIR_Stream *MPIR_stream_comm_get_local_stream(MPIR_Comm * comm_ptr)
{
    if (comm_ptr->stream_comm_type == MPIR_STREAM_COMM_SINGLE) {
        return comm_ptr->stream_comm.single.stream;
    } else if (comm_ptr->stream_comm_type == MPIR_STREAM_COMM_MULTIPLEX) {
        return comm_ptr->stream_comm.multiplex.local_streams[comm_ptr->rank];
    } else {
        return NULL;
    }
}

/* We never skip reference counting for built-in comms */
#define MPIR_Comm_add_ref(comm_p_) \
    do { MPIR_Object_add_ref_always((comm_p_)); } while (0)
#define MPIR_Comm_release_ref(comm_p_, inuse_) \
    do { MPIR_Object_release_ref_always(comm_p_, inuse_); } while (0)


/* Release a reference to a communicator.  If there are no pending
   references, delete the communicator and recover all storage and
   context ids.

   This routine has been inlined because keeping it as a separate routine
   results in a >5% performance hit for the SQMR benchmark.
*/
static inline int MPIR_Comm_release(MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int in_use;

    MPIR_Comm_release_ref(comm_ptr, &in_use);
    if (unlikely(!in_use)) {
        /* the following routine should only be called by this function and its
         * "_always" variant. */
        mpi_errno = MPIR_Comm_delete_internal(comm_ptr);
        /* not ERR_POPing here to permit simpler inlining.  Our caller will
         * still report the error from the comm_delete level. */
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIR_Stream_comm_set_attr(MPIR_Comm * comm, int src_rank, int dst_rank,
                                                       int src_index, int dst_index, int *attr_out)
{
    int mpi_errno = MPI_SUCCESS;

    *attr_out = 0;

    MPIR_ERR_CHKANDJUMP(comm->stream_comm_type != MPIR_STREAM_COMM_MULTIPLEX,
                        mpi_errno, MPI_ERR_OTHER, "**streamcomm_notmult");

    MPI_Aint *displs = comm->stream_comm.multiplex.vci_displs;

    MPIR_ERR_CHKANDJUMP(displs[src_rank] + src_index >= displs[src_rank + 1],
                        mpi_errno, MPI_ERR_OTHER, "**streamcomm_srcidx");
    MPIR_ERR_CHKANDJUMP(displs[dst_rank] + dst_index >= displs[dst_rank + 1],
                        mpi_errno, MPI_ERR_OTHER, "**streamcomm_dstidx");

    int src_vci = comm->stream_comm.multiplex.vci_table[displs[src_rank] + src_index];
    int dst_vci = comm->stream_comm.multiplex.vci_table[displs[src_rank] + dst_index];

    MPIR_PT2PT_ATTR_SET_VCIS(*attr_out, src_vci, dst_vci);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIR_Get_internode_rank(MPIR_Comm * comm, int r)
{
    MPIR_Assert(comm->attr | MPIR_COMM_ATTR__HIERARCHY);
    if (comm->internode_table) {
        return comm->internode_table[r];
    } else {
        /* canonical or trivial */
        return r / comm->num_local;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIR_Get_intranode_rank(MPIR_Comm * comm, int r)
{
    MPIR_Assert(comm->attr | MPIR_COMM_ATTR__HIERARCHY);
    if (comm->intranode_table) {
        return comm->intranode_table[r];
    } else {
        /* canonical or trivial */
        if ((comm->rank / comm->num_local) == (r / comm->num_local)) {
            return r % comm->num_local;
        } else {
            return -1;
        }
    }
}

MPL_STATIC_INLINE_PREFIX bool MPII_Comm_is_node_consecutive(MPIR_Comm * comm)
{
    return (comm->attr & MPIR_COMM_ATTR__HIERARCHY) &&
        (comm->hierarchy_flags & MPIR_COMM_HIERARCHY__NODE_CONSECUTIVE);
}

MPL_STATIC_INLINE_PREFIX bool MPII_Comm_is_node_balanced(MPIR_Comm * comm)
{
    return (comm->attr & MPIR_COMM_ATTR__HIERARCHY) &&
        (comm->hierarchy_flags & MPIR_COMM_HIERARCHY__NODE_BALANCED);
}

/* node_canonical means node_balanced and node_consecutive */
MPL_STATIC_INLINE_PREFIX bool MPII_Comm_is_node_canonical(MPIR_Comm * comm)
{
    return (comm->attr & MPIR_COMM_ATTR__HIERARCHY) &&
        (comm->hierarchy_flags & MPIR_COMM_HIERARCHY__NODE_BALANCED) &&
        (comm->hierarchy_flags & MPIR_COMM_HIERARCHY__NODE_CONSECUTIVE);
}

MPL_STATIC_INLINE_PREFIX bool MPIR_Comm_is_parent_comm(MPIR_Comm * comm)
{
    return (comm->attr & MPIR_COMM_ATTR__HIERARCHY) &&
        (comm->hierarchy_flags & MPIR_COMM_HIERARCHY__PARENT);
}

int MPIR_Comm_create(MPIR_Comm **);
int MPIR_Comm_create_intra(MPIR_Comm * comm_ptr, MPIR_Group * group_ptr, MPIR_Comm ** newcomm_ptr);
int MPIR_Comm_create_inter(MPIR_Comm * comm_ptr, MPIR_Group * group_ptr, MPIR_Comm ** newcomm_ptr);


int MPIR_Subcomm_create(MPIR_Comm * comm, int sub_rank, int sub_size, int *procs,
                        int context_offset, MPIR_Comm ** subcomm_out);
int MPIR_Subcomm_free(MPIR_Comm * subcomm);
int MPIR_Comm_create_subcomms(MPIR_Comm * comm);
int MPIR_Comm_commit(MPIR_Comm *);
/* we may not always construct comm->node_comm or comm->node_roots_comm. Use
 * following routines if needed. */
MPIR_Comm *MPIR_Comm_get_node_comm(MPIR_Comm * comm);
MPIR_Comm *MPIR_Comm_get_node_roots_comm(MPIR_Comm * comm);

#ifdef ENABLE_THREADCOMM
#define MPIR_COMM_RANK_SIZE(comm, rank_, size_) MPIR_THREADCOMM_RANK_SIZE(comm, rank_, size_)
#else
#define MPIR_COMM_RANK_SIZE(comm, rank_, size_) do {            \
        MPIR_Assert((comm)->threadcomm == NULL);                \
        rank_ = (comm)->rank;                                   \
        size_ = (comm)->local_size;                             \
    } while (0)
#endif

#define MPIR_Comm_rank(comm_ptr) ((comm_ptr)->rank)
#define MPIR_Comm_size(comm_ptr) ((comm_ptr)->local_size)

int MPIR_Comm_save_inactive_request(MPIR_Comm * comm, MPIR_Request * request);
int MPIR_Comm_delete_inactive_request(MPIR_Comm * comm, MPIR_Request * request);
int MPIR_Comm_free_inactive_requests(MPIR_Comm * comm);

/* Comm hint registration.
 *
 * Hint function is optional. If it is NULL, MPIR_layer will set corresponding
 * hints array directly. If it is supplied, MPIR_layer will *NOT* set hints array.
 * The hint function is responsible for setting it, as well as validating it and
 * update whatever side-effects.
 *
 * Current supported type is boolean and int and the value parsed accordingly.
 *
 * If the attr is 0, it is requires the hint value to be consistent across the
 * communicator. If the LOCAL bit is set, the hint values is treated as local.
 * Additional attributes may be added in the future.
 */
void MPIR_Comm_hint_init(void);
typedef int (*MPIR_Comm_hint_fn_t) (MPIR_Comm *, int, int);     /* comm, key, val */
int MPIR_Comm_register_hint(int index, const char *hint_key, MPIR_Comm_hint_fn_t fn,
                            int type, int attr, int default_val);

int MPIR_Comm_accept_impl(const char *port_name, MPIR_Info * info_ptr, int root,
                          MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm_ptr);
int MPIR_Comm_connect_impl(const char *port_name, MPIR_Info * info_ptr, int root,
                           MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm_ptr);

int MPIR_Comm_split_type_self(MPIR_Comm * comm_ptr, int key, MPIR_Comm ** newcomm_ptr);
int MPIR_Comm_split_type_node_topo(MPIR_Comm * comm_ptr, int key,
                                   MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr);
int MPIR_Comm_split_type(MPIR_Comm * comm_ptr, int split_type, int key, MPIR_Info * info_ptr,
                         MPIR_Comm ** newcomm_ptr);

int MPIR_Comm_split_type_neighborhood(MPIR_Comm * comm_ptr, int split_type, int key,
                                      MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr);

int MPIR_Intercomm_create_timeout(MPIR_Comm * local_comm_ptr, int local_leader,
                                  MPIR_Comm * peer_comm_ptr, int remote_leader,
                                  int tag, int timeout, MPIR_Comm ** new_intercomm_ptr);

/* Preallocated comm objects.  There are 3: comm_world, comm_self, and
   a private (non-user accessible) dup of comm world that is provided
   if needed in MPI_Finalize.  Having a separate version of comm_world
   avoids possible interference with User code */
extern MPIR_Comm MPIR_Comm_builtin[MPIR_COMM_N_BUILTIN];
extern MPIR_Comm MPIR_Comm_direct[];
/* This is the handle for the internal MPI_COMM_WORLD .  The "2" at the end
   of the handle is 3-1 (e.g., the index in the builtin array) */
#define MPIR_ICOMM_WORLD  ((MPI_Comm)0x44000002)

typedef struct MPIR_Commops {
    int (*split_type) (MPIR_Comm *, int, int, MPIR_Info *, MPIR_Comm **);
} MPIR_Commops;
extern struct MPIR_Commops *MPIR_Comm_fns;      /* Communicator creation functions */


/* internal functions */

int MPII_Comm_init(MPIR_Comm *);

int MPII_Comm_dup(MPIR_Comm * comm_ptr, MPIR_Info * info, MPIR_Comm ** newcomm_ptr);
int MPII_Comm_copy(MPIR_Comm * comm_ptr, int size, MPIR_Info * info, MPIR_Comm ** outcomm_ptr);
int MPII_Comm_copy_data(MPIR_Comm * comm_ptr, MPIR_Info * info, MPIR_Comm ** outcomm_ptr);

int MPII_Setup_intercomm_localcomm(MPIR_Comm *);

int MPII_Comm_set_hints(MPIR_Comm * comm_ptr, MPIR_Info * info, bool in_comm_create);
int MPII_Comm_get_hints(MPIR_Comm * comm_ptr, MPIR_Info * info);
int MPII_Comm_check_hints(MPIR_Comm * comm_ptr);

int MPIR_init_comm_self(void);
int MPIR_init_comm_world(void);
int MPIR_finalize_builtin_comms(void);

void MPIR_Comm_set_session_ptr(MPIR_Comm * comm_ptr, MPIR_Session * session_ptr);
#endif /* MPIR_COMM_H_INCLUDED */
