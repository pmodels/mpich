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

#include "allreduce_group.h"

#ifndef COLL_IMPL_H_INCLUDED
#define COLL_IMPL_H_INCLUDED

#include "stubtran_impl.h"
#include "gentran_impl.h"

#include "../algorithms/stubalgo/stubalgo.h"
#include "../algorithms/treealgo/treealgo.h"
#include "../algorithms/recexchalgo/recexchalgo.h"

extern int MPIR_Nbc_progress_hook_id;

/* Enumerate the list of algorithms */
typedef enum MPIR_Allgather_intra_algo_t {
    MPIR_ALLGATHER_INTRA_ALGO_AUTO,
    MPIR_ALLGATHER_INTRA_ALGO_BRUCKS,
    MPIR_ALLGATHER_INTRA_ALGO_NB,
    MPIR_ALLGATHER_INTRA_ALGO_RECURSIVE_DOUBLING,
    MPIR_ALLGATHER_INTRA_ALGO_RING,
} MPIR_Allgather_intra_algo_t;
/* Have an easy way of finding the algorithm choice later without doing string
 * comparisons */
extern MPIR_Allgather_intra_algo_t MPIR_Allgather_intra_algo_choice;

typedef enum MPIR_Allgather_inter_algo_t {
    MPIR_ALLGATHER_INTER_ALGO_AUTO,
    MPIR_ALLGATHER_INTER_ALGO_LOCAL_GATHER_REMOTE_BCAST,
    MPIR_ALLGATHER_INTER_ALGO_NB,
} MPIR_Allgather_inter_algo_t;
extern MPIR_Allgather_inter_algo_t MPIR_Allgather_inter_algo_choice;

typedef enum MPIR_Allgatherv_intra_algo_t {
    MPIR_ALLGATHERV_INTRA_ALGO_AUTO,
    MPIR_ALLGATHERV_INTRA_ALGO_BRUCKS,
    MPIR_ALLGATHERV_INTRA_ALGO_NB,
    MPIR_ALLGATHERV_INTRA_ALGO_RECURSIVE_DOUBLING,
    MPIR_ALLGATHERV_INTRA_ALGO_RING,
} MPIR_Allgatherv_intra_algo_t;
extern MPIR_Allgatherv_intra_algo_t MPIR_Allgatherv_intra_algo_choice;

typedef enum MPIR_Allgatherv_inter_algo_t {
    MPIR_ALLGATHERV_INTER_ALGO_AUTO,
    MPIR_ALLGATHERV_INTER_ALGO_NB,
    MPIR_ALLGATHERV_INTER_ALGO_REMOTE_GATHER_LOCAL_BCAST,
} MPIR_Allgatherv_inter_algo_t;
extern MPIR_Allgatherv_inter_algo_t MPIR_Allgatherv_inter_algo_choice;

typedef enum MPIR_Allreduce_intra_algo_t {
    MPIR_ALLREDUCE_INTRA_ALGO_AUTO,
    MPIR_ALLREDUCE_INTRA_ALGO_NB,
    MPIR_ALLREDUCE_INTRA_ALGO_RECURSIVE_DOUBLING,
    MPIR_ALLREDUCE_INTRA_ALGO_REDUCE_SCATTER_ALLGATHER,
} MPIR_Allreduce_intra_algo_t;
extern MPIR_Allreduce_intra_algo_t MPIR_Allreduce_intra_algo_choice;

typedef enum MPIR_Allreduce_inter_algo_t {
    MPIR_ALLREDUCE_INTER_ALGO_AUTO,
    MPIR_ALLREDUCE_INTER_ALGO_NB,
    MPIR_ALLREDUCE_INTER_ALGO_REDUCE_EXCHANGE_BCAST,
} MPIR_Allreduce_inter_algo_t;
extern MPIR_Allreduce_inter_algo_t MPIR_Allreduce_inter_algo_choice;

typedef enum MPIR_Alltoall_intra_algo_t {
    MPIR_ALLTOALL_INTRA_ALGO_AUTO,
    MPIR_ALLTOALL_INTRA_ALGO_BRUCKS,
    MPIR_ALLTOALL_INTRA_ALGO_NB,
    MPIR_ALLTOALL_INTRA_ALGO_PAIRWISE,
    MPIR_ALLTOALL_INTRA_ALGO_PAIRWISE_SENDRECV_REPLACE,
    MPIR_ALLTOALL_INTRA_ALGO_SCATTERED,
} MPIR_Alltoall_intra_algo_t;
extern MPIR_Alltoall_intra_algo_t MPIR_Alltoall_intra_algo_choice;

typedef enum MPIR_Alltoall_inter_algo_t {
    MPIR_ALLTOALL_INTER_ALGO_AUTO,
    MPIR_ALLTOALL_INTER_ALGO_NB,
    MPIR_ALLTOALL_INTER_ALGO_PAIRWISE_EXCHANGE,
} MPIR_Alltoall_inter_algo_t;
extern MPIR_Alltoall_inter_algo_t MPIR_Alltoall_inter_algo_choice;

typedef enum MPIR_Alltoallv_intra_algo_t {
    MPIR_ALLTOALLV_INTRA_ALGO_AUTO,
    MPIR_ALLTOALLV_INTRA_ALGO_NB,
    MPIR_ALLTOALLV_INTRA_ALGO_PAIRWISE_SENDRECV_REPLACE,
    MPIR_ALLTOALLV_INTRA_ALGO_SCATTERED,
} MPIR_Alltoallv_intra_algo_t;
extern MPIR_Alltoallv_intra_algo_t MPIR_Alltoallv_intra_algo_choice;

