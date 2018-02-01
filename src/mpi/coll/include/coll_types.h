/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef COLL_TYPES_H_INCLUDED
#define COLL_TYPES_H_INCLUDED

/* include types from transports */
#include "tsp_stubtran_types.h"
#include "tsp_gentran_types.h"

/* Type definitions from all algorithms */
#include "stubalgo_types.h"
#include "treealgo_types.h"

#define MPIR_COLL_FLAG_REDUCE_L 1
#define MPIR_COLL_FLAG_REDUCE_R 0

/* enumerator for different tree types */
enum {
    TREE_TYPE_KNOMIAL = 0,
    TREE_TYPE_KARY
};

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
