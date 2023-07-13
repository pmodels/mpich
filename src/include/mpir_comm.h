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

/* ideally we could add these to MPIR_Comm_kind_t, but there's too much existing
 * code that assumes that the only valid values are INTRACOMM or INTERCOMM */
typedef enum MPIR_Comm_hierarchy_kind_t {
    MPIR_COMM_HIERARCHY_KIND__FLAT = 0, /* no hierarchy */
    MPIR_COMM_HIERARCHY_KIND__PARENT = 1,       /* has subcommunicators */
    MPIR_COMM_HIERARCHY_KIND__NODE_ROOTS = 2,   /* is the subcomm for node roots */
    MPIR_COMM_HIERARCHY_KIND__NODE = 3, /* is the subcomm for a node */
    MPIR_COMM_HIERARCHY_KIND__MULTI_LEADS = 4,  /* is the multi_leaders_comm for a node */
} MPIR_Comm_hierarchy_kind_t;

typedef enum {
    MPIR_COMM_MAP_TYPE__DUP,
    MPIR_COMM_MAP_TYPE__IRREGULAR
} MPIR_Comm_map_type_t;

/* direction of mapping: local to local, local to remote, remote to
 * local, remote to remote */
typedef enum {
    MPIR_COMM_MAP_DIR__L2L,
    MPIR_COMM_MAP_DIR__L2R,
    MPIR_COMM_MAP_DIR__R2L,
    MPIR_COMM_MAP_DIR__R2R
} MPIR_Comm_map_dir_t;

typedef struct MPIR_Comm_map {
    MPIR_Comm_map_type_t type;

    struct MPIR_Comm *src_comm;

    /* mapping direction for intercomms, which contain local and
     * remote groups */
    MPIR_Comm_map_dir_t dir;

    /* only valid for irregular map type */
    int src_mapping_size;
    int *src_mapping;
    int free_mapping;           /* we allocated the mapping */

    struct MPIR_Comm_map *next;
} MPIR_Comm_map_t;

int MPIR_Comm_map_irregular(struct MPIR_Comm *newcomm, struct MPIR_Comm *src_comm,
                            int *src_mapping, int src_mapping_size,
                            MPIR_Comm_map_dir_t dir, MPIR_Comm_map_t ** map);
int MPIR_Comm_map_dup(struct MPIR_Comm *newcomm, struct MPIR_Comm *src_comm,
                      MPIR_Comm_map_dir_t dir);
int MPIR_Comm_map_free(struct MPIR_Comm *comm);

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
    MPID_Thread_mutex_t mutex;
    MPIR_Context_id_t context_id;       /* Send context id.  See notes */
    MPIR_Context_id_t recvcontext_id;   /* Recv context id (locally allocated).  See notes */
    int remote_size;            /* Value of MPI_Comm_(remote)_size */
    int rank;                   /* Value of MPI_Comm_rank */
    MPIR_Attribute *attributes; /* List of attributes */
    int local_size;             /* Value of MPI_Comm_size for local group */
    MPIR_Group *local_group,    /* Groups in communicator. */
    *remote_group;              /* The local and remote groups are the
                                 * same for intra communicators */
    MPIR_Comm_kind_t comm_kind; /* MPIR_COMM_KIND__INTRACOMM or MPIR_COMM_KIND__INTERCOMM */
    char name[MPI_MAX_OBJECT_NAME];     /* Required for MPI-2 */
    MPIR_Errhandler *errhandler;        /* Pointer to the error handler structure */
    struct MPIR_Comm *local_comm;       /* Defined only for intercomms, holds
                                         * an intracomm for the local group */
    struct MPIR_Threadcomm *threadcomm; /* Not NULL only if it's associated with a threadcomm */

    MPIR_Comm_hierarchy_kind_t hierarchy_kind;  /* flat, parent, node, or node_roots */
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
    int node_count;             /* number of nodes this comm is spread over */

    int is_low_group;           /* For intercomms only, this boolean is
                                 * set for all members of one of the
                                 * two groups of processes and clear for
                                 * the other.  It enables certain
                                 * intercommunicator collective operations
                                 * that wish to use half-duplex operations
                                 * to implement a full-duplex operation */

    struct MPIR_Comm *comm_next;        /* Provides a chain through all active
                                         * communicators */
    struct MPII_Topo_ops *topo_fns;     /* Pointer to a table of functions
                                         * implementting the topology routines */
    int next_sched_tag;         /* used by the NBC schedule code to allocate tags */

    int revoked;                /* Flag to track whether the communicator
                                 * has been revoked */
    /* A sequence number used for e.g. vci hashing. We can't directly use context_id
     * because context_id is non-sequential and can't be used to identify user-level
     * communicators (due to sub-comms). */
    int seq;
    /* Certain comm and its offsprings should be restricted to sequence 0 due to
     * various restrictions. E.g. multiple-vci doesn't support dynamic process,
     * nor intercomms (even after its merge).
     */
    int tainted;


    int hints[MPIR_COMM_HINT_MAX];      /* Hints to the communicator
                                         * use int array for fast access */

    struct {
        int pof2;               /* Nearest (smaller than or equal to) power of 2
                                 * to the number of ranks in the communicator.
                                 * To be used during collective communication */
        int pofk[MAX_RADIX - 1];
        int k[MAX_RADIX - 1];
        int step1_sendto[MAX_RADIX - 1];
        int step1_nrecvs[MAX_RADIX - 1];
        int *step1_recvfrom[MAX_RADIX - 1];
        int step2_nphases[MAX_RADIX - 1];
        int **step2_nbrs[MAX_RADIX - 1];
        int nbrs_defined[MAX_RADIX - 1];
        void **recexch_allreduce_nbr_buffer;
    } coll;

    void *csel_comm;            /* collective selector handle */