typedef enum MPIR_Alltoallv_inter_algo_t {
    MPIR_ALLTOALLV_INTER_ALGO_AUTO,
    MPIR_ALLTOALLV_INTER_ALGO_NB,
    MPIR_ALLTOALLV_INTER_ALGO_PAIRWISE_EXCHANGE,
} MPIR_Alltoallv_inter_algo_t;
extern MPIR_Alltoallv_inter_algo_t MPIR_Alltoallv_inter_algo_choice;

typedef enum MPIR_Alltoallw_intra_algo_t {
    MPIR_ALLTOALLW_INTRA_ALGO_AUTO,
    MPIR_ALLTOALLW_INTRA_ALGO_NB,
    MPIR_ALLTOALLW_INTRA_ALGO_PAIRWISE_SENDRECV_REPLACE,
    MPIR_ALLTOALLW_INTRA_ALGO_SCATTERED,
} MPIR_Alltoallw_intra_algo_t;
extern MPIR_Alltoallw_intra_algo_t MPIR_Alltoallw_intra_algo_choice;

typedef enum MPIR_Alltoallw_inter_algo_t {
    MPIR_ALLTOALLW_INTER_ALGO_AUTO,
    MPIR_ALLTOALLW_INTER_ALGO_NB,
    MPIR_ALLTOALLW_INTER_ALGO_PAIRWISE_EXCHANGE,
} MPIR_Alltoallw_inter_algo_t;
extern MPIR_Alltoallw_inter_algo_t MPIR_Alltoallw_inter_algo_choice;

typedef enum MPIR_Barrier_intra_algo_t {
    MPIR_BARRIER_INTRA_ALGO_AUTO,
    MPIR_BARRIER_INTRA_ALGO_NB,
    MPIR_BARRIER_INTRA_ALGO_RECURSIVE_DOUBLING,
} MPIR_Barrier_intra_algo_t;
extern MPIR_Barrier_intra_algo_t MPIR_Barrier_intra_algo_choice;

typedef enum MPIR_Barrier_inter_algo_t {
    MPIR_BARRIER_INTER_ALGO_AUTO,
    MPIR_BARRIER_INTER_ALGO_BCAST,
    MPIR_BARRIER_INTER_ALGO_NB,
} MPIR_Barrier_inter_algo_t;
extern MPIR_Barrier_inter_algo_t MPIR_Barrier_inter_algo_choice;

typedef enum MPIR_Bcast_intra_algo_t {
    MPIR_BCAST_INTRA_ALGO_AUTO,
    MPIR_BCAST_INTRA_ALGO_BINOMIAL,
    MPIR_BCAST_INTRA_ALGO_NB,
    MPIR_BCAST_INTRA_ALGO_SCATTER_RECURSIVE_DOUBLING_ALLGATHER,
    MPIR_BCAST_INTRA_ALGO_SCATTER_RING_ALLGATHER,
} MPIR_Bcast_intra_algo_t;
extern MPIR_Bcast_intra_algo_t MPIR_Bcast_intra_algo_choice;

typedef enum MPIR_Bcast_inter_algo_t {
    MPIR_BCAST_INTER_ALGO_AUTO,
    MPIR_BCAST_INTER_ALGO_NB,
    MPIR_BCAST_INTER_ALGO_REMOTE_SEND_LOCAL_BCAST,
} MPIR_Bcast_inter_algo_t;
extern MPIR_Bcast_inter_algo_t MPIR_Bcast_inter_algo_choice;

typedef enum MPIR_Exscan_intra_algo_t {
    MPIR_EXSCAN_INTRA_ALGO_AUTO,
    MPIR_EXSCAN_INTRA_ALGO_NB,
    MPIR_EXSCAN_INTRA_ALGO_RECURSIVE_DOUBLING,
} MPIR_Exscan_intra_algo_t;
extern MPIR_Exscan_intra_algo_t MPIR_Exscan_intra_algo_choice;

typedef enum MPIR_Gather_intra_algo_t {
    MPIR_GATHER_INTRA_ALGO_AUTO,
    MPIR_GATHER_INTRA_ALGO_BINOMIAL,
    MPIR_GATHER_INTRA_ALGO_NB,
} MPIR_Gather_intra_algo_t;
extern MPIR_Gather_intra_algo_t MPIR_Gather_intra_algo_choice;

typedef enum MPIR_Gather_inter_algo_t {
    MPIR_GATHER_INTER_ALGO_AUTO,
    MPIR_GATHER_INTER_ALGO_LINEAR,
    MPIR_GATHER_INTER_ALGO_LOCAL_GATHER_REMOTE_SEND,
    MPIR_GATHER_INTER_ALGO_NB,
} MPIR_Gather_inter_algo_t;
extern MPIR_Gather_inter_algo_t MPIR_Gather_inter_algo_choice;

typedef enum MPIR_Gatherv_intra_algo_t {
    MPIR_GATHERV_INTRA_ALGO_AUTO,
    MPIR_GATHERV_INTRA_ALGO_LINEAR,
    MPIR_GATHERV_INTRA_ALGO_NB,
} MPIR_Gatherv_intra_algo_t;
extern MPIR_Gatherv_intra_algo_t MPIR_Gatherv_intra_algo_choice;

