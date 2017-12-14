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
#ifndef MPIR_COLL_TYPES_H_INCLUDED
#define MPIR_COLL_TYPES_H_INCLUDED

#include "queue.h"

#define MPIR_COLL_FLAG_REDUCE_L 1
#define MPIR_COLL_FLAG_REDUCE_R 0

struct MPIR_Comm;
struct MPIR_Op;

#ifndef container_of
#define container_of(P,T,F) \
    ((T*) ((char *)P - offsetof(T,F)))
#endif

#define MPIR_COLL_Assert MPIR_Assert

#define MPIR_COLL_COMM(comm) (&((comm)->coll))
#define MPIR_COLL_REQ(req) (&((req)->coll))

#include "coll_sched_db.h"

/* Queue element data structure in collectives queue.
 * Only non-blocking collectives are put on the queue */
typedef struct MPIR_COLL_queue_elem_t {
    TAILQ_ENTRY(MPIR_COLL_queue_elem_t) list_data;
    int (*kick_fn) (struct MPIR_COLL_queue_elem_t *); /* fn to make progress on the collective */
} MPIR_COLL_queue_elem_t;

/* Collectives request data structure */
typedef struct MPIR_COLL_req_t {
    MPIR_COLL_queue_elem_t elem;
    void *sched; /* pointer to the schedule */
} MPIR_COLL_req_t;

typedef struct MPIR_COLL_progress_global_t {
    TAILQ_HEAD(MPIR_COLL_queue_t, MPIR_COLL_queue_elem_t) head;
    int (*progress_fn) (void);  /* function to make progress on the collectives queue */
} MPIR_COLL_progress_global_t;


/* Coll 'pre'-definitions */
#include "types_decl.h"

/* Generic datatypes */
/* collective algorithm communicators */
typedef struct {
    int use_tag;
    MPIR_COLL_STUB_STUB_comm_t stub_stub;
    MPIR_COLL_STUB_TREE_comm_t stub_tree;
    MPIR_COLL_MPICH_STUB_comm_t mpich_stub;
    MPIR_COLL_MPICH_TREE_comm_t mpich_tree;
} MPIR_COLL_comm_t;

/* global data for every algorithm communicator */
typedef struct {
    MPIR_COLL_STUB_global_t tsp_stub;
    MPIR_COLL_MPICH_global_t tsp_mpich;

    MPIR_COLL_STUB_STUB_global_t stub_stub;
    MPIR_COLL_STUB_TREE_global_t stub_tree;
    MPIR_COLL_MPICH_STUB_global_t mpich_stub;
    MPIR_COLL_MPICH_TREE_global_t mpich_tree;
} MPIR_COLL_global_t;

extern MPIR_COLL_global_t MPIR_COLL_global_instance;

#endif /* MPIR_COLL_TYPES_H_INCLUDED */
