/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_CSEL_H_INCLUDED
#define MPIR_CSEL_H_INCLUDED

#include "json.h"
#include "coll_impl.h"

typedef enum {
    MPIR_CSEL_COLL_TYPE__ALLGATHER = 0,
    MPIR_CSEL_COLL_TYPE__ALLGATHERV,
    MPIR_CSEL_COLL_TYPE__ALLREDUCE,
    MPIR_CSEL_COLL_TYPE__ALLTOALL,
    MPIR_CSEL_COLL_TYPE__ALLTOALLV,
    MPIR_CSEL_COLL_TYPE__ALLTOALLW,
    MPIR_CSEL_COLL_TYPE__BARRIER,
    MPIR_CSEL_COLL_TYPE__BCAST,
    MPIR_CSEL_COLL_TYPE__EXSCAN,
    MPIR_CSEL_COLL_TYPE__GATHER,
    MPIR_CSEL_COLL_TYPE__GATHERV,
    MPIR_CSEL_COLL_TYPE__IALLGATHER,
    MPIR_CSEL_COLL_TYPE__IALLGATHERV,
    MPIR_CSEL_COLL_TYPE__IALLREDUCE,
    MPIR_CSEL_COLL_TYPE__IALLTOALL,
    MPIR_CSEL_COLL_TYPE__IALLTOALLV,
    MPIR_CSEL_COLL_TYPE__IALLTOALLW,
    MPIR_CSEL_COLL_TYPE__IBARRIER,
    MPIR_CSEL_COLL_TYPE__IBCAST,
    MPIR_CSEL_COLL_TYPE__IEXSCAN,
    MPIR_CSEL_COLL_TYPE__IGATHER,
    MPIR_CSEL_COLL_TYPE__IGATHERV,
    MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLGATHER,
    MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLGATHERV,
    MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLTOALL,
    MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLTOALLV,
    MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLTOALLW,
    MPIR_CSEL_COLL_TYPE__IREDUCE,
    MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER,
    MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER_BLOCK,
    MPIR_CSEL_COLL_TYPE__ISCAN,
    MPIR_CSEL_COLL_TYPE__ISCATTER,
    MPIR_CSEL_COLL_TYPE__ISCATTERV,
    MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLGATHER,
    MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLGATHERV,
    MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLTOALL,
    MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLTOALLV,
    MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLTOALLW,
    MPIR_CSEL_COLL_TYPE__REDUCE,
    MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER,
    MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER_BLOCK,
    MPIR_CSEL_COLL_TYPE__SCAN,
    MPIR_CSEL_COLL_TYPE__SCATTER,
    MPIR_CSEL_COLL_TYPE__SCATTERV,
    MPIR_CSEL_COLL_TYPE__END,
} MPIR_Csel_coll_type_e;