typedef enum MPIR_Gatherv_inter_algo_t {
    MPIR_GATHERV_INTER_ALGO_AUTO,
    MPIR_GATHERV_INTER_ALGO_LINEAR,
    MPIR_GATHERV_INTER_ALGO_NB,
} MPIR_Gatherv_inter_algo_t;
extern MPIR_Gatherv_inter_algo_t MPIR_Gatherv_inter_algo_choice;

typedef enum MPIR_Iallgather_intra_algo_t {
    MPIR_IALLGATHER_INTRA_ALGO_AUTO,
    MPIR_IALLGATHER_INTRA_ALGO_BRUCKS,
    MPIR_IALLGATHER_INTRA_ALGO_RECURSIVE_DOUBLING,
    MPIR_IALLGATHER_INTRA_ALGO_RING,
    MPIR_IALLGATHER_INTRA_ALGO_GENTRAN_RECEXCH_DISTANCE_DOUBLING,
    MPIR_IALLGATHER_INTRA_ALGO_GENTRAN_RECEXCH_DISTANCE_HALVING,
    MPIR_IALLGATHER_INTRA_ALGO_GENTRAN_BRUCKS,
} MPIR_Iallgather_intra_algo_t;
extern MPIR_Iallgather_intra_algo_t MPIR_Iallgather_intra_algo_choice;

typedef enum MPIR_Iallgather_inter_algo_t {
    MPIR_IALLGATHER_INTER_ALGO_AUTO,
    MPIR_IALLGATHER_INTER_ALGO_LOCAL_GATHER_REMOTE_BCAST,
} MPIR_Iallgather_inter_algo_t;
extern MPIR_Iallgather_inter_algo_t MPIR_Iallgather_inter_algo_choice;

typedef enum MPIR_Iallgatherv_intra_algo_t {
    MPIR_IALLGATHERV_INTRA_ALGO_AUTO,
    MPIR_IALLGATHERV_INTRA_ALGO_BRUCKS,
    MPIR_IALLGATHERV_INTRA_ALGO_RECURSIVE_DOUBLING,
    MPIR_IALLGATHERV_INTRA_ALGO_RING,
    MPIR_IALLGATHERV_INTRA_ALGO_GENTRAN_RECEXCH_DISTANCE_DOUBLING,
    MPIR_IALLGATHERV_INTRA_ALGO_GENTRAN_RECEXCH_DISTANCE_HALVING,
    MPIR_IALLGATHERV_INTRA_ALGO_GENTRAN_RING,
} MPIR_Iallgatherv_intra_algo_t;
extern MPIR_Iallgatherv_intra_algo_t MPIR_Iallgatherv_intra_algo_choice;

typedef enum MPIR_Iallgatherv_inter_algo_t {
    MPIR_IALLGATHERV_INTER_ALGO_AUTO,
    MPIR_IALLGATHERV_INTER_ALGO_REMOTE_GATHER_LOCAL_BCAST,
} MPIR_Iallgatherv_inter_algo_t;
extern MPIR_Iallgatherv_inter_algo_t MPIR_Iallgatherv_inter_algo_choice;

typedef enum MPIR_Iallreduce_intra_algo_t {
    MPIR_IALLREDUCE_INTRA_ALGO_AUTO,
    MPIR_IALLREDUCE_INTRA_ALGO_NAIVE,
    MPIR_IALLREDUCE_INTRA_ALGO_RECURSIVE_DOUBLING,
    MPIR_IALLREDUCE_INTRA_ALGO_REDUCE_SCATTER_ALLGATHER,
    MPIR_IALLREDUCE_INTRA_ALGO_GENTRAN_RECEXCH_SINGLE_BUFFER,
    MPIR_IALLREDUCE_INTRA_ALGO_GENTRAN_RECEXCH_MULTIPLE_BUFFER,
    MPIR_IALLREDUCE_INTRA_ALGO_GENTRAN_TREE_KARY,
    MPIR_IALLREDUCE_INTRA_ALGO_GENTRAN_TREE_KNOMIAL,
} MPIR_Iallreduce_intra_algo_t;
extern MPIR_Iallreduce_intra_algo_t MPIR_Iallreduce_intra_algo_choice;

typedef enum MPIR_Iallreduce_inter_algo_t {
    MPIR_IALLREDUCE_INTER_ALGO_AUTO,
    MPIR_IALLREDUCE_INTER_ALGO_REMOTE_REDUCE_LOCAL_BCAST,
} MPIR_Iallreduce_inter_algo_t;
extern MPIR_Iallreduce_inter_algo_t MPIR_Iallreduce_inter_algo_choice;

typedef enum MPIR_Ialltoall_intra_algo_t {
    MPIR_IALLTOALL_INTRA_ALGO_AUTO,
    MPIR_IALLTOALL_INTRA_ALGO_BRUCKS,
    MPIR_IALLTOALL_INTRA_ALGO_INPLACE,
    MPIR_IALLTOALL_INTRA_ALGO_PAIRWISE,
    MPIR_IALLTOALL_INTRA_ALGO_PERMUTED_SENDRECV,
} MPIR_Ialltoall_intra_algo_t;
extern MPIR_Ialltoall_intra_algo_t MPIR_Ialltoall_intra_algo_choice;

typedef enum MPIR_Ialltoall_inter_algo_t {
    MPIR_IALLTOALL_INTER_ALGO_AUTO,
    MPIR_IALLTOALL_INTER_ALGO_PAIRWISE_EXCHANGE,
} MPIR_Ialltoall_inter_algo_t;
extern MPIR_Ialltoall_inter_algo_t MPIR_Ialltoall_inter_algo_choice;

