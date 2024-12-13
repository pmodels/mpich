/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_GROUP_H_INCLUDED
#define MPIR_GROUP_H_INCLUDED

/*---------------------------------------------------------------------------
 * Groups are *not* a major data structure in MPICH-2.  They are provided
 * only because they are required for the group operations (e.g.,
 * MPI_Group_intersection) and for the scalable RMA synchronization
 *---------------------------------------------------------------------------*/

/*S
 MPIR_Group - Description of the Group data structure

 The processes in the group of 'MPI_COMM_WORLD' have lpid values 0 to 'size'-1,
 where 'size' is the size of 'MPI_COMM_WORLD'.  Processes created by
 'MPI_Comm_spawn' or 'MPI_Comm_spawn_multiple' or added by 'MPI_Comm_attach'
 or
 'MPI_Comm_connect'
 are numbered greater than 'size - 1' (on the calling process). See the
 discussion of LocalPID values.

 Note that when dynamic process creation is used, the pids are `not` unique
 across the universe of connected MPI processes.  This is ok, as long as
 pids are interpreted `only` on the process that owns them.

 Only for MPI-1 are the lpid''s equal to the `global` pids.  The local pids
 can be thought of as a reference not to the remote process itself, but
 how the remote process can be reached from this process.  We may want to
 have a structure 'MPID_Lpid_t' that contains information on the remote
 process, such as (for TCP) the hostname, ip address (it may be different if
 multiple interfaces are supported; we may even want plural ip addresses for
 stripping communication), and port (or ports).  For shared memory connected
 processes, it might have the address of a remote queue.  The lpid number
 is an index into a table of 'MPID_Lpid_t'''s that contain this (device- and
 method-specific) information.

 Module:
 Group-DS

 S*/

/* In addition to MPI_GROUP_EMPTY, internally we have a few more builtins */
#define MPIR_GROUP_WORLD  ((MPI_Group)0x48000001)
#define MPIR_GROUP_SELF   ((MPI_Group)0x48000002)

#define MPIR_GROUP_WORLD_PTR (MPIR_Group_builtin + 1)
#define MPIR_GROUP_SELF_PTR  (MPIR_Group_builtin + 2)

/* Worlds -
 * We need a device-independent way of identifying processes. Assuming the concept of
 * "worlds", we can describe a process with (world_idx, world_rank).
 *
 * The world_idx is a local id because each process may not see all worlds. Thus,
 * each process only can maintain a list of worlds as it encounters them. Thus,
 * a process id derived from (world_idx, world_rank) is referred as LPID, or
 * "local process id".
 *
 * Each process should maintain a table of worlds with sufficient information so
 * processes can match worlds upon connection or making address exchange.
 */

#define MPIR_NAMESPACE_MAX 128
struct MPIR_World {
    char namespace[MPIR_NAMESPACE_MAX];
    /* other useful fields */
    int num_procs;
};

extern struct MPIR_World MPIR_Worlds[];

int MPIR_add_world(const char *namespace, int num_procs);
int MPIR_find_world(const char *namespace);

/* Abstract the integer type for lpid (process id). It is possible to use 32-bit
 * in principle, but 64-bit is simpler since we can trivially combine
 * (world_idx, world_rank).
 */
typedef int64_t MPIR_Lpid;

struct MPIR_Pmap {
    int size;                   /* same as group->size, duplicate here so Pmap is logically complete */
    bool use_map;
    union {
        MPIR_Lpid *map;
        struct {
            MPIR_Lpid offset;
            MPIR_Lpid stride;
            MPIR_Lpid blocksize;
        } stride;
    } u;
};

struct MPIR_Group {
    MPIR_OBJECT_HEADER;         /* adds handle and ref_count fields */
    int size;                   /* Size of a group */
    int rank;                   /* rank of this process relative to this group */
    struct MPIR_Pmap pmap;
    MPIR_Session *session_ptr;  /* Pointer to session to which this group belongs */
#ifdef MPID_DEV_GROUP_DECL
     MPID_DEV_GROUP_DECL
#endif
};

/* NOTE-G1: is_local_dense_monotonic will be true iff the group meets the
 * following criteria:
 * 1) the lpids are all in the range [0,size-1], i.e. a subset of comm world
 * 2) the pids are sequentially numbered in increasing order, without any gaps,
 *    stride, or repetitions
 *
 * This additional information allows us to handle the common case (insofar as
 * group ops are common) for MPI_Group_translate_ranks where group2 is
 * group_of(MPI_COMM_WORLD), or some simple subset.  This is an important use
 * case for many MPI tool libraries, such as Scalasca.
 */

extern MPIR_Object_alloc_t MPIR_Group_mem;
/* Preallocated group objects */
extern MPIR_Group MPIR_Group_builtin[MPIR_GROUP_N_BUILTIN];
extern MPIR_Group MPIR_Group_direct[];

/* Object for empty group */
extern MPIR_Group *const MPIR_Group_empty;

#define MPIR_Group_add_ref(_group) \
    do { MPIR_Object_add_ref(_group); } while (0)

#define MPIR_Group_release_ref(_group, _inuse) \
     do { MPIR_Object_release_ref(_group, _inuse); } while (0)

int MPIR_Group_check_valid_ranks(MPIR_Group *, const int[], int);
int MPIR_Group_check_valid_ranges(MPIR_Group *, int[][3], int);
int MPIR_Group_create(int, MPIR_Group **);
int MPIR_Group_release(MPIR_Group * group_ptr);

int MPIR_Group_dup(MPIR_Group * old_group, MPIR_Session * session_ptr, MPIR_Group ** new_group_ptr);
int MPIR_Group_create_map(int size, int rank, MPIR_Session * session_ptr, MPIR_Lpid * map,
                          MPIR_Group ** new_group_ptr);
int MPIR_Group_create_stride(int size, int rank, MPIR_Session * session_ptr,
                             MPIR_Lpid offset, MPIR_Lpid stride, MPIR_Lpid blocksize,
                             MPIR_Group ** new_group_ptr);
MPIR_Lpid MPIR_Group_rank_to_lpid(MPIR_Group * group, int rank);
int MPIR_Group_lpid_to_rank(MPIR_Group * group, MPIR_Lpid lpid);

int MPIR_Group_check_subset(MPIR_Group * group_ptr, MPIR_Comm * comm_ptr);
void MPIR_Group_set_session_ptr(MPIR_Group * group_ptr, MPIR_Session * session_out);
int MPIR_Group_init(void);
void MPIR_Group_finalize(void);

#endif /* MPIR_GROUP_H_INCLUDED */