typedef enum {
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_brucks,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_k_brucks,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_recursive_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_ring,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_recexch_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_recexch_halving,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_inter_local_gather_remote_bcast,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_intra_brucks,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_intra_recursive_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_intra_ring,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_inter_remote_gather_local_bcast,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_recursive_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_recursive_multiplying,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_reduce_scatter_allgather,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_smp,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_tree,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_recexch,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_ring,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_k_reduce_scatter_allgather,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_ccl,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_inter_reduce_exchange_bcast,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_brucks,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_k_brucks,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_pairwise,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_pairwise_sendrecv_replace,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_scattered,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_inter_pairwise_exchange,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallv_intra_pairwise_sendrecv_replace,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallv_intra_scattered,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallv_inter_pairwise_exchange,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallv_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallw_intra_pairwise_sendrecv_replace,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallw_intra_scattered,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallw_inter_pairwise_exchange,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallw_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_intra_k_dissemination,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_intra_recexch,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_intra_smp,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_inter_bcast,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_binomial,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_scatter_recursive_doubling_allgather,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_scatter_ring_allgather,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_smp,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_tree,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_pipelined_tree,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_inter_remote_send_local_bcast,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Exscan_intra_recursive_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Exscan_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gather_intra_binomial,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gather_inter_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gather_inter_local_gather_remote_send,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gather_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gatherv_allcomm_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gatherv_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_tsp_brucks,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_sched_brucks,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_sched_recursive_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_sched_ring,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_tsp_recexch_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_tsp_recexch_halving,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_tsp_ring,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_inter_sched_local_gather_remote_bcast,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_tsp_brucks,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_brucks,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_recursive_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_ring,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_tsp_recexch_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_tsp_recexch_halving,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_tsp_ring,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_naive,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_recursive_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_reduce_scatter_allgather,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_single_buffer,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_multiple_buffer,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_tree,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_ring,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_reduce_scatter_recexch_allgatherv,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_smp,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_inter_sched_remote_reduce_local_bcast,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_tsp_ring,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_tsp_brucks,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_tsp_scattered,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_brucks,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_inplace,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_pairwise,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_permuted_sendrecv,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_inter_sched_pairwise_exchange,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_sched_blocked,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_sched_inplace,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_tsp_scattered,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_tsp_blocked,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_tsp_inplace,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_inter_sched_pairwise_exchange,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_tsp_blocked,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_tsp_inplace,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_sched_blocked,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_sched_inplace,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_inter_sched_pairwise_exchange,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_sched_recursive_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_tsp_recexch,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_tsp_k_dissemination,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_inter_sched_bcast,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_tree,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_scatterv_recexch_allgatherv,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_scatterv_ring_allgatherv,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_ring,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_binomial,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_scatter_recursive_doubling_allgather,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_scatter_ring_allgather,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_smp,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_inter_sched_flat,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iexscan_intra_sched_recursive_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_intra_tsp_tree,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_intra_sched_binomial,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_inter_sched_long,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_inter_sched_short,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igatherv_allcomm_tsp_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igatherv_allcomm_sched_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_allgather_allcomm_tsp_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_allgather_allcomm_sched_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_allgatherv_allcomm_tsp_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_allgatherv_allcomm_sched_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoall_allcomm_tsp_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoall_allcomm_sched_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallv_allcomm_tsp_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallv_allcomm_sched_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallw_allcomm_tsp_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallw_allcomm_sched_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_tsp_tree,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_tsp_ring,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_sched_binomial,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_sched_reduce_scatter_gather,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_sched_smp,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_inter_sched_local_reduce_remote_send,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_noncommutative,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_pairwise,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_recursive_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_recursive_halving,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_tsp_recexch,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_intra_tsp_recexch,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_intra_sched_noncommutative,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_intra_sched_pairwise,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_intra_sched_recursive_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_intra_sched_recursive_halving,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_inter_sched_remote_reduce_local_scatterv,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscan_intra_sched_recursive_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscan_intra_sched_smp,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscan_intra_tsp_recursive_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_intra_tsp_tree,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_intra_sched_binomial,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_inter_sched_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_inter_sched_remote_send_local_scatter,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatterv_allcomm_tsp_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatterv_allcomm_sched_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_allgather_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_allgatherv_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_alltoall_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_alltoallv_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_alltoallw_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_intra_binomial,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_intra_reduce_scatter_gather,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_intra_smp,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_inter_local_reduce_remote_send,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_intra_noncommutative,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_intra_pairwise,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_intra_recursive_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_intra_recursive_halving,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_inter_remote_reduce_local_scatter,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_intra_noncommutative,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_intra_pairwise,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_intra_recursive_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_intra_recursive_halving,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_inter_remote_reduce_local_scatter,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scan_intra_recursive_doubling,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scan_intra_smp,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scan_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatter_intra_binomial,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatter_inter_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatter_inter_remote_send_local_scatter,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatter_allcomm_nb,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatterv_allcomm_linear,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatterv_allcomm_nb,
    /* composition algorithms */
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Coll_auto,
    /* end */
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__Algorithm_count,
} MPII_Csel_container_type_e;