typedef enum MPIR_Ialltoallv_intra_algo_t {
    MPIR_IALLTOALLV_INTRA_ALGO_AUTO,
    MPIR_IALLTOALLV_INTRA_ALGO_BLOCKED,
    MPIR_IALLTOALLV_INTRA_ALGO_INPLACE,
} MPIR_Ialltoallv_intra_algo_t;
extern MPIR_Ialltoallv_intra_algo_t MPIR_Ialltoallv_intra_algo_choice;

typedef enum MPIR_Ialltoallv_inter_algo_t {
    MPIR_IALLTOALLV_INTER_ALGO_AUTO,
    MPIR_IALLTOALLV_INTER_ALGO_PAIRWISE_EXCHANGE,
} MPIR_Ialltoallv_inter_algo_t;
extern MPIR_Ialltoallv_inter_algo_t MPIR_Ialltoallv_inter_algo_choice;

typedef enum MPIR_Ialltoallw_intra_algo_t {
    MPIR_IALLTOALLW_INTRA_ALGO_AUTO,
    MPIR_IALLTOALLW_INTRA_ALGO_BLOCKED,
    MPIR_IALLTOALLW_INTRA_ALGO_INPLACE,
} MPIR_Ialltoallw_intra_algo_t;
extern MPIR_Ialltoallw_intra_algo_t MPIR_Ialltoallw_intra_algo_choice;

typedef enum MPIR_Ialltoallw_inter_algo_t {
    MPIR_IALLTOALLW_INTER_ALGO_AUTO,
    MPIR_IALLTOALLW_INTER_ALGO_PAIRWISE_EXCHANGE,
} MPIR_Ialltoallw_inter_algo_t;
extern MPIR_Ialltoallw_inter_algo_t MPIR_Ialltoallw_inter_algo_choice;

typedef enum MPIR_Ibarrier_intra_algo_t {
    MPIR_IBARRIER_INTRA_ALGO_AUTO,
    MPIR_IBARRIER_INTRA_ALGO_RECURSIVE_DOUBLING,
    MPIR_IBARRIER_INTRA_ALGO_GENTRAN_RECEXCH,
} MPIR_Ibarrier_intra_algo_t;
extern MPIR_Ibarrier_intra_algo_t MPIR_Ibarrier_intra_algo_choice;

typedef enum MPIR_Ibarrier_inter_algo_t {
    MPIR_IBARRIER_INTER_ALGO_AUTO,
    MPIR_IBARRIER_INTER_ALGO_BCAST,
} MPIR_Ibarrier_inter_algo_t;
extern MPIR_Ibarrier_inter_algo_t MPIR_Ibarrier_inter_algo_choice;

typedef enum MPIR_Ibcast_intra_algo_t {
    MPIR_IBCAST_INTRA_ALGO_AUTO,
    MPIR_IBCAST_INTRA_ALGO_BINOMIAL,
    MPIR_IBCAST_INTRA_ALGO_SCATTER_RECURSIVE_DOUBLING_ALLGATHER,
    MPIR_IBCAST_INTRA_ALGO_SCATTER_RING_ALLGATHER,
    MPIR_IBCAST_INTRA_ALGO_GENTRAN_TREE,
    MPIR_IBCAST_INTRA_ALGO_GENTRAN_SCATTER_RECEXCH_ALLGATHER,
    MPIR_IBCAST_INTRA_ALGO_GENTRAN_RING,
} MPIR_Ibcast_intra_algo_t;
extern MPIR_Ibcast_intra_algo_t MPIR_Ibcast_intra_algo_choice;

typedef enum MPIR_Ibcast_inter_algo_t {
    MPIR_IBCAST_INTER_ALGO_AUTO,
    MPIR_IBCAST_INTER_ALGO_FLAT,
} MPIR_Ibcast_inter_algo_t;
extern MPIR_Ibcast_inter_algo_t MPIR_Ibcast_inter_algo_choice;

typedef enum MPIR_Iexscan_intra_algo_t {
    MPIR_IEXSCAN_INTRA_ALGO_AUTO,
    MPIR_IEXSCAN_INTRA_ALGO_RECURSIVE_DOUBLING,
} MPIR_Iexscan_intra_algo_t;
extern MPIR_Iexscan_intra_algo_t MPIR_Iexscan_intra_algo_choice;

typedef enum MPIR_Igather_intra_algo_t {
    MPIR_IGATHER_INTRA_ALGO_AUTO,
    MPIR_IGATHER_INTRA_ALGO_BINOMIAL,
    MPIR_IGATHER_INTRA_ALGO_TREE,
} MPIR_Igather_intra_algo_t;
extern MPIR_Igather_intra_algo_t MPIR_Igather_intra_algo_choice;

typedef enum MPIR_Igather_inter_algo_t {
    MPIR_IGATHER_INTER_ALGO_AUTO,
    MPIR_IGATHER_INTER_ALGO_LONG,
    MPIR_IGATHER_INTER_ALGO_SHORT,
} MPIR_Igather_inter_algo_t;
extern MPIR_Igather_inter_algo_t MPIR_Igather_inter_algo_choice;

typedef enum MPIR_Igatherv_intra_algo_t {
    MPIR_IGATHERV_INTRA_ALGO_AUTO,
    MPIR_IGATHERV_INTRA_ALGO_LINEAR,
} MPIR_Igatherv_intra_algo_t;
extern MPIR_Igatherv_intra_algo_t MPIR_Igatherv_intra_algo_choice;

