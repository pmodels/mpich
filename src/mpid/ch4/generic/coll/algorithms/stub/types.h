/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
typedef struct COLL_dt_t {
    int id; /*unique id for the datatype*/
    TSP_dt_t tsp_dt;
} COLL_dt_t;

typedef struct COLL_op_t {
    int id; /*unique id for the operation*/
    TSP_op_t tsp_op;
} COLL_op_t;

typedef struct COLL_comm_t {
    int id; /*unique id for the communicator*/
    TSP_comm_t tsp_comm;
} COLL_comm_t;

typedef struct COLL_req_t {
    COLL_queue_elem_t elem;
} COLL_req_t;

typedef struct COLL_sched_t {
    TSP_sched_t tsp_sched;
} COLL_sched_t;

typedef long int  COLL_aint_t;

typedef struct COLL_global_t {
} COLL_global_t;

typedef struct {
}COLL_args_t;
