/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* TODO: this file should be generated */

#include "csel_internal.h"

const char *Csel_coll_type_str[] = {
    /* MPIR_CSEL_COLL_TYPE__ALLGATHER */ "allgather",
    /* MPIR_CSEL_COLL_TYPE__ALLGATHERV */ "allgatherv",
    /* MPIR_CSEL_COLL_TYPE__ALLREDUCE */ "allreduce",
    /* MPIR_CSEL_COLL_TYPE__ALLTOALL */ "alltoall",
    /* MPIR_CSEL_COLL_TYPE__ALLTOALLV */ "alltoallv",
    /* MPIR_CSEL_COLL_TYPE__ALLTOALLW */ "alltoallw",
    /* MPIR_CSEL_COLL_TYPE__BARRIER */ "barrier",
    /* MPIR_CSEL_COLL_TYPE__BCAST */ "bcast",
    /* MPIR_CSEL_COLL_TYPE__EXSCAN */ "exscan",
    /* MPIR_CSEL_COLL_TYPE__GATHER */ "gather",
    /* MPIR_CSEL_COLL_TYPE__GATHERV */ "gatherv",
    /* MPIR_CSEL_COLL_TYPE__IALLGATHER */ "iallgather",
    /* MPIR_CSEL_COLL_TYPE__IALLGATHERV */ "iallgatherv",
    /* MPIR_CSEL_COLL_TYPE__IALLREDUCE */ "iallreduce",
    /* MPIR_CSEL_COLL_TYPE__IALLTOALL */ "ialltoall",
    /* MPIR_CSEL_COLL_TYPE__IALLTOALLV */ "ialltoallv",
    /* MPIR_CSEL_COLL_TYPE__IALLTOALLW */ "ialltoallw",
    /* MPIR_CSEL_COLL_TYPE__IBARRIER */ "ibarrier",
    /* MPIR_CSEL_COLL_TYPE__IBCAST */ "ibcast",
    /* MPIR_CSEL_COLL_TYPE__IEXSCAN */ "iexscan",
    /* MPIR_CSEL_COLL_TYPE__IGATHER */ "igather",
    /* MPIR_CSEL_COLL_TYPE__IGATHERV */ "igatherv",
    /* MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLGATHER */ "ineighbor_allgather",
    /* MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLGATHERV */ "ineighbor_allgatherv",
    /* MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLTOALL */ "ineighbor_alltoall",
    /* MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLTOALLV */ "ineighbor_alltoallv",
    /* MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLTOALLW */ "ineighbor_alltoallw",
    /* MPIR_CSEL_COLL_TYPE__IREDUCE */ "ireduce",
    /* MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER */ "ireduce_scatter",
    /* MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER_BLOCK */ "ireduce_scatter_block",
    /* MPIR_CSEL_COLL_TYPE__ISCAN */ "iscan",
    /* MPIR_CSEL_COLL_TYPE__ISCATTER */ "iscatter",
    /* MPIR_CSEL_COLL_TYPE__ISCATTERV */ "iscatterv",
    /* MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLGATHER */ "neighbor_allgather",
    /* MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLGATHERV */ "neighbor_allgatherv",
    /* MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLTOALL */ "neighbor_alltoall",
    /* MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLTOALLV */ "neighbor_alltoallv",
    /* MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLTOALLW */ "neighbor_alltoallw",
    /* MPIR_CSEL_COLL_TYPE__REDUCE */ "reduce",
    /* MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER */ "reduce_scatter",
    /* MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER_BLOCK */ "reduce_scatter_block",
    /* MPIR_CSEL_COLL_TYPE__SCAN */ "scan",
    /* MPIR_CSEL_COLL_TYPE__SCATTER */ "scatter",
    /* MPIR_CSEL_COLL_TYPE__SCATTERV */ "scatterv",
    /* MPIR_CSEL_COLL_TYPE__END */ "!END_OF_COLLECTIVE"
};

const char *Csel_comm_hierarchy_str[] = {
    /* MPIR_CSEL_COMM_HIERARCHY__FLAT */ "flat",
    /* MPIR_CSEL_COMM_HIERARCHY__NODE */ "node",
    /* MPIR_CSEL_COMM_HIERARCHY__NODE_ROOTS */ "node_roots",
    /* MPIR_CSEL_COMM_HIERARCHY__PARENT */ "parent",
    /* MPIR_CSEL_COMM_HIERARCHY__END */ "!END_OF_COMM_HIERARCHY"
};

