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
    int id; /*unique id for the datatype*/
    TSP_dt_t tsp_dt;
} COLL_dt_t;

typedef struct COLL_op_t {
    int id; /*unique id for the operation*/
    TSP_op_t tsp_op;
} COLL_op_t;

typedef struct COLL_comm_t {
    TSP_comm_t tsp_comm;
    int* curTag; /*tag for collective operations*/
    int id; /*unique id for this communicator*/
} COLL_comm_t;

typedef struct COLL_sched_t {
    TSP_sched_t tsp_sched;
    int sched_started;
} COLL_sched_t;

typedef struct COLL_req_t {
    COLL_queue_elem_t elem;
    COLL_sched_t *phases;
} COLL_req_t;

typedef TSP_aint_t COLL_aint_t;

typedef struct COLL_global_t {
} COLL_global_t;

typedef struct {
    int algo; /*algorithm type*/
    int tsp; /*transport type*/
    int nargs; /*number of arguments*/
    union{
        struct{
            void *buf; int count; int dt_id; int root; int comm_id;
        }bcast;
        struct {
            void *sbuf; void* rbuf; int count; int dt_id; int op_id; int root; int comm_id;    
        } reduce;
        struct {
            void *sbuf; void *rbuf; int count; int dt_id; int op_id; int comm_id;
        }allreduce;
    } args;
} COLL_args_t;
