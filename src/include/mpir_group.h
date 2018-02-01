/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_GROUP_H_INCLUDED
#define MPIR_GROUP_H_INCLUDED

/*---------------------------------------------------------------------------
 * Groups are *not* a major data structure in MPICH-2.  They are provided
 * only because they are required for the group operations (e.g.,
 * MPI_Group_intersection) and for the scalable RMA synchronization
 *---------------------------------------------------------------------------*/
/* This structure is used to implement the group operations such as
   MPI_Group_translate_ranks */
typedef struct MPII_Group_pmap_t {
    int lpid;                   /* local process id, from VCONN */
    int next_lpid;              /* Index of next lpid (in lpid order) */
    int flag;                   /* marker, used to implement group operations */
} MPII_Group_pmap_t;

/* Any changes in the MPIR_Group structure must be made to the
   predefined value in MPIR_Group_builtin for MPI_GROUP_EMPTY in
   src/mpi/group/grouputil.c */
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
struct MPIR_Group {
    MPIR_OBJECT_HEADER;         /* adds handle and ref_count fields */
    int size;                   /* Size of a group */
    int rank;                   /* rank of this process relative to this
                                 * group */
    int idx_of_first_lpid;
    MPII_Group_pmap_t *lrank_to_lpid;   /* Array mapping a local rank to local
                                         * process number */
    int is_local_dense_monotonic;       /* see NOTE-G1 */

    /* We may want some additional data for the RMA syncrhonization calls */
    /* Other, device-specific information */
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
#define MPIR_GROUP_N_BUILTIN 1
extern MPIR_Group MPIR_Group_builtin[MPIR_GROUP_N_BUILTIN];
extern MPIR_Group MPIR_Group_direct[];

/* Object for empty group */
extern MPIR_Group *const MPIR_Group_empty;

#define MPIR_Group_add_ref(_group) \
    do { MPIR_Object_add_ref(_group); } while (0)

#define MPIR_Group_release_ref(_group, _inuse) \
     do { MPIR_Object_release_ref(_group, _inuse); } while (0)

int MPIR_Group_create(int, MPIR_Group **);
int MPIR_Group_release(MPIR_Group * group_ptr);

int MPIR_Group_compare_impl(MPIR_Group * group_ptr1, MPIR_Group * group_ptr2, int *result);
int MPIR_Group_difference_impl(MPIR_Group * group_ptr1, MPIR_Group * group_ptr2,
                               MPIR_Group ** new_group_ptr);
int MPIR_Group_excl_impl(MPIR_Group * group_ptr, int n, const int *ranks,
                         MPIR_Group ** new_group_ptr);
int MPIR_Group_free_impl(MPIR_Group * group_ptr);
int MPIR_Group_incl_impl(MPIR_Group * group_ptr, int n, const int *ranks,
                         MPIR_Group ** new_group_ptr);
int MPIR_Group_intersection_impl(MPIR_Group * group_ptr1, MPIR_Group * group_ptr2,
                                 MPIR_Group ** new_group_ptr);
int MPIR_Group_range_excl_impl(MPIR_Group * group_ptr, int n, int ranges[][3],
                               MPIR_Group ** new_group_ptr);
int MPIR_Group_range_incl_impl(MPIR_Group * group_ptr, int n, int ranges[][3],
                               MPIR_Group ** new_group_ptr);
int MPIR_Group_translate_ranks_impl(MPIR_Group * group_ptr1, int n, const int *ranks1,
                                    MPIR_Group * group_ptr2, int *ranks2);
int MPIR_Group_union_impl(MPIR_Group * group_ptr1, MPIR_Group * group_ptr2,
                          MPIR_Group ** new_group_ptr);
int MPIR_Group_check_subset(MPIR_Group * group_ptr, MPIR_Comm * comm_ptr);
int MPIR_Group_init(void);

/* internal functions */
void MPII_Group_setup_lpid_list(MPIR_Group *);

#endif /* MPIR_GROUP_H_INCLUDED */
