/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

typedef struct COLL_global_t {
    TSP_dt_t control_dt;
}
COLL_global_t;

typedef struct COLL_tree_t {
    int        rank;
    int        nranks;
}
COLL_tree_t;

typedef struct COLL_tree_comm_t {
    COLL_tree_t  tree;
    int         *curTag;
}
COLL_tree_comm_t;

typedef struct COLL_dt_t {
    TSP_dt_t tsp_dt;
}
COLL_dt_t;

typedef struct COLL_op_t {
    TSP_op_t tsp_op;
}
COLL_op_t;

typedef struct COLL_comm_t {
    TSP_comm_t       tsp_comm;
    COLL_tree_comm_t tree_comm;
}
COLL_comm_t;

typedef struct COLL_sched_t {
    TSP_sched_t tsp_sched;
    int         sched_started;
}
COLL_sched_t;

typedef struct COLL_req_t {
    COLL_queue_elem_t  elem;
    COLL_sched_t      *phases;
}
COLL_req_t;

typedef struct COLL_aint_t {
    TSP_aint_t tsp_aint;
}
COLL_aint_t;

extern COLL_global_t COLL_global;