typedef enum MPIR_Igatherv_inter_algo_t {
    MPIR_IGATHERV_INTER_ALGO_AUTO,
    MPIR_IGATHERV_INTER_ALGO_LINEAR,
} MPIR_Igatherv_inter_algo_t;
extern MPIR_Igatherv_inter_algo_t MPIR_Igatherv_inter_algo_choice;

typedef enum MPIR_Ineighbor_allgather_intra_algo_t {
    MPIR_INEIGHBOR_ALLGATHER_INTRA_ALGO_AUTO,
    MPIR_INEIGHBOR_ALLGATHER_INTRA_ALGO_LINEAR,
} MPIR_Ineighbor_allgather_intra_algo_t;
extern MPIR_Ineighbor_allgather_intra_algo_t MPIR_Ineighbor_allgather_intra_algo_choice;

typedef enum MPIR_Ineighbor_allgather_inter_algo_t {
    MPIR_INEIGHBOR_ALLGATHER_INTER_ALGO_AUTO,
    MPIR_INEIGHBOR_ALLGATHER_INTER_ALGO_LINEAR,
} MPIR_Ineighbor_allgather_inter_algo_t;
extern MPIR_Ineighbor_allgather_inter_algo_t MPIR_Ineighbor_allgather_inter_algo_choice;

typedef enum MPIR_Ineighbor_allgatherv_intra_algo_t {
    MPIR_INEIGHBOR_ALLGATHERV_INTRA_ALGO_AUTO,
    MPIR_INEIGHBOR_ALLGATHERV_INTRA_ALGO_LINEAR,
} MPIR_Ineighbor_allgatherv_intra_algo_t;
extern MPIR_Ineighbor_allgatherv_intra_algo_t MPIR_Ineighbor_allgatherv_intra_algo_choice;

typedef enum MPIR_Ineighbor_allgatherv_inter_algo_t {
    MPIR_INEIGHBOR_ALLGATHERV_INTER_ALGO_AUTO,
    MPIR_INEIGHBOR_ALLGATHERV_INTER_ALGO_LINEAR,
} MPIR_Ineighbor_allgatherv_inter_algo_t;
extern MPIR_Ineighbor_allgatherv_inter_algo_t MPIR_Ineighbor_allgatherv_inter_algo_choice;

typedef enum MPIR_Ineighbor_alltoall_intra_algo_t {
    MPIR_INEIGHBOR_ALLTOALL_INTRA_ALGO_AUTO,
    MPIR_INEIGHBOR_ALLTOALL_INTRA_ALGO_LINEAR,
} MPIR_Ineighbor_alltoall_intra_algo_t;
extern MPIR_Ineighbor_alltoall_intra_algo_t MPIR_Ineighbor_alltoall_intra_algo_choice;

typedef enum MPIR_Ineighbor_alltoall_inter_algo_t {
    MPIR_INEIGHBOR_ALLTOALL_INTER_ALGO_AUTO,
    MPIR_INEIGHBOR_ALLTOALL_INTER_ALGO_LINEAR,
} MPIR_Ineighbor_alltoall_inter_algo_t;
extern MPIR_Ineighbor_alltoall_inter_algo_t MPIR_Ineighbor_alltoall_inter_algo_choice;

typedef enum MPIR_Ineighbor_alltoallv_intra_algo_t {
    MPIR_INEIGHBOR_ALLTOALLV_INTRA_ALGO_AUTO,
    MPIR_INEIGHBOR_ALLTOALLV_INTRA_ALGO_LINEAR,
} MPIR_Ineighbor_alltoallv_intra_algo_t;
extern MPIR_Ineighbor_alltoallv_intra_algo_t MPIR_Ineighbor_alltoallv_intra_algo_choice;

typedef enum MPIR_Ineighbor_alltoallv_inter_algo_t {
    MPIR_INEIGHBOR_ALLTOALLV_INTER_ALGO_AUTO,
    MPIR_INEIGHBOR_ALLTOALLV_INTER_ALGO_LINEAR,
} MPIR_Ineighbor_alltoallv_inter_algo_t;
extern MPIR_Ineighbor_alltoallv_inter_algo_t MPIR_Ineighbor_alltoallv_inter_algo_choice;

typedef enum MPIR_Ineighbor_alltoallw_intra_algo_t {
    MPIR_INEIGHBOR_ALLTOALLW_INTRA_ALGO_AUTO,
    MPIR_INEIGHBOR_ALLTOALLW_INTRA_ALGO_LINEAR,
} MPIR_Ineighbor_alltoallw_intra_algo_t;
extern MPIR_Ineighbor_alltoallw_intra_algo_t MPIR_Ineighbor_alltoallw_intra_algo_choice;

typedef enum MPIR_Ineighbor_alltoallw_inter_algo_t {
    MPIR_INEIGHBOR_ALLTOALLW_INTER_ALGO_AUTO,
    MPIR_INEIGHBOR_ALLTOALLW_INTER_ALGO_LINEAR,
} MPIR_Ineighbor_alltoallw_inter_algo_t;
extern MPIR_Ineighbor_alltoallw_inter_algo_t MPIR_Ineighbor_alltoallw_inter_algo_choice;

