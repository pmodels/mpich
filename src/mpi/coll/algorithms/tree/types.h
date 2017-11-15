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

#include "coll_tree_types.h" /* from the ../common directory */
#include "coll_args_generic_types.h"

typedef struct MPIR_ALGO_global_t {
} MPIR_ALGO_global_t;

typedef struct MPIR_ALGO_tree_comm_t {
    int id; /* unique local id for the communicator */
    MPIR_ALGO_tree_t tree;
    int *curTag;
} MPIR_ALGO_tree_comm_t;

typedef MPIR_TSP_dt_t MPIR_ALGO_dt_t;

typedef MPIR_TSP_op_t MPIR_ALGO_op_t;

typedef struct MPIR_ALGO_comm_t {
    MPIR_TSP_comm_t tsp_comm;        /* transport communicator */
    MPIR_ALGO_tree_comm_t tree_comm; /* algorithm specific structures */
} MPIR_ALGO_comm_t;

typedef MPIC_req_t MPIR_ALGO_req_t;
typedef long int MPIR_ALGO_aint_t;

typedef struct {
    int coll_op;
    union {
        struct {
            bcast_args_t bcast;
            int tree_type;
            int k;
            int segsize;
        } bcast;
        struct {
            reduce_args_t reduce;
            int nbuffers;
            int tree_type;
            int k;
            int segsize;
        } reduce;
        struct {
            allreduce_args_t allreduce;
            int tree_type;
            int k;
        } allreduce;
        struct {
            barrier_args_t barrier;
            int tree_type;
            int k;
        } barrier;
        struct {} scatter;
        struct {} gather;
        struct {} allgather;
        struct {} alltoall;
        struct {} alltoallv;
        struct {} alltoallw;
        struct {} reducescatter;
        struct {} scan;
        struct {} exscan;
        struct {} gatherv;
        struct {} allgatherv;
        struct {} scatterv;
    } args;
} MPIR_ALGO_args_t;    /* structure used as key for schedule database */
