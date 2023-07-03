/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_CSEL_CONTAINER_H_INCLUDED
#define MPIR_CSEL_CONTAINER_H_INCLUDED

#include <json.h>

void *MPII_Create_container(struct json_object *obj);

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
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_reduce_scatter_allgather,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_smp,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_tree,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_recexch,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_ring,
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_k_reduce_scatter_allgather,
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
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__Algorithm_count,
} MPII_Csel_container_type_e;

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
                int tree_type;
                int k;
                int chunk_size;
                int buffer_per_child;
            } intra_tree;
            struct {
                int k;
                bool single_phase_recv;
            } intra_recexch;
            struct {
                int k;
                bool single_phase_recv;
            } intra_k_reduce_scatter_allgather;
        } allreduce;
        struct {
            struct {
                int k;
            } intra_tsp_tree;
        } iscatter;
    } u;
} MPII_Csel_container_s;

#endif /* MPIR_CSEL_CONTAINER_H_INCLUDED */