typedef enum MPIR_Ireduce_scatter_intra_algo_t {
    MPIR_IREDUCE_SCATTER_INTRA_ALGO_AUTO,
    MPIR_IREDUCE_SCATTER_INTRA_ALGO_NONCOMMUTATIVE,
    MPIR_IREDUCE_SCATTER_INTRA_ALGO_PAIRWISE,
    MPIR_IREDUCE_SCATTER_INTRA_ALGO_RECURSIVE_DOUBLING,
    MPIR_IREDUCE_SCATTER_INTRA_ALGO_RECURSIVE_HALVING,
    MPIR_IREDUCE_SCATTER_INTRA_ALGO_GENTRAN_RECEXCH,
} MPIR_Ireduce_scatter_intra_algo_t;
extern MPIR_Ireduce_scatter_intra_algo_t MPIR_Ireduce_scatter_intra_algo_choice;
extern MPIR_Tree_type_t MPIR_Ireduce_tree_type;
extern MPIR_Tree_type_t MPIR_Ibcast_tree_type;

typedef enum MPIR_Ireduce_scatter_inter_algo_t {
    MPIR_IREDUCE_SCATTER_INTER_ALGO_AUTO,
    MPIR_IREDUCE_SCATTER_INTER_ALGO_REMOTE_REDUCE_LOCAL_SCATTERV,
} MPIR_Ireduce_scatter_inter_algo_t;
extern MPIR_Ireduce_scatter_inter_algo_t MPIR_Ireduce_scatter_inter_algo_choice;

typedef enum MPIR_Ireduce_scatter_block_intra_algo_t {
    MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_AUTO,
    MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_NONCOMMUTATIVE,
    MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_PAIRWISE,
    MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_RECURSIVE_DOUBLING,
    MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_RECURSIVE_HALVING,
    MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_GENTRAN_RECEXCH,
} MPIR_Ireduce_scatter_block_intra_algo_t;
extern MPIR_Ireduce_scatter_block_intra_algo_t MPIR_Ireduce_scatter_block_intra_algo_choice;

typedef enum MPIR_Ireduce_scatter_block_inter_algo_t {
    MPIR_IREDUCE_SCATTER_BLOCK_INTER_ALGO_AUTO,
    MPIR_IREDUCE_SCATTER_BLOCK_INTER_ALGO_REMOTE_REDUCE_LOCAL_SCATTERV,
} MPIR_Ireduce_scatter_block_inter_algo_t;
extern MPIR_Ireduce_scatter_block_inter_algo_t MPIR_Ireduce_scatter_block_inter_algo_choice;

typedef enum MPIR_Ireduce_intra_algo_t {
    MPIR_IREDUCE_INTRA_ALGO_AUTO,
    MPIR_IREDUCE_INTRA_ALGO_BINOMIAL,
    MPIR_IREDUCE_INTRA_ALGO_REDUCE_SCATTER_GATHER,
    MPIR_IREDUCE_INTRA_ALGO_GENTRAN_TREE,
    MPIR_IREDUCE_INTRA_ALGO_GENTRAN_RING,
} MPIR_Ireduce_intra_algo_t;
extern MPIR_Ireduce_intra_algo_t MPIR_Ireduce_intra_algo_choice;

typedef enum MPIR_Ireduce_inter_algo_t {
    MPIR_IREDUCE_INTER_ALGO_AUTO,
    MPIR_IREDUCE_INTER_ALGO_LOCAL_REDUCE_REMOTE_SEND,
} MPIR_Ireduce_inter_algo_t;
extern MPIR_Ireduce_inter_algo_t MPIR_Ireduce_inter_algo_choice;

typedef enum MPIR_Iscan_intra_algo_t {
    MPIR_ISCAN_INTRA_ALGO_AUTO,
    MPIR_ISCAN_INTRA_ALGO_RECURSIVE_DOUBLING,
} MPIR_Iscan_intra_algo_t;
extern MPIR_Iscan_intra_algo_t MPIR_Iscan_intra_algo_choice;

typedef enum MPIR_Iscatter_intra_algo_t {
    MPIR_ISCATTER_INTRA_ALGO_AUTO,
    MPIR_ISCATTER_INTRA_ALGO_BINOMIAL,
    MPIR_ISCATTER_INTRA_ALGO_TREE
} MPIR_Iscatter_intra_algo_t;
extern MPIR_Iscatter_intra_algo_t MPIR_Iscatter_intra_algo_choice;

typedef enum MPIR_Iscatter_inter_algo_t {
    MPIR_ISCATTER_INTER_ALGO_AUTO,
    MPIR_ISCATTER_INTER_ALGO_LINEAR,
    MPIR_ISCATTER_INTER_ALGO_REMOTE_SEND_LOCAL_SCATTER,
} MPIR_Iscatter_inter_algo_t;
extern MPIR_Iscatter_inter_algo_t MPIR_Iscatter_inter_algo_choice;

typedef enum MPIR_Iscatterv_intra_algo_t {
    MPIR_ISCATTERV_INTRA_ALGO_AUTO,
    MPIR_ISCATTERV_INTRA_ALGO_LINEAR,
} MPIR_Iscatterv_intra_algo_t;
extern MPIR_Iscatterv_intra_algo_t MPIR_Iscatterv_intra_algo_choice;

typedef enum MPIR_Iscatterv_inter_algo_t {
    MPIR_ISCATTERV_INTER_ALGO_AUTO,
    MPIR_ISCATTERV_INTER_ALGO_LINEAR,
} MPIR_Iscatterv_inter_algo_t;
extern MPIR_Iscatterv_inter_algo_t MPIR_Iscatterv_inter_algo_choice;

