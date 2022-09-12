/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef RELEASE_GATHER_TYPES_H_INCLUDED
#define RELEASE_GATHER_TYPES_H_INCLUDED

#include "treealgo_types.h"

typedef enum {
    MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST,
    MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE,
    MPIDI_POSIX_RELEASE_GATHER_OPCODE_ALLREDUCE,
    MPIDI_POSIX_RELEASE_GATHER_OPCODE_BARRIER,
} MPIDI_POSIX_release_gather_opcode_t;

typedef enum MPIDI_POSIX_release_gather_tree_type_t {
    MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KARY,
    MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KNOMIAL_1,
    MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KNOMIAL_2,
} MPIDI_POSIX_release_gather_tree_type_t;

typedef struct MPIDI_POSIX_release_gather_comm_t {
    int is_initialized;
    int num_collective_calls;

    MPIR_Treealgo_tree_t bcast_tree, reduce_tree;
    /* shm mem allocated to this comm */
    MPI_Aint flags_shm_size;
    MPI_Aint bcast_shm_size;
    MPI_Aint reduce_shm_size;
    MPI_Aint gather_state, release_state;

    void *flags_addr, *bcast_buf_addr, *reduce_buf_addr;
    void *ibcast_flags_addr, *ireduce_flags_addr;
    void **child_reduce_buf_addr;
    MPL_atomic_uint64_t *release_flag_addr, *gather_flag_addr;

    /* parameters need persist for each communicator */
    int bcast_tree_type, bcast_tree_kval;
    int bcast_num_cells;

    int reduce_tree_type, reduce_tree_kval;
    int reduce_num_cells;
    int *ibcast_last_seq_no_completed, *ireduce_last_seq_no_completed;
} MPIDI_POSIX_release_gather_comm_t;

typedef struct MPIDI_POSIX_per_call_ibcast_info_t {
    int seq_no;
    uint64_t *parent_gather_flag_addr, *my_gather_flag_addr, *my_release_flag_addr;
    void *local_buf;
    MPI_Aint count;
    int root, tag;
    MPI_Datatype datatype;
    MPIR_Comm *comm_ptr;
    MPIR_Request *sreq, *rreq;
} MPIDI_POSIX_per_call_ibcast_info_t;

typedef struct MPIDI_POSIX_per_call_ireduce_info_t {
    int seq_no;
    uint64_t *parent_gather_flag_addr, *my_gather_flag_addr, *my_release_flag_addr;
    void *send_buf, *recv_buf;
    MPI_Aint count;
    int root, tag;
    MPI_Op op;
    MPI_Datatype datatype;
    MPIR_Comm *comm_ptr;
    MPIR_Request *sreq, *rreq;
} MPIDI_POSIX_per_call_ireduce_info_t;

#endif /* RELEASE_GATHER_TYPES_H_INCLUDED */