typedef struct {
    MPIR_Csel_coll_type_e coll_type;
    MPIR_Comm *comm_ptr;

    union {
        struct {
            const void *sendbuf;
            MPI_Aint sendcount;
            MPI_Datatype sendtype;
            void *recvbuf;
            MPI_Aint recvcount;
            MPI_Datatype recvtype;
        } allgather, iallgather, neighbor_allgather, ineighbor_allgather;
        struct {
            const void *sendbuf;
            MPI_Aint sendcount;
            MPI_Datatype sendtype;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            const MPI_Aint *displs;
            MPI_Datatype recvtype;
        } allgatherv, iallgatherv, neighbor_allgatherv, ineighbor_allgatherv;
        struct {
            const void *sendbuf;
            void *recvbuf;
            MPI_Aint count;
            MPI_Datatype datatype;
            MPI_Op op;
        } allreduce, iallreduce;
        struct {
            const void *sendbuf;
            MPI_Aint sendcount;
            MPI_Datatype sendtype;
            MPI_Aint recvcount;
            void *recvbuf;
            MPI_Datatype recvtype;
        } alltoall, ialltoall, neighbor_alltoall, ineighbor_alltoall;
        struct {
            const void *sendbuf;
            const MPI_Aint *sendcounts;
            const MPI_Aint *sdispls;
            MPI_Datatype sendtype;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            const MPI_Aint *rdispls;
            MPI_Datatype recvtype;
        } alltoallv, ialltoallv, neighbor_alltoallv, ineighbor_alltoallv;
        struct {
            const void *sendbuf;
            const MPI_Aint *sendcounts;
            const MPI_Aint *sdispls;
            const MPI_Datatype *sendtypes;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            const MPI_Aint *rdispls;
            const MPI_Datatype *recvtypes;
        } alltoallw, ialltoallw;
        struct {
            const void *sendbuf;
            const MPI_Aint *sendcounts;
            const MPI_Aint *sdispls;
            const MPI_Datatype *sendtypes;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            const MPI_Aint *rdispls;
            const MPI_Datatype *recvtypes;
        } neighbor_alltoallw, ineighbor_alltoallw;
        struct {
            int dummy;          /* some compiler (suncc) doesn't like empty struct */
        } barrier, ibarrier;
        struct {
            void *buffer;
            MPI_Aint count;
            MPI_Datatype datatype;
            int root;
        } bcast, ibcast;
        struct {
            const void *sendbuf;
            void *recvbuf;
            MPI_Aint count;
            MPI_Datatype datatype;
            MPI_Op op;
        } exscan, iexscan;
        struct {
            const void *sendbuf;
            MPI_Aint sendcount;
            MPI_Datatype sendtype;
            MPI_Aint recvcount;
            void *recvbuf;
            MPI_Datatype recvtype;
            int root;
        } gather, igather, scatter, iscatter;
        struct {
            const void *sendbuf;
            MPI_Aint sendcount;
            MPI_Datatype sendtype;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            const MPI_Aint *displs;
            MPI_Datatype recvtype;
            int root;
        } gatherv, igatherv;
        struct {
            const void *sendbuf;
            void *recvbuf;
            MPI_Aint count;
            MPI_Datatype datatype;
            MPI_Op op;
            int root;
        } reduce, ireduce;
        struct {
            const void *sendbuf;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            MPI_Datatype datatype;
            MPI_Op op;
        } reduce_scatter, ireduce_scatter;
        struct {
            const void *sendbuf;
            void *recvbuf;
            MPI_Aint recvcount;
            MPI_Datatype datatype;
            MPI_Op op;
        } reduce_scatter_block, ireduce_scatter_block;
        struct {
            const void *sendbuf;
            void *recvbuf;
            MPI_Aint count;
            MPI_Datatype datatype;
            MPI_Op op;
        } scan, iscan;
        struct {
            const void *sendbuf;
            const MPI_Aint *sendcounts;
            const MPI_Aint *displs;
            MPI_Datatype sendtype;
            MPI_Aint recvcount;
            void *recvbuf;
            MPI_Datatype recvtype;
            int root;
        } scatterv, iscatterv;
    } u;
} MPIR_Csel_coll_sig_s;