typedef enum MPIR_Neighbor_allgather_intra_algo_t {
    MPIR_NEIGHBOR_ALLGATHER_INTRA_ALGO_AUTO,
    MPIR_NEIGHBOR_ALLGATHER_INTRA_ALGO_NB,
} MPIR_Neighbor_allgather_intra_algo_t;
extern MPIR_Neighbor_allgather_intra_algo_t MPIR_Neighbor_allgather_intra_algo_choice;

typedef enum MPIR_Neighbor_allgather_inter_algo_t {
    MPIR_NEIGHBOR_ALLGATHER_INTER_ALGO_AUTO,
    MPIR_NEIGHBOR_ALLGATHER_INTER_ALGO_NB,
} MPIR_Neighbor_allgather_inter_algo_t;
extern MPIR_Neighbor_allgather_inter_algo_t MPIR_Neighbor_allgather_inter_algo_choice;

typedef enum MPIR_Neighbor_allgatherv_intra_algo_t {
    MPIR_NEIGHBOR_ALLGATHERV_INTRA_ALGO_AUTO,
    MPIR_NEIGHBOR_ALLGATHERV_INTRA_ALGO_NB,
} MPIR_Neighbor_allgatherv_intra_algo_t;
extern MPIR_Neighbor_allgatherv_intra_algo_t MPIR_Neighbor_allgatherv_intra_algo_choice;

typedef enum MPIR_Neighbor_allgatherv_inter_algo_t {
    MPIR_NEIGHBOR_ALLGATHERV_INTER_ALGO_AUTO,
    MPIR_NEIGHBOR_ALLGATHERV_INTER_ALGO_NB,
} MPIR_Neighbor_allgatherv_inter_algo_t;
extern MPIR_Neighbor_allgatherv_inter_algo_t MPIR_Neighbor_allgatherv_inter_algo_choice;

typedef enum MPIR_Neighbor_alltoall_intra_algo_t {
    MPIR_NEIGHBOR_ALLTOALL_INTRA_ALGO_AUTO,
    MPIR_NEIGHBOR_ALLTOALL_INTRA_ALGO_NB,
} MPIR_Neighbor_alltoall_intra_algo_t;
extern MPIR_Neighbor_alltoall_intra_algo_t MPIR_Neighbor_alltoall_intra_algo_choice;

typedef enum MPIR_Neighbor_alltoall_inter_algo_t {
    MPIR_NEIGHBOR_ALLTOALL_INTER_ALGO_AUTO,
    MPIR_NEIGHBOR_ALLTOALL_INTER_ALGO_NB,
} MPIR_Neighbor_alltoall_inter_algo_t;
extern MPIR_Neighbor_alltoall_inter_algo_t MPIR_Neighbor_alltoall_inter_algo_choice;

typedef enum MPIR_Neighbor_alltoallv_intra_algo_t {
    MPIR_NEIGHBOR_ALLTOALLV_INTRA_ALGO_AUTO,
    MPIR_NEIGHBOR_ALLTOALLV_INTRA_ALGO_NB,
} MPIR_Neighbor_alltoallv_intra_algo_t;
extern MPIR_Neighbor_alltoallv_intra_algo_t MPIR_Neighbor_alltoallv_intra_algo_choice;

typedef enum MPIR_Neighbor_alltoallv_inter_algo_t {
    MPIR_NEIGHBOR_ALLTOALLV_INTER_ALGO_AUTO,
    MPIR_NEIGHBOR_ALLTOALLV_INTER_ALGO_NB,
} MPIR_Neighbor_alltoallv_inter_algo_t;
extern MPIR_Neighbor_alltoallv_inter_algo_t MPIR_Neighbor_alltoallv_inter_algo_choice;

typedef enum MPIR_Neighbor_alltoallw_intra_algo_t {
    MPIR_NEIGHBOR_ALLTOALLW_INTRA_ALGO_AUTO,
    MPIR_NEIGHBOR_ALLTOALLW_INTRA_ALGO_NB,
} MPIR_Neighbor_alltoallw_intra_algo_t;
extern MPIR_Neighbor_alltoallw_intra_algo_t MPIR_Neighbor_alltoallw_intra_algo_choice;

typedef enum MPIR_Neighbor_alltoallw_inter_algo_t {
    MPIR_NEIGHBOR_ALLTOALLW_INTER_ALGO_AUTO,
    MPIR_NEIGHBOR_ALLTOALLW_INTER_ALGO_NB,
} MPIR_Neighbor_alltoallw_inter_algo_t;
extern MPIR_Neighbor_alltoallw_inter_algo_t MPIR_Neighbor_alltoallw_inter_algo_choice;

typedef enum MPIR_Reduce_scatter_intra_algo_t {
    MPIR_REDUCE_SCATTER_INTRA_ALGO_AUTO,
    MPIR_REDUCE_SCATTER_INTRA_ALGO_NB,
    MPIR_REDUCE_SCATTER_INTRA_ALGO_NONCOMMUTATIVE,
    MPIR_REDUCE_SCATTER_INTRA_ALGO_PAIRWISE,
    MPIR_REDUCE_SCATTER_INTRA_ALGO_RECURSIVE_DOUBLING,
    MPIR_REDUCE_SCATTER_INTRA_ALGO_RECURSIVE_HALVING,
} MPIR_Reduce_scatter_intra_algo_t;
extern MPIR_Reduce_scatter_intra_algo_t MPIR_Reduce_scatter_intra_algo_choice;