const char *Csel_container_type_str[] = {
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_brucks */
    "MPIR_Allgather_intra_brucks",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_k_brucks */
    "MPIR_Allgather_intra_k_brucks",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_recursive_doubling */
    "MPIR_Allgather_intra_recursive_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_ring */
    "MPIR_Allgather_intra_ring",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_recexch_doubling */
    "MPIR_Allgather_intra_recexch_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_recexch_halving */
    "MPIR_Allgather_intra_recexch_halving",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_inter_local_gather_remote_bcast */
    "MPIR_Allgather_inter_local_gather_remote_bcast",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_allcomm_nb */
    "MPIR_Allgather_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_intra_brucks */
    "MPIR_Allgatherv_intra_brucks",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_intra_recursive_doubling */
    "MPIR_Allgatherv_intra_recursive_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_intra_ring */
    "MPIR_Allgatherv_intra_ring",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_inter_remote_gather_local_bcast */
    "MPIR_Allgatherv_inter_remote_gather_local_bcast",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_allcomm_nb */
    "MPIR_Allgatherv_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_recursive_doubling */
    "MPIR_Allreduce_intra_recursive_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_recursive_multiplying */
    "MPIR_Allreduce_intra_recursive_multiplying",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_reduce_scatter_allgather */
    "MPIR_Allreduce_intra_reduce_scatter_allgather",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_smp */ "MPIR_Allreduce_intra_smp",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_tree */
    "MPIR_Allreduce_intra_tree",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_recexch */
    "MPIR_Allreduce_intra_recexch",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_ring */
    "MPIR_Allreduce_intra_ring",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_k_reduce_scatter_allgather */
    "MPIR_Allreduce_intra_k_reduce_scatter_allgather",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_ccl */ "MPIR_Allreduce_intra_ccl",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_inter_reduce_exchange_bcast */
    "MPIR_Allreduce_inter_reduce_exchange_bcast",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_allcomm_nb */
    "MPIR_Allreduce_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_brucks */
    "MPIR_Alltoall_intra_brucks",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_k_brucks */
    "MPIR_Alltoall_intra_k_brucks",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_pairwise */
    "MPIR_Alltoall_intra_pairwise",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_pairwise_sendrecv_replace */
    "MPIR_Alltoall_intra_pairwise_sendrecv_replace",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_scattered */
    "MPIR_Alltoall_intra_scattered",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_inter_pairwise_exchange */
    "MPIR_Alltoall_inter_pairwise_exchange",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_allcomm_nb */ "MPIR_Alltoall_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallv_intra_pairwise_sendrecv_replace */
    "MPIR_Alltoallv_intra_pairwise_sendrecv_replace",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallv_intra_scattered */
    "MPIR_Alltoallv_intra_scattered",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallv_inter_pairwise_exchange */
    "MPIR_Alltoallv_inter_pairwise_exchange",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallv_allcomm_nb */
    "MPIR_Alltoallv_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallw_intra_pairwise_sendrecv_replace */
    "MPIR_Alltoallw_intra_pairwise_sendrecv_replace",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallw_intra_scattered */
    "MPIR_Alltoallw_intra_scattered",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallw_inter_pairwise_exchange */
    "MPIR_Alltoallw_inter_pairwise_exchange",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallw_allcomm_nb */
    "MPIR_Alltoallw_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_intra_k_dissemination */
    "MPIR_Barrier_intra_k_dissemination",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_intra_recexch */
    "MPIR_Barrier_intra_recexch",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_intra_smp */ "MPIR_Barrier_intra_smp",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_inter_bcast */ "MPIR_Barrier_inter_bcast",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_allcomm_nb */ "MPIR_Barrier_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_binomial */
    "MPIR_Bcast_intra_binomial",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_scatter_recursive_doubling_allgather */
    "MPIR_Bcast_intra_scatter_recursive_doubling_allgather",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_scatter_ring_allgather */
    "MPIR_Bcast_intra_scatter_ring_allgather",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_smp */ "MPIR_Bcast_intra_smp",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_tree */ "MPIR_Bcast_intra_tree",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_pipelined_tree */
    "MPIR_Bcast_intra_pipelined_tree",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_inter_remote_send_local_bcast */
    "MPIR_Bcast_inter_remote_send_local_bcast",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_allcomm_nb */ "MPIR_Bcast_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Exscan_intra_recursive_doubling */
    "MPIR_Exscan_intra_recursive_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Exscan_allcomm_nb */ "MPIR_Exscan_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gather_intra_binomial */
    "MPIR_Gather_intra_binomial",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gather_inter_linear */ "MPIR_Gather_inter_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gather_inter_local_gather_remote_send */
    "MPIR_Gather_inter_local_gather_remote_send",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gather_allcomm_nb */ "MPIR_Gather_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gatherv_allcomm_linear */
    "MPIR_Gatherv_allcomm_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gatherv_allcomm_nb */ "MPIR_Gatherv_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_tsp_brucks */
    "MPIR_Iallgather_intra_tsp_brucks",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_sched_brucks */
    "MPIR_Iallgather_intra_sched_brucks",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_sched_recursive_doubling */
    "MPIR_Iallgather_intra_sched_recursive_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_sched_ring */
    "MPIR_Iallgather_intra_sched_ring",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_tsp_recexch_doubling */
    "MPIR_Iallgather_intra_tsp_recexch_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_tsp_recexch_halving */
    "MPIR_Iallgather_intra_tsp_recexch_halving",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_tsp_ring */
    "MPIR_Iallgather_intra_tsp_ring",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_inter_sched_local_gather_remote_bcast */
    "MPIR_Iallgather_inter_sched_local_gather_remote_bcast",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_tsp_brucks */
    "MPIR_Iallgatherv_intra_tsp_brucks",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_brucks */
    "MPIR_Iallgatherv_intra_sched_brucks",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_recursive_doubling */
    "MPIR_Iallgatherv_intra_sched_recursive_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_ring */
    "MPIR_Iallgatherv_intra_sched_ring",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_tsp_recexch_doubling */
    "MPIR_Iallgatherv_intra_tsp_recexch_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_tsp_recexch_halving */
    "MPIR_Iallgatherv_intra_tsp_recexch_halving",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_tsp_ring */
    "MPIR_Iallgatherv_intra_tsp_ring",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast */
    "MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_naive */
    "MPIR_Iallreduce_intra_sched_naive",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_recursive_doubling */
    "MPIR_Iallreduce_intra_sched_recursive_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_reduce_scatter_allgather */
    "MPIR_Iallreduce_intra_sched_reduce_scatter_allgather",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_single_buffer */
    "MPIR_Iallreduce_intra_tsp_recexch_single_buffer",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_multiple_buffer */
    "MPIR_Iallreduce_intra_tsp_recexch_multiple_buffer",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_tree */
    "MPIR_Iallreduce_intra_tsp_tree",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_ring */
    "MPIR_Iallreduce_intra_tsp_ring",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_reduce_scatter_recexch_allgatherv */
    "MPIR_Iallreduce_intra_tsp_recexch_reduce_scatter_recexch_allgatherv",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_smp */
    "MPIR_Iallreduce_intra_sched_smp",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_inter_sched_remote_reduce_local_bcast */
    "MPIR_Iallreduce_inter_sched_remote_reduce_local_bcast",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_tsp_ring */
    "MPIR_Ialltoall_intra_tsp_ring",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_tsp_brucks */
    "MPIR_Ialltoall_intra_tsp_brucks",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_tsp_scattered */
    "MPIR_Ialltoall_intra_tsp_scattered",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_brucks */
    "MPIR_Ialltoall_intra_sched_brucks",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_inplace */
    "MPIR_Ialltoall_intra_sched_inplace",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_pairwise */
    "MPIR_Ialltoall_intra_sched_pairwise",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_permuted_sendrecv */
    "MPIR_Ialltoall_intra_sched_permuted_sendrecv",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_inter_sched_pairwise_exchange */
    "MPIR_Ialltoall_inter_sched_pairwise_exchange",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_sched_blocked */
    "MPIR_Ialltoallv_intra_sched_blocked",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_sched_inplace */
    "MPIR_Ialltoallv_intra_sched_inplace",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_tsp_scattered */
    "MPIR_Ialltoallv_intra_tsp_scattered",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_tsp_blocked */
    "MPIR_Ialltoallv_intra_tsp_blocked",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_tsp_inplace */
    "MPIR_Ialltoallv_intra_tsp_inplace",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_inter_sched_pairwise_exchange */
    "MPIR_Ialltoallv_inter_sched_pairwise_exchange",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_tsp_blocked */
    "MPIR_Ialltoallw_intra_tsp_blocked",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_tsp_inplace */
    "MPIR_Ialltoallw_intra_tsp_inplace",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_sched_blocked */
    "MPIR_Ialltoallw_intra_sched_blocked",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_sched_inplace */
    "MPIR_Ialltoallw_intra_sched_inplace",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_inter_sched_pairwise_exchange */
    "MPIR_Ialltoallw_inter_sched_pairwise_exchange",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_sched_recursive_doubling */
    "MPIR_Ibarrier_intra_sched_recursive_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_tsp_recexch */
    "MPIR_Ibarrier_intra_tsp_recexch",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_tsp_k_dissemination */
    "MPIR_Ibarrier_intra_tsp_k_dissemination",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_inter_sched_bcast */
    "MPIR_Ibarrier_inter_sched_bcast",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_tree */
    "MPIR_Ibcast_intra_tsp_tree",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_scatterv_recexch_allgatherv */
    "MPIR_Ibcast_intra_tsp_scatterv_recexch_allgatherv",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_scatterv_ring_allgatherv */
    "MPIR_Ibcast_intra_tsp_scatterv_ring_allgatherv",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_ring */
    "MPIR_Ibcast_intra_tsp_ring",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_binomial */
    "MPIR_Ibcast_intra_sched_binomial",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_scatter_recursive_doubling_allgather */
    "MPIR_Ibcast_intra_sched_scatter_recursive_doubling_allgather",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_scatter_ring_allgather */
    "MPIR_Ibcast_intra_sched_scatter_ring_allgather",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_smp */
    "MPIR_Ibcast_intra_sched_smp",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_inter_sched_flat */
    "MPIR_Ibcast_inter_sched_flat",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iexscan_intra_sched_recursive_doubling */
    "MPIR_Iexscan_intra_sched_recursive_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_intra_tsp_tree */
    "MPIR_Igather_intra_tsp_tree",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_intra_sched_binomial */
    "MPIR_Igather_intra_sched_binomial",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_inter_sched_long */
    "MPIR_Igather_inter_sched_long",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_inter_sched_short */
    "MPIR_Igather_inter_sched_short",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igatherv_allcomm_tsp_linear */
    "MPIR_Igatherv_allcomm_tsp_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igatherv_allcomm_sched_linear */
    "MPIR_Igatherv_allcomm_sched_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_allgather_allcomm_tsp_linear */
    "MPIR_Ineighbor_allgather_allcomm_tsp_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_allgather_allcomm_sched_linear */
    "MPIR_Ineighbor_allgather_allcomm_sched_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_allgatherv_allcomm_tsp_linear */
    "MPIR_Ineighbor_allgatherv_allcomm_tsp_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_allgatherv_allcomm_sched_linear */
    "MPIR_Ineighbor_allgatherv_allcomm_sched_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoall_allcomm_tsp_linear */
    "MPIR_Ineighbor_alltoall_allcomm_tsp_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoall_allcomm_sched_linear */
    "MPIR_Ineighbor_alltoall_allcomm_sched_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallv_allcomm_tsp_linear */
    "MPIR_Ineighbor_alltoallv_allcomm_tsp_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallv_allcomm_sched_linear */
    "MPIR_Ineighbor_alltoallv_allcomm_sched_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallw_allcomm_tsp_linear */
    "MPIR_Ineighbor_alltoallw_allcomm_tsp_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallw_allcomm_sched_linear */
    "MPIR_Ineighbor_alltoallw_allcomm_sched_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_tsp_tree */
    "MPIR_Ireduce_intra_tsp_tree",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_tsp_ring */
    "MPIR_Ireduce_intra_tsp_ring",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_sched_binomial */
    "MPIR_Ireduce_intra_sched_binomial",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_sched_reduce_scatter_gather */
    "MPIR_Ireduce_intra_sched_reduce_scatter_gather",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_sched_smp */
    "MPIR_Ireduce_intra_sched_smp",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_inter_sched_local_reduce_remote_send */
    "MPIR_Ireduce_inter_sched_local_reduce_remote_send",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_noncommutative */
    "MPIR_Ireduce_scatter_intra_sched_noncommutative",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_pairwise */
    "MPIR_Ireduce_scatter_intra_sched_pairwise",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_recursive_doubling */
    "MPIR_Ireduce_scatter_intra_sched_recursive_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_recursive_halving */
    "MPIR_Ireduce_scatter_intra_sched_recursive_halving",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_tsp_recexch */
    "MPIR_Ireduce_scatter_intra_tsp_recexch",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv */
    "MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_intra_tsp_recexch */
    "MPIR_Ireduce_scatter_block_intra_tsp_recexch",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_intra_sched_noncommutative */
    "MPIR_Ireduce_scatter_block_intra_sched_noncommutative",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_intra_sched_pairwise */
    "MPIR_Ireduce_scatter_block_intra_sched_pairwise",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_intra_sched_recursive_doubling */
    "MPIR_Ireduce_scatter_block_intra_sched_recursive_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_intra_sched_recursive_halving */
    "MPIR_Ireduce_scatter_block_intra_sched_recursive_halving",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_inter_sched_remote_reduce_local_scatterv */
    "MPIR_Ireduce_scatter_block_inter_sched_remote_reduce_local_scatterv",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscan_intra_sched_recursive_doubling */
    "MPIR_Iscan_intra_sched_recursive_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscan_intra_sched_smp */
    "MPIR_Iscan_intra_sched_smp",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscan_intra_tsp_recursive_doubling */
    "MPIR_Iscan_intra_tsp_recursive_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_intra_tsp_tree */
    "MPIR_Iscatter_intra_tsp_tree",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_intra_sched_binomial */
    "MPIR_Iscatter_intra_sched_binomial",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_inter_sched_linear */
    "MPIR_Iscatter_inter_sched_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_inter_sched_remote_send_local_scatter */
    "MPIR_Iscatter_inter_sched_remote_send_local_scatter",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatterv_allcomm_tsp_linear */
    "MPIR_Iscatterv_allcomm_tsp_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatterv_allcomm_sched_linear */
    "MPIR_Iscatterv_allcomm_sched_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_allgather_allcomm_nb */
    "MPIR_Neighbor_allgather_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_allgatherv_allcomm_nb */
    "MPIR_Neighbor_allgatherv_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_alltoall_allcomm_nb */
    "MPIR_Neighbor_alltoall_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_alltoallv_allcomm_nb */
    "MPIR_Neighbor_alltoallv_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_alltoallw_allcomm_nb */
    "MPIR_Neighbor_alltoallw_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_intra_binomial */
    "MPIR_Reduce_intra_binomial",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_intra_reduce_scatter_gather */
    "MPIR_Reduce_intra_reduce_scatter_gather",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_intra_smp */ "MPIR_Reduce_intra_smp",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_inter_local_reduce_remote_send */
    "MPIR_Reduce_inter_local_reduce_remote_send",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_allcomm_nb */ "MPIR_Reduce_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_intra_noncommutative */
    "MPIR_Reduce_scatter_intra_noncommutative",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_intra_pairwise */
    "MPIR_Reduce_scatter_intra_pairwise",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_intra_recursive_doubling */
    "MPIR_Reduce_scatter_intra_recursive_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_intra_recursive_halving */
    "MPIR_Reduce_scatter_intra_recursive_halving",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_inter_remote_reduce_local_scatter */
    "MPIR_Reduce_scatter_inter_remote_reduce_local_scatter",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_allcomm_nb */
    "MPIR_Reduce_scatter_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_intra_noncommutative */
    "MPIR_Reduce_scatter_block_intra_noncommutative",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_intra_pairwise */
    "MPIR_Reduce_scatter_block_intra_pairwise",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_intra_recursive_doubling */
    "MPIR_Reduce_scatter_block_intra_recursive_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_intra_recursive_halving */
    "MPIR_Reduce_scatter_block_intra_recursive_halving",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_inter_remote_reduce_local_scatter */
    "MPIR_Reduce_scatter_block_inter_remote_reduce_local_scatter",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_allcomm_nb */
    "MPIR_Reduce_scatter_block_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scan_intra_recursive_doubling */
    "MPIR_Scan_intra_recursive_doubling",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scan_intra_smp */ "MPIR_Scan_intra_smp",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scan_allcomm_nb */ "MPIR_Scan_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatter_intra_binomial */
    "MPIR_Scatter_intra_binomial",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatter_inter_linear */
    "MPIR_Scatter_inter_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatter_inter_remote_send_local_scatter */
    "MPIR_Scatter_inter_remote_send_local_scatter",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatter_allcomm_nb */ "MPIR_Scatter_allcomm_nb",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatterv_allcomm_linear */
    "MPIR_Scatterv_allcomm_linear",
    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatterv_allcomm_nb */ "MPIR_Scatterv_allcomm_nb",

    /* MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_bcast_release_gather */
    "MPIDI_POSIX_mpi_bcast_release_gather",
    /* MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_bcast_ipc_read */
    "MPIDI_POSIX_mpi_bcast_ipc_read",
    /* MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_impl */ "MPIR_Bcast_impl",
    /* MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_barrier_release_gather */
    "MPIDI_POSIX_mpi_barrier_release_gather",
    /* MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_impl */ "MPIR_Barrier_impl",
    /* MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_allreduce_release_gather */
    "MPIDI_POSIX_mpi_allreduce_release_gather",
    /* MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_impl */ "MPIR_Allreduce_impl",
    /* MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_reduce_release_gather */
    "MPIDI_POSIX_mpi_reduce_release_gather",
    /* MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_impl */ "MPIR_Reduce_impl",

    /* MPII_CSEL_CONTAINER_TYPE__ALGORITHM__Algorithm_count */ "END_OF_MPIR_ALGO"
};