typedef struct {
    MPII_Csel_container_type_e id;

    union {
        struct {
            struct {
                int k;
            } intra_tsp_brucks;
            struct {
                int k;
            } intra_tsp_recexch_doubling;
            struct {
                int k;
            } intra_tsp_recexch_halving;
        } iallgather;
        struct {
            struct {
                int k;
            } intra_tsp_brucks;
            struct {
                int k;
            } intra_tsp_recexch_doubling;
            struct {
                int k;
            } intra_tsp_recexch_halving;
        } iallgatherv;
        struct {
            struct {
                int k;
            } intra_tsp_recexch_single_buffer;
            struct {
                int k;
            } intra_tsp_recexch_multiple_buffer;
            struct {
                int tree_type;
                int k;
                int chunk_size;
                int buffer_per_child;
            } intra_tsp_tree;
            struct {
                int k;
            } intra_tsp_recexch_reduce_scatter_recexch_allgatherv;
        } iallreduce;
        struct {
            struct {
                int k;
                int buffer_per_phase;
            } intra_tsp_brucks;
            struct {
                int batch_size;
                int bblock;
            } intra_tsp_scattered;
        } ialltoall;
        struct {
            struct {
                int batch_size;
                int bblock;
            } intra_tsp_scattered;
            struct {
                int bblock;
            } intra_tsp_blocked;
        } ialltoallv;
        struct {
            struct {
                int bblock;
            } intra_tsp_blocked;
        } ialltoallw;
        struct {
            struct {
                int k;
            } intra_k_dissemination;
            struct {
                int k;
                bool single_phase_recv;
            } intra_recexch;
        } barrier;
        struct {
            struct {
                int k;
            } intra_tsp_recexch;
            struct {
                int k;
            } intra_tsp_k_dissemination;
        } ibarrier;
        struct {
            struct {
                int tree_type;
                int k;
                int chunk_size;
            } intra_tsp_tree;
            struct {
                int chunk_size;
            } intra_tsp_ring;
            struct {
                int scatterv_k;
                int allgatherv_k;
            } intra_tsp_scatterv_recexch_allgatherv;
            struct {
                int scatterv_k;
            } intra_tsp_scatterv_ring_allgatherv;
        } ibcast;
        struct {
            struct {
                int tree_type;
                int k;
                int is_non_blocking;
                int topo_overhead;
                int topo_diff_groups;
                int topo_diff_switches;
                int topo_same_switches;
            } intra_tree;
            struct {
                int tree_type;
                int k;
                int is_non_blocking;
                int chunk_size;
                int recv_pre_posted;
            } intra_pipelined_tree;
        } bcast;
        struct {
            struct {
                int k;
            } intra_k_brucks;
            struct {
                int k;
                bool single_phase_recv;
            } intra_recexch_doubling;
            struct {
                int k;
                bool single_phase_recv;
            } intra_recexch_halving;
        } allgather;
        struct {
            struct {
                int k;
            } intra_k_brucks;
        } alltoall;
        struct {
            struct {
                int k;
            } intra_tsp_tree;
        } igather;
        struct {
            struct {
                int tree_type;
                int k;
                int chunk_size;
                int buffer_per_child;
                int topo_overhead;
                int topo_diff_groups;
                int topo_diff_switches;
                int topo_same_switches;
            } intra_tsp_tree;
            struct {
                int chunk_size;
                int buffer_per_child;
            } intra_tsp_ring;
        } ireduce;
        struct {
            struct {
                int k;
            } intra_tsp_recexch;
        } ireduce_scatter;
        struct {
            struct {
                int k;
            } intra_tsp_recexch;
        } ireduce_scatter_block;
        struct {
            struct {
                int k;
            } intra_recursive_multiplying;
            struct {
                int tree_type;
                int k;
                int chunk_size;
                int buffer_per_child;
                int topo_overhead;
                int topo_diff_groups;
                int topo_diff_switches;
                int topo_same_switches;
            } intra_tree;
            struct {
                int k;
                bool single_phase_recv;
            } intra_recexch;
            struct {
                int k;
                bool single_phase_recv;
            } intra_k_reduce_scatter_allgather;
            struct {
                int ccl;
            } intra_ccl;
        } allreduce;
        struct {
            struct {
                int k;
            } intra_tsp_tree;
        } iscatter;
    } u;
} MPII_Csel_container_s;

int MPIR_Csel_create_from_file(const char *json_file,
                               void *(*create_container) (struct json_object *), void **csel);
int MPIR_Csel_create_from_buf(const char *json,
                              void *(*create_container) (struct json_object *), void **csel);
int MPIR_Csel_free(void *csel);
int MPIR_Csel_prune(void *root_csel, MPIR_Comm * comm_ptr, void **comm_csel);
void *MPIR_Csel_search(void *csel, MPIR_Csel_coll_sig_s * coll_sig);

void *MPII_Create_container(struct json_object *obj);

typedef int (*MPIR_Coll_algo_fn) (MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt);
void MPIR_Coll_algo_init(void);
/* NOTE: MPIR_Coll_auto is one of the composition container functions. However,
 *       MPIR_Coll_composition_auto is a gate function, thus does not take "cnt" parameter. */
int MPIR_Coll_composition_auto(MPIR_Csel_coll_sig_s * coll_sig);
int MPIR_Coll_auto(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt);

#endif /* MPIR_CSEL_H_INCLUDED */
