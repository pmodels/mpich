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

typedef struct COLL_global_t {
} COLL_global_t;

typedef struct COLL_tree_t {
    int rank;
    int nranks;
} COLL_tree_t;

typedef TSP_dt_t COLL_dt_t;
typedef TSP_op_t OLL_op_t;

typedef struct COLL_comm_t {
    int id;
    int *curTag;
    int rank;                   /*my rank */
    int nranks;                 /*total ranks */
    TSP_comm_t tsp_comm;
} COLL_comm_t;

typedef MPIC_req_t COLL_req_t;

typedef struct {
    int coll_op;
    union {
        struct {
            bcast_args_t bcast;
        } bcast;
        struct {
            reduce_args_t reduce;
        } reduce;
        struct {
            allreduce_args_t allreduce;
        } allreduce;
        struct {
            barrier_args_t barrier;
            int k;
        } barrier;
        struct {} scatter;
        struct {} gather;
        struct {} allgather;
        struct {
            alltoall_args_t alltoall;
            int k;
            bool buffer_per_phase;
        } alltoall;
        struct {} alltoallv;
        struct {} alltoallw;
        struct {} reducescatter;
        struct {} scan;
        struct {} exscan;
        struct {} gatherv;
        struct {} allgatherv;
        struct {} scatterv;
    } args;
} COLL_args_t;    /* structure used as key for schedule database */