#if defined HAVE_HCOLL
    hcoll_comm_priv_t hcoll_priv;
#endif                          /* HAVE_HCOLL */

    /* the mapper is temporarily filled out in order to allow the
     * device to setup its network addresses.  it will be freed after
     * the device has initialized the comm. */
    MPIR_Comm_map_t *mapper_head;
    MPIR_Comm_map_t *mapper_tail;

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

    /* Other, device-specific information */
#ifdef MPID_DEV_COMM_DECL
     MPID_DEV_COMM_DECL
#endif
};

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


int MPIR_Comm_create(MPIR_Comm **);
int MPIR_Comm_create_intra(MPIR_Comm * comm_ptr, MPIR_Group * group_ptr, MPIR_Comm ** newcomm_ptr);
int MPIR_Comm_create_inter(MPIR_Comm * comm_ptr, MPIR_Group * group_ptr, MPIR_Comm ** newcomm_ptr);


int MPIR_Comm_create_subcomms(MPIR_Comm * comm);
int MPIR_Comm_commit(MPIR_Comm *);

int MPIR_Comm_is_parent_comm(MPIR_Comm *);

/* peer intercomm is an internal 1-to-1 intercomm used for connecting dynamic processes */
int MPIR_peer_intercomm_create(MPIR_Context_id_t context_id, MPIR_Context_id_t recvcontext_id,
                               uint64_t remote_lpid, int is_low_group, MPIR_Comm ** newcomm);

#if defined(HAVE_ROMIO)
int MPIR_Comm_split_filesystem(MPI_Comm comm, int key, const char *dirname, MPI_Comm * newcomm);
#endif

#define MPIR_Comm_rank(comm_ptr) ((comm_ptr)->rank)
#define MPIR_Comm_size(comm_ptr) ((comm_ptr)->local_size)

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
int MPIR_Comm_split_type_hw_guided(MPIR_Comm * comm_ptr, int key, MPIR_Info * info_ptr,
                                   MPIR_Comm ** newcomm_ptr);
int MPIR_Comm_split_type_hw_unguided(MPIR_Comm * comm_ptr, int key, MPIR_Info * info_ptr,
                                     MPIR_Comm ** newcomm_ptr);
int MPIR_Comm_split_type_by_node(MPIR_Comm * comm_ptr, int key, MPIR_Comm ** newcomm_ptr);
int MPIR_Comm_split_type_node_topo(MPIR_Comm * comm_ptr, int key,
                                   MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr);
int MPIR_Comm_split_type(MPIR_Comm * comm_ptr, int split_type, int key, MPIR_Info * info_ptr,
                         MPIR_Comm ** newcomm_ptr);

int MPIR_Comm_split_type_neighborhood(MPIR_Comm * comm_ptr, int split_type, int key,
                                      MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr);
int MPIR_Comm_split_type_nbhd_common_dir(MPIR_Comm * user_comm_ptr, int key, const char *hintval,
                                         MPIR_Comm ** newcomm_ptr);
int MPIR_Comm_split_type_network_topo(MPIR_Comm * user_comm_ptr, int key, const char *hintval,
                                      MPIR_Comm ** newcomm_ptr);

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

int MPII_Comm_is_node_consecutive(MPIR_Comm *);
int MPII_Comm_is_node_balanced(MPIR_Comm *, int *, bool *);

int MPII_Comm_dup(MPIR_Comm * comm_ptr, MPIR_Info * info, MPIR_Comm ** newcomm_ptr);
int MPII_Comm_copy(MPIR_Comm * comm_ptr, int size, MPIR_Info * info, MPIR_Comm ** outcomm_ptr);
int MPII_Comm_copy_data(MPIR_Comm * comm_ptr, MPIR_Info * info, MPIR_Comm ** outcomm_ptr);

int MPII_Setup_intercomm_localcomm(MPIR_Comm *);

/* comm_create helper functions, used by both comm_create and comm_create_group */
int MPII_Comm_create_calculate_mapping(MPIR_Group * group_ptr,
                                       MPIR_Comm * comm_ptr,
                                       int **mapping_out, MPIR_Comm ** mapping_comm);

int MPII_Comm_create_map(int local_n,
                         int remote_n,
                         int *local_mapping,
                         int *remote_mapping, MPIR_Comm * mapping_comm, MPIR_Comm * newcomm);

int MPII_Comm_set_hints(MPIR_Comm * comm_ptr, MPIR_Info * info, bool in_comm_create);
int MPII_Comm_get_hints(MPIR_Comm * comm_ptr, MPIR_Info * info);
int MPII_Comm_check_hints(MPIR_Comm * comm_ptr);

int MPIR_init_comm_self(void);
int MPIR_init_comm_world(void);
#ifdef MPID_NEEDS_ICOMM_WORLD
int MPIR_init_icomm_world(void);
#endif
int MPIR_finalize_builtin_comms(void);
#endif /* MPIR_COMM_H_INCLUDED */