typedef enum MPIR_Reduce_scatter_inter_algo_t {
    MPIR_REDUCE_SCATTER_INTER_ALGO_AUTO,
    MPIR_REDUCE_SCATTER_INTER_ALGO_NB,
    MPIR_REDUCE_SCATTER_INTER_ALGO_REMOTE_REDUCE_LOCAL_SCATTER,
} MPIR_Reduce_scatter_inter_algo_t;
extern MPIR_Reduce_scatter_inter_algo_t MPIR_Reduce_scatter_inter_algo_choice;

typedef enum MPIR_Reduce_scatter_block_intra_algo_t {
    MPIR_REDUCE_SCATTER_BLOCK_INTRA_ALGO_AUTO,
    MPIR_REDUCE_SCATTER_BLOCK_INTRA_ALGO_NB,
    MPIR_REDUCE_SCATTER_BLOCK_INTRA_ALGO_NONCOMMUTATIVE,
    MPIR_REDUCE_SCATTER_BLOCK_INTRA_ALGO_PAIRWISE,
    MPIR_REDUCE_SCATTER_BLOCK_INTRA_ALGO_RECURSIVE_DOUBLING,
    MPIR_REDUCE_SCATTER_BLOCK_INTRA_ALGO_RECURSIVE_HALVING,
} MPIR_Reduce_scatter_block_intra_algo_t;
extern MPIR_Reduce_scatter_block_intra_algo_t MPIR_Reduce_scatter_block_intra_algo_choice;

typedef enum MPIR_Reduce_scatter_block_inter_algo_t {
    MPIR_REDUCE_SCATTER_BLOCK_INTER_ALGO_AUTO,
    MPIR_REDUCE_SCATTER_BLOCK_INTER_ALGO_NB,
    MPIR_REDUCE_SCATTER_BLOCK_INTER_ALGO_REMOTE_REDUCE_LOCAL_SCATTER,
} MPIR_Reduce_scatter_block_inter_algo_t;
extern MPIR_Reduce_scatter_block_inter_algo_t MPIR_Reduce_scatter_block_inter_algo_choice;

typedef enum MPIR_Reduce_intra_algo_t {
    MPIR_REDUCE_INTRA_ALGO_AUTO,
    MPIR_REDUCE_INTRA_ALGO_BINOMIAL,
    MPIR_REDUCE_INTRA_ALGO_NB,
    MPIR_REDUCE_INTRA_ALGO_REDUCE_SCATTER_GATHER,
} MPIR_Reduce_intra_algo_t;
extern MPIR_Reduce_intra_algo_t MPIR_Reduce_intra_algo_choice;

typedef enum MPIR_Reduce_inter_algo_t {
    MPIR_REDUCE_INTER_ALGO_AUTO,
    MPIR_REDUCE_INTER_ALGO_LOCAL_REDUCE_REMOTE_SEND,
    MPIR_REDUCE_INTER_ALGO_NB,
} MPIR_Reduce_inter_algo_t;
extern MPIR_Reduce_inter_algo_t MPIR_Reduce_inter_algo_choice;

typedef enum MPIR_Scan_intra_algo_t {
    MPIR_SCAN_INTRA_ALGO_AUTO,
    MPIR_SCAN_INTRA_ALGO_NB,
    MPIR_SCAN_INTRA_ALGO_RECURSIVE_DOUBLING,
} MPIR_Scan_intra_algo_t;
extern MPIR_Scan_intra_algo_t MPIR_Scan_intra_algo_choice;

typedef enum MPIR_Scatter_intra_algo_t {
    MPIR_SCATTER_INTRA_ALGO_AUTO,
    MPIR_SCATTER_INTRA_ALGO_BINOMIAL,
    MPIR_SCATTER_INTRA_ALGO_NB,
} MPIR_Scatter_intra_algo_t;
extern MPIR_Scatter_intra_algo_t MPIR_Scatter_intra_algo_choice;

typedef enum MPIR_Scatter_inter_algo_t {
    MPIR_SCATTER_INTER_ALGO_AUTO,
    MPIR_SCATTER_INTER_ALGO_LINEAR,
    MPIR_SCATTER_INTER_ALGO_NB,
    MPIR_SCATTER_INTER_ALGO_REMOTE_SEND_LOCAL_SCATTER,
} MPIR_Scatter_inter_algo_t;
extern MPIR_Scatter_inter_algo_t MPIR_Scatter_inter_algo_choice;

typedef enum MPIR_Scatterv_intra_algo_t {
    MPIR_SCATTERV_INTRA_ALGO_AUTO,
    MPIR_SCATTERV_INTRA_ALGO_LINEAR,
    MPIR_SCATTERV_INTRA_ALGO_NB,
} MPIR_Scatterv_intra_algo_t;
extern MPIR_Scatterv_intra_algo_t MPIR_Scatterv_intra_algo_choice;

typedef enum MPIR_Scatterv_inter_algo_t {
    MPIR_SCATTERV_INTER_ALGO_AUTO,
    MPIR_SCATTERV_INTER_ALGO_LINEAR,
    MPIR_SCATTERV_INTER_ALGO_NB,
} MPIR_Scatterv_inter_algo_t;
extern MPIR_Scatterv_inter_algo_t MPIR_Scatterv_inter_algo_choice;

/* Function to initialze communicators for collectives */
int MPIR_Coll_comm_init(MPIR_Comm * comm);

/* Function to cleanup any communicators for collectives */
int MPII_Coll_comm_cleanup(MPIR_Comm * comm);

/* Hook for any collective algorithms related initialization */
int MPII_Coll_init(void);

int MPIR_Coll_safe_to_block(void);

int MPII_Coll_finalize(void);

#endif /* COLL_IMPL_H_INCLUDED */
