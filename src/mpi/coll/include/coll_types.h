/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef COLL_TYPES_H_INCLUDED
#define COLL_TYPES_H_INCLUDED

/* Type definitions from all algorithms */
#include "stubalgo_types.h"
#include "treealgo_types.h"

#define MPIR_COLL_FLAG_REDUCE_L 1
#define MPIR_COLL_FLAG_REDUCE_R 0

/* enumerator for different tree types */
typedef enum MPIR_Tree_type_t {
    MPIR_TREE_TYPE_KARY = 0,
    MPIR_TREE_TYPE_KNOMIAL_1,
    MPIR_TREE_TYPE_KNOMIAL_2,
} MPIR_Tree_type_t;

/* enumerator for different recexch types */
enum {
    MPIR_IALLREDUCE_RECEXCH_TYPE_SINGLE_BUFFER = 0,
    MPIR_IALLREDUCE_RECEXCH_TYPE_MULTIPLE_BUFFER
};

/* enumerator for different recexch types */
enum {
    MPIR_IALLGATHER_RECEXCH_TYPE_DISTANCE_DOUBLING = 0,
    MPIR_IALLGATHER_RECEXCH_TYPE_DISTANCE_HALVING
};

/* enumerator for different recexch types */
enum {
    MPIR_IALLGATHERV_RECEXCH_TYPE_DISTANCE_DOUBLING = 0,
    MPIR_IALLGATHERV_RECEXCH_TYPE_DISTANCE_HALVING
};

/* enumerator for different reduce scatter types */
enum {
    IREDUCE_SCATTER_RECEXCH_TYPE_DISTANCE_DOUBLING = 0,
    IREDUCE_SCATTER_RECEXCH_TYPE_DISTANCE_HALVING
};

/* Collectives request data structure */
typedef struct MPII_Coll_req_t {
    void *sched;                /* pointer to the schedule */

    /*
     * Fields used by GPU-aware fallback path for op collectives. GPU buffers
     * are swapped for host buffers for collective execution. If the user
     * recv buffer is on the GPU, data is copied to it at completion.
     */
    void *host_sendbuf;         /* temporary host buffer */
    void *host_recvbuf;         /* temporary recv buffer */
    void *user_recvbuf;         /* pointer to user recv buffer */
    MPI_Aint count;             /* recv count */
    MPI_Datatype datatype;      /* recv datatype */

    struct MPII_Coll_req_t *next;       /* linked-list next pointer */
    struct MPII_Coll_req_t *prev;       /* linked-list prev pointer */
} MPII_Coll_req_t;

typedef struct {
    struct MPII_Coll_req_t *head;
} MPII_Coll_queue_t;

#endif /* COLL_TYPES_H_INCLUDED */
