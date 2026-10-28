/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef COLL_TYPES_H_INCLUDED
#define COLL_TYPES_H_INCLUDED

/* Type definitions from all algorithms */
#include "stubalgo_types.h"
#include "treealgo_types.h"
#include "utarray.h"

#define MPIR_COLL_FLAG_REDUCE_L 1
#define MPIR_COLL_FLAG_REDUCE_R 0

/* enumerator for different tree types */
typedef enum MPIR_Tree_type_t {
    MPIR_TREE_TYPE_KARY = 0,
    MPIR_TREE_TYPE_KNOMIAL_1,
    MPIR_TREE_TYPE_KNOMIAL_2,
    MPIR_TREE_TYPE_TOPOLOGY_AWARE,
    MPIR_TREE_TYPE_TOPOLOGY_AWARE_K,
    MPIR_TREE_TYPE_TOPOLOGY_WAVE,
} MPIR_Tree_type_t;

/* enumerator for different recexch types */
enum {
    MPIR_IALLREDUCE_RECEXCH_TYPE_SINGLE_BUFFER = 0,
    MPIR_IALLREDUCE_RECEXCH_TYPE_MULTIPLE_BUFFER
};

/* What an operation object of a nonblocking or persistent collective owns.
 * It is freed once, when the operation object is freed. */
typedef struct {
    UT_array buffers;           /* MPL_malloc'd memory, MPL_free'd */
} MPIR_Coll_owned_t;

/* Collectives request data structure */
typedef struct MPII_Coll_req_t {
    void *sched;                /* pointer to the schedule */

    struct MPII_Coll_req_t *next;       /* linked-list next pointer */
    struct MPII_Coll_req_t *prev;       /* linked-list prev pointer */
} MPII_Coll_req_t;

typedef struct {
    struct MPII_Coll_req_t *head;
} MPII_Coll_queue_t;

#endif /* COLL_TYPES_H_INCLUDED */
