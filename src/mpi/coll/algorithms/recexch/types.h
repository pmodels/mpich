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

#include "coll_args_generic_types.h"

typedef TSP_dt_t COLL_dt_t;
typedef TSP_op_t COLL_op_t;

typedef struct COLL_global_t {
} COLL_global_t;

typedef struct COLL_comm_t {
    int id;                     /*unique id for this communicator */
    TSP_comm_t tsp_comm;
    int *curTag;                /*tag for collective operations */
} COLL_comm_t;

typedef MPIC_req_t COLL_req_t;

typedef TSP_aint_t COLL_aint_t;


typedef struct {
    int coll_op;
    union {
        struct {
            allgather_args_t allgather;
            int k;
            int halving;
        } allgather;
        struct {
            allreduce_args_t allreduce;
            int k;
        } allreduce;
        struct {
            barrier_args_t barrier;
            int k;
        } barrier;
        struct {} scatter;
        struct {} gather;
        struct {} reduce;
        struct {} alltoall;
        struct {} alltoallv;
        struct {} alltoallw;
        struct {} reducescatter;
        struct {} scan;
        struct {} bcast;
        struct {} exscan;
        struct {} gatherv;
        struct {} allgatherv;
        struct {} scatterv;
    } args;
} COLL_args_t;
