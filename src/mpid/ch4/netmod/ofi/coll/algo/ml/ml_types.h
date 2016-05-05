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

typedef struct COLL_dt_t {
    COLL_NAMESPACE(hybrid_dt_t) dt;
} COLL_dt_t;

typedef struct COLL_op_t {
    COLL_NAMESPACE(hybrid_op_t) op;
} COLL_op_t;

typedef struct COLL_comm_t {
    COLL_NAMESPACE(hybrid_comm_t) comm;
} COLL_comm_t;

typedef struct COLL_req_t {
    COLL_queue_elem_t elem;
} COLL_req_t;

typedef struct COLL_sched_t {
    COLL_NAMESPACE(hybrid_sched_t) sched;
} COLL_sched_t;

typedef struct COLL_aint_t {
    COLL_NAMESPACE(hybrid_sched_t) aint;
} COLL_aint_t;

typedef struct COLL_global_t {
    int dummy;
} COLL_global_t;

extern COLL_global_t COLL_global;
