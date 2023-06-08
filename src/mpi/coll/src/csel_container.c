/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "coll_impl.h"
#include "csel_container.h"
#include "mpl.h"
#include "csel_json.h"

void *MPII_Create_container(const char *ckey, struct json_stream *json_stream);

static int get_container_id(const char *ckey)
{
    if (!strcmp(ckey, "algorithm=MPIR_Allgather_intra_brucks"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_brucks;
    else if (!strcmp(ckey, "algorithm=MPIR_Allgather_intra_k_brucks"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_k_brucks;
    else if (!strcmp(ckey, "algorithm=MPIR_Allgather_intra_recursive_doubling"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_recursive_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Allgather_intra_ring"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_ring;
    else if (!strcmp(ckey, "algorithm=MPIR_Allgather_inter_local_gather_remote_bcast"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_inter_local_gather_remote_bcast;
    else if (!strcmp(ckey, "algorithm=MPIR_Allgather_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Allgatherv_intra_brucks"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_intra_brucks;
    else if (!strcmp(ckey, "algorithm=MPIR_Allgatherv_intra_recursive_doubling"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_intra_recursive_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Allgatherv_intra_ring"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_intra_ring;
    else if (!strcmp(ckey, "algorithm=MPIR_Allgatherv_inter_remote_gather_local_bcast"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_inter_remote_gather_local_bcast;
    else if (!strcmp(ckey, "algorithm=MPIR_Allgatherv_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Allreduce_intra_recursive_doubling"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_recursive_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Allreduce_intra_reduce_scatter_allgather"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_reduce_scatter_allgather;
    else if (!strcmp(ckey, "algorithm=MPIR_Allreduce_intra_smp"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_smp;
    else if (!strcmp(ckey, "algorithm=MPIR_Allreduce_intra_tree"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_tree;
    else if (!strcmp(ckey, "algorithm=MPIR_Allreduce_intra_recexch"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_recexch;
    else if (!strcmp(ckey, "algorithm=MPIR_Allreduce_intra_ring"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_ring;
    else if (!strcmp(ckey, "algorithm=MPIR_Allreduce_intra_k_reduce_scatter_allgather"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_k_reduce_scatter_allgather;
    else if (!strcmp(ckey, "algorithm=MPIR_Allreduce_inter_reduce_exchange_bcast"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_inter_reduce_exchange_bcast;
    else if (!strcmp(ckey, "algorithm=MPIR_Allreduce_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Alltoall_intra_brucks"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_brucks;
    else if (!strcmp(ckey, "algorithm=MPIR_Alltoall_intra_k_brucks"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_k_brucks;
    else if (!strcmp(ckey, "algorithm=MPIR_Alltoall_intra_pairwise"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_pairwise;
    else if (!strcmp(ckey, "algorithm=MPIR_Alltoall_intra_pairwise_sendrecv_replace"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_pairwise_sendrecv_replace;
    else if (!strcmp(ckey, "algorithm=MPIR_Alltoall_intra_scattered"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_scattered;
    else if (!strcmp(ckey, "algorithm=MPIR_Alltoall_inter_pairwise_exchange"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_inter_pairwise_exchange;
    else if (!strcmp(ckey, "algorithm=MPIR_Alltoall_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Alltoallv_intra_pairwise_sendrecv_replace"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallv_intra_pairwise_sendrecv_replace;
    else if (!strcmp(ckey, "algorithm=MPIR_Alltoallv_intra_scattered"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallv_intra_scattered;
    else if (!strcmp(ckey, "algorithm=MPIR_Alltoallv_inter_pairwise_exchange"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallv_inter_pairwise_exchange;
    else if (!strcmp(ckey, "algorithm=MPIR_Alltoallv_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallv_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Alltoallw_intra_pairwise_sendrecv_replace"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallw_intra_pairwise_sendrecv_replace;
    else if (!strcmp(ckey, "algorithm=MPIR_Alltoallw_intra_scattered"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallw_intra_scattered;
    else if (!strcmp(ckey, "algorithm=MPIR_Alltoallw_inter_pairwise_exchange"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallw_inter_pairwise_exchange;
    else if (!strcmp(ckey, "algorithm=MPIR_Alltoallw_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoallw_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Barrier_intra_k_dissemination"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_intra_k_dissemination;
    else if (!strcmp(ckey, "algorithm=MPIR_Barrier_intra_recexch"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_intra_recexch;
    else if (!strcmp(ckey, "algorithm=MPIR_Barrier_intra_smp"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_intra_smp;
    else if (!strcmp(ckey, "algorithm=MPIR_Barrier_inter_bcast"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_inter_bcast;
    else if (!strcmp(ckey, "algorithm=MPIR_Barrier_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Bcast_intra_binomial"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_binomial;
    else if (!strcmp(ckey, "algorithm=MPIR_Bcast_intra_scatter_recursive_doubling_allgather"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_scatter_recursive_doubling_allgather;
    else if (!strcmp(ckey, "algorithm=MPIR_Bcast_intra_scatter_ring_allgather"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_scatter_ring_allgather;
    else if (!strcmp(ckey, "algorithm=MPIR_Bcast_intra_smp"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_smp;
    else if (!strcmp(ckey, "algorithm=MPIR_Bcast_intra_tree"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_tree;
    else if (!strcmp(ckey, "algorithm=MPIR_Bcast_intra_pipelined_tree"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_pipelined_tree;
    else if (!strcmp(ckey, "algorithm=MPIR_Bcast_inter_remote_send_local_bcast"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_inter_remote_send_local_bcast;
    else if (!strcmp(ckey, "algorithm=MPIR_Bcast_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Exscan_intra_recursive_doubling"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Exscan_intra_recursive_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Exscan_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Exscan_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Gather_intra_binomial"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gather_intra_binomial;
    else if (!strcmp(ckey, "algorithm=MPIR_Gather_inter_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gather_inter_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Gather_inter_local_gather_remote_send"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gather_inter_local_gather_remote_send;
    else if (!strcmp(ckey, "algorithm=MPIR_Gather_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gather_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Gatherv_allcomm_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gatherv_allcomm_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Gatherv_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Gatherv_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallgather_intra_tsp_brucks"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_tsp_brucks;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallgather_intra_sched_brucks"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_sched_brucks;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallgather_intra_sched_recursive_doubling"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_sched_recursive_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallgather_intra_sched_ring"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_sched_ring;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallgather_intra_tsp_recexch_doubling"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_tsp_recexch_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallgather_intra_tsp_recexch_halving"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_tsp_recexch_halving;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallgather_intra_tsp_ring"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_tsp_ring;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallgather_inter_sched_local_gather_remote_bcast"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_inter_sched_local_gather_remote_bcast;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallgatherv_intra_tsp_brucks"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_tsp_brucks;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallgatherv_intra_sched_brucks"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_brucks;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallgatherv_intra_sched_recursive_doubling"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_recursive_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallgatherv_intra_sched_ring"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_ring;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallgatherv_intra_tsp_recexch_doubling"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_tsp_recexch_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallgatherv_intra_tsp_recexch_halving"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_tsp_recexch_halving;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallgatherv_intra_tsp_ring"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_tsp_ring;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallreduce_intra_sched_naive"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_naive;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallreduce_intra_sched_recursive_doubling"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_recursive_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallreduce_intra_sched_reduce_scatter_allgather"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_reduce_scatter_allgather;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallreduce_intra_tsp_recexch_single_buffer"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_single_buffer;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallreduce_intra_tsp_recexch_multiple_buffer"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_multiple_buffer;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallreduce_intra_tsp_tree"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_tree;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallreduce_intra_tsp_ring"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_ring;
    else if (!strcmp
             (ckey,
              "algorithm=MPIR_Iallreduce_intra_tsp_recexch_reduce_scatter_recexch_allgatherv"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_reduce_scatter_recexch_allgatherv;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallreduce_intra_sched_smp"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_smp;
    else if (!strcmp(ckey, "algorithm=MPIR_Iallreduce_inter_sched_remote_reduce_local_bcast"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_inter_sched_remote_reduce_local_bcast;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoall_intra_tsp_ring"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_tsp_ring;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoall_intra_tsp_brucks"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_tsp_brucks;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoall_intra_tsp_scattered"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_tsp_scattered;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoall_intra_sched_brucks"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_brucks;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoall_intra_sched_inplace"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_inplace;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoall_intra_sched_pairwise"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_pairwise;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoall_intra_sched_permuted_sendrecv"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_permuted_sendrecv;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoall_inter_sched_pairwise_exchange"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_inter_sched_pairwise_exchange;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoallv_intra_sched_blocked"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_sched_blocked;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoallv_intra_sched_inplace"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_sched_inplace;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoallv_intra_tsp_scattered"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_tsp_scattered;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoallv_intra_tsp_blocked"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_tsp_blocked;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoallv_intra_tsp_inplace"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_tsp_inplace;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoallv_inter_sched_pairwise_exchange"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_inter_sched_pairwise_exchange;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoallw_intra_tsp_blocked"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_tsp_blocked;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoallw_intra_tsp_inplace"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_tsp_inplace;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoallw_intra_sched_blocked"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_sched_blocked;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoallw_intra_sched_inplace"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_intra_sched_inplace;
    else if (!strcmp(ckey, "algorithm=MPIR_Ialltoallw_inter_sched_pairwise_exchange"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallw_inter_sched_pairwise_exchange;
    else if (!strcmp(ckey, "algorithm=MPIR_Ibarrier_intra_sched_recursive_doubling"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_sched_recursive_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Ibarrier_intra_tsp_recexch"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_tsp_recexch;
    else if (!strcmp(ckey, "algorithm=MPIR_Ibarrier_intra_tsp_k_dissem"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_tsp_k_dissemination;
    else if (!strcmp(ckey, "algorithm=MPIR_Ibarrier_inter_sched_bcast"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_inter_sched_bcast;
    else if (!strcmp(ckey, "algorithm=MPIR_Ibcast_intra_tsp_tree"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_tree;
    else if (!strcmp(ckey, "algorithm=MPIR_Ibcast_intra_tsp_scatterv_recexch_allgatherv"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_scatterv_recexch_allgatherv;
    else if (!strcmp(ckey, "algorithm=MPIR_Ibcast_intra_tsp_ring"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_ring;
    else if (!strcmp(ckey, "algorithm=MPIR_Ibcast_intra_sched_binomial"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_binomial;
    else if (!strcmp
             (ckey, "algorithm=MPIR_Ibcast_intra_sched_scatter_recursive_doubling_allgather"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_scatter_recursive_doubling_allgather;
    else if (!strcmp(ckey, "algorithm=MPIR_Ibcast_intra_sched_scatter_ring_allgather"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_scatter_ring_allgather;
    else if (!strcmp(ckey, "algorithm=MPIR_Ibcast_intra_sched_smp"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_smp;
    else if (!strcmp(ckey, "algorithm=MPIR_Ibcast_inter_sched_flat"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_inter_sched_flat;
    else if (!strcmp(ckey, "algorithm=MPIR_Iexscan_intra_sched_recursive_doubling"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iexscan_intra_sched_recursive_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Igather_intra_tsp_tree"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_intra_tsp_tree;
    else if (!strcmp(ckey, "algorithm=MPIR_Igather_intra_sched_binomial"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_intra_sched_binomial;
    else if (!strcmp(ckey, "algorithm=MPIR_Igather_inter_sched_long"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_inter_sched_long;
    else if (!strcmp(ckey, "algorithm=MPIR_Igather_inter_sched_short"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_inter_sched_short;
    else if (!strcmp(ckey, "algorithm=MPIR_Igatherv_allcomm_tsp_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igatherv_allcomm_tsp_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Igatherv_allcomm_sched_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igatherv_allcomm_sched_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Ineighbor_allgather_allcomm_tsp_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_allgather_allcomm_tsp_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Ineighbor_allgather_allcomm_sched_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_allgather_allcomm_sched_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Ineighbor_allgatherv_allcomm_tsp_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_allgatherv_allcomm_tsp_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Ineighbor_allgatherv_allcomm_sched_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_allgatherv_allcomm_sched_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Ineighbor_alltoall_allcomm_tsp_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoall_allcomm_tsp_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Ineighbor_alltoall_allcomm_sched_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoall_allcomm_sched_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Ineighbor_alltoallv_allcomm_tsp_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallv_allcomm_tsp_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Ineighbor_alltoallv_allcomm_sched_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallv_allcomm_sched_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Ineighbor_alltoallw_allcomm_tsp_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallw_allcomm_tsp_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Ineighbor_alltoallw_allcomm_sched_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallw_allcomm_sched_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Ireduce_intra_tsp_tree"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_tsp_tree;
    else if (!strcmp(ckey, "algorithm=MPIR_Ireduce_intra_tsp_ring"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_tsp_ring;
    else if (!strcmp(ckey, "algorithm=MPIR_Ireduce_intra_sched_binomial"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_sched_binomial;
    else if (!strcmp(ckey, "algorithm=MPIR_Ireduce_intra_sched_reduce_scatter_gather"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_sched_reduce_scatter_gather;
    else if (!strcmp(ckey, "algorithm=MPIR_Ireduce_intra_sched_smp"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_sched_smp;
    else if (!strcmp(ckey, "algorithm=MPIR_Ireduce_inter_sched_local_reduce_remote_send"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_inter_sched_local_reduce_remote_send;
    else if (!strcmp(ckey, "algorithm=MPIR_Ireduce_scatter_intra_sched_noncommutative"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_noncommutative;
    else if (!strcmp(ckey, "algorithm=MPIR_Ireduce_scatter_intra_sched_pairwise"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_pairwise;
    else if (!strcmp(ckey, "algorithm=MPIR_Ireduce_scatter_intra_sched_recursive_doubling"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_recursive_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Ireduce_scatter_intra_sched_recursive_halving"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_recursive_halving;
    else if (!strcmp(ckey, "algorithm=MPIR_Ireduce_scatter_intra_tsp_recexch"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_tsp_recexch;
    else if (!strcmp
             (ckey, "algorithm=MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv;
    else if (!strcmp(ckey, "algorithm=MPIR_Ireduce_scatter_block_intra_tsp_recexch"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_intra_tsp_recexch;
    else if (!strcmp(ckey, "algorithm=MPIR_Ireduce_scatter_block_intra_sched_noncommutative"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_intra_sched_noncommutative;
    else if (!strcmp(ckey, "algorithm=MPIR_Ireduce_scatter_block_intra_sched_pairwise"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_intra_sched_pairwise;
    else if (!strcmp(ckey, "algorithm=MPIR_Ireduce_scatter_block_intra_sched_recursive_doubling"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_intra_sched_recursive_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Ireduce_scatter_block_intra_sched_recursive_halving"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_intra_sched_recursive_halving;
    else if (!strcmp
             (ckey,
              "algorithm=MPIR_Ireduce_scatter_block_inter_sched_remote_reduce_local_scatterv"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_block_inter_sched_remote_reduce_local_scatterv;
    else if (!strcmp(ckey, "algorithm=MPIR_Iscan_intra_sched_recursive_doubling"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscan_intra_sched_recursive_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Iscan_intra_sched_smp"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscan_intra_sched_smp;
    else if (!strcmp(ckey, "algorithm=MPIR_Iscan_intra_tsp_recursive_doubling"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscan_intra_tsp_recursive_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Iscatter_intra_tsp_tree"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_intra_tsp_tree;
    else if (!strcmp(ckey, "algorithm=MPIR_Iscatter_intra_sched_binomial"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_intra_sched_binomial;
    else if (!strcmp(ckey, "algorithm=MPIR_Iscatter_inter_sched_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_inter_sched_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Iscatter_inter_sched_remote_send_local_scatter"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_inter_sched_remote_send_local_scatter;
    else if (!strcmp(ckey, "algorithm=MPIR_Iscatterv_allcomm_tsp_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatterv_allcomm_tsp_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Iscatterv_allcomm_sched_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatterv_allcomm_sched_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Neighbor_allgather_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_allgather_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Neighbor_allgatherv_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_allgatherv_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Neighbor_alltoall_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_alltoall_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Neighbor_alltoallv_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_alltoallv_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Neighbor_alltoallw_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_alltoallw_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_intra_binomial"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_intra_binomial;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_intra_reduce_scatter_gather"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_intra_reduce_scatter_gather;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_intra_smp"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_intra_smp;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_inter_local_reduce_remote_send"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_inter_local_reduce_remote_send;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_scatter_intra_noncommutative"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_intra_noncommutative;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_scatter_intra_pairwise"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_intra_pairwise;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_scatter_intra_recursive_doubling"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_intra_recursive_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_scatter_intra_recursive_halving"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_intra_recursive_halving;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_scatter_inter_remote_reduce_local_scatter"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_inter_remote_reduce_local_scatter;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_scatter_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_scatter_block_intra_noncommutative"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_intra_noncommutative;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_scatter_block_intra_pairwise"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_intra_pairwise;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_scatter_block_intra_recursive_doubling"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_intra_recursive_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_scatter_block_intra_recursive_halving"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_intra_recursive_halving;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_scatter_block_inter_remote_reduce_local_scatter"))
        return
            MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_inter_remote_reduce_local_scatter;
    else if (!strcmp(ckey, "algorithm=MPIR_Reduce_scatter_block_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_scatter_block_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Scan_intra_recursive_doubling"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scan_intra_recursive_doubling;
    else if (!strcmp(ckey, "algorithm=MPIR_Scan_intra_smp"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scan_intra_smp;
    else if (!strcmp(ckey, "algorithm=MPIR_Scan_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scan_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Scatter_intra_binomial"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatter_intra_binomial;
    else if (!strcmp(ckey, "algorithm=MPIR_Scatter_inter_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatter_inter_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Scatter_inter_remote_send_local_scatter"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatter_inter_remote_send_local_scatter;
    else if (!strcmp(ckey, "algorithm=MPIR_Scatter_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatter_allcomm_nb;
    else if (!strcmp(ckey, "algorithm=MPIR_Scatterv_allcomm_linear"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatterv_allcomm_linear;
    else if (!strcmp(ckey, "algorithm=MPIR_Scatterv_allcomm_nb"))
        return MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatterv_allcomm_nb;
    else {
        fprintf(stderr, "unrecognized key %s\n", ckey);
        MPIR_Assert(0);
        return -1;
    }
}

static void parse_container_param(const char *ckey, MPII_Csel_container_s * cnt)
{
    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_tree:
            {
                if (!strncmp(ckey, "chunk_size=", strlen("chunk_size=")))
                    cnt->u.ibcast.intra_tsp_tree.chunk_size = atoi(ckey + strlen("chunk_size="));
                else if (!strncmp(ckey, "tree_type=", strlen("tree_type=")))
                    cnt->u.ibcast.intra_tsp_tree.tree_type = atoi(ckey + strlen("tree_type="));
                else if (!strncmp(ckey, "k=", strlen("k=")))
                    cnt->u.ibcast.intra_tsp_tree.k = atoi(ckey + strlen("k="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_ring:
            {
                if (!strncmp(ckey, "chunk_size=", strlen("chunk_size=")))
                    cnt->u.ibcast.intra_tsp_ring.chunk_size = atoi(ckey + strlen("chunk_size="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_tree:
            {
                if (!strncmp(ckey, "tree_type=", strlen("tree_type=")))
                    cnt->u.bcast.intra_tree.tree_type = atoi(ckey + strlen("tree_type="));
                else if (!strncmp(ckey, "k=", strlen("k=")))
                    cnt->u.bcast.intra_tree.k = atoi(ckey + strlen("k="));
                else if (!strncmp(ckey, "is_non_blocking=", strlen("is_non_blocking=")))
                    cnt->u.bcast.intra_tree.k = atoi(ckey + strlen("k="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_pipelined_tree:
            {
                if (!strncmp(ckey, "tree_type=", strlen("tree_type=")))
                    cnt->u.bcast.intra_pipelined_tree.tree_type = atoi(ckey + strlen("tree_type="));
                else if (!strncmp(ckey, "k=", strlen("k=")))
                    cnt->u.bcast.intra_pipelined_tree.k = atoi(ckey + strlen("k="));
                else if (!strncmp(ckey, "is_non_blocking=", strlen("is_non_blocking=")))
                    cnt->u.bcast.intra_pipelined_tree.k = atoi(ckey + strlen("k="));
                else if (!strncmp(ckey, "chunk_size=", strlen("chunk_size=")))
                    cnt->u.bcast.intra_pipelined_tree.chunk_size =
                        atoi(ckey + strlen("chunk_size="));
                else if (!strncmp(ckey, "recv_pre_posted=", strlen("recv_pre_posted=")))
                    cnt->u.bcast.intra_pipelined_tree.recv_pre_posted =
                        atoi(ckey + strlen("recv_pre_posted="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_tsp_tree:
            {
                if (!strncmp(ckey, "buffer_per_child=", strlen("buffer_per_child=")))
                    cnt->u.ireduce.intra_tsp_tree.buffer_per_child =
                        atoi(ckey + strlen("buffer_per_child="));
                else if (!strncmp(ckey, "k=", strlen("k=")))
                    cnt->u.ireduce.intra_tsp_tree.k = atoi(ckey + strlen("k="));
                else if (!strncmp(ckey, "tree_type=", strlen("tree_type=")))
                    cnt->u.ireduce.intra_tsp_tree.tree_type = atoi(ckey + strlen("tree_type="));
                else if (!strncmp(ckey, "chunk_size=", strlen("chunk_size=")))
                    cnt->u.ireduce.intra_tsp_tree.chunk_size = atoi(ckey + strlen("chunk_size="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_tsp_ring:
            {
                if (!strncmp(ckey, "buffer_per_child=", strlen("buffer_per_child=")))
                    cnt->u.ireduce.intra_tsp_ring.buffer_per_child =
                        atoi(ckey + strlen("buffer_per_child="));
                else if (!strncmp(ckey, "chunk_size=", strlen("chunk_size=")))
                    cnt->u.ireduce.intra_tsp_tree.chunk_size = atoi(ckey + strlen("chunk_size="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_tree:
            {
                if (!strncmp(ckey, "buffer_per_child=", strlen("buffer_per_child=")))
                    cnt->u.iallreduce.intra_tsp_tree.buffer_per_child =
                        atoi(ckey + strlen("buffer_per_child="));
                else if (!strncmp(ckey, "k=", strlen("k=")))
                    cnt->u.iallreduce.intra_tsp_tree.k = atoi(ckey + strlen("k="));
                else if (!strncmp(ckey, "tree_type=", strlen("tree_type=")))
                    cnt->u.iallreduce.intra_tsp_tree.tree_type = atoi(ckey + strlen("tree_type="));
                else if (!strncmp(ckey, "chunk_size=", strlen("chunk_size=")))
                    cnt->u.iallreduce.intra_tsp_tree.chunk_size =
                        atoi(ckey + strlen("chunk_size="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_tree:
            {
                if (!strncmp(ckey, "buffer_per_child=", strlen("buffer_per_child=")))
                    cnt->u.allreduce.intra_tree.buffer_per_child =
                        atoi(ckey + strlen("buffer_per_child="));
                else if (!strncmp(ckey, "k=", strlen("k=")))
                    cnt->u.allreduce.intra_tree.k = atoi(ckey + strlen("k="));
                else if (!strncmp(ckey, "tree_type=", strlen("tree_type=")))
                    cnt->u.allreduce.intra_tree.tree_type = atoi(ckey + strlen("tree_type="));
                else if (!strncmp(ckey, "chunk_size=", strlen("chunk_size=")))
                    cnt->u.allreduce.intra_tree.chunk_size = atoi(ckey + strlen("chunk_size="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_recexch:
            {
                if (!strncmp(ckey, "k=", strlen("k=")))
                    cnt->u.allreduce.intra_recexch.k = atoi(ckey + strlen("k="));
                else if (!strncmp(ckey, "single_phase_recv=", strlen("single_phase_recv=")))
                    cnt->u.allreduce.intra_recexch.single_phase_recv =
                        atoi(ckey + strlen("single_phase_recv="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_k_reduce_scatter_allgather:
            {
                if (!strncmp(ckey, "k=", strlen("k=")))
                    cnt->u.allreduce.intra_k_reduce_scatter_allgather.k = atoi(ckey + strlen("k="));
                else if (!strncmp(ckey, "single_phase_recv=", strlen("single_phase_recv=")))
                    cnt->u.allreduce.intra_k_reduce_scatter_allgather.single_phase_recv =
                        atoi(ckey + strlen("single_phase_recv="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_scatterv_recexch_allgatherv:
            {
                if (!strncmp(ckey, "scatterv_k=", strlen("scatterv_k=")))
                    cnt->u.ibcast.intra_tsp_scatterv_recexch_allgatherv.scatterv_k =
                        atoi(ckey + strlen("scatterv_k="));
                else if (!strncmp(ckey, "allgatherv_k=", strlen("allgatherv_k=")))
                    cnt->u.ibcast.intra_tsp_scatterv_recexch_allgatherv.allgatherv_k =
                        atoi(ckey + strlen("allgatherv_k="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_single_buffer:
            {
                if (!strncmp(ckey, "k=", strlen("k=")))
                    cnt->u.iallreduce.intra_tsp_recexch_single_buffer.k = atoi(ckey + strlen("k="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_multiple_buffer:
            {
                if (!strncmp(ckey, "k=", strlen("k=")))
                    cnt->u.iallreduce.intra_tsp_recexch_multiple_buffer.k =
                        atoi(ckey + strlen("k="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_reduce_scatter_recexch_allgatherv:
            {
                if (!strncmp(ckey, "k=", strlen("k=")))
                    cnt->u.iallreduce.intra_tsp_recexch_reduce_scatter_recexch_allgatherv.k =
                        atoi(ckey + strlen("k="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_k_brucks:
            {
                if (!strncmp(ckey, "k=", strlen("k=")))
                    cnt->u.allgather.intra_k_brucks.k = atoi(ckey + strlen("k="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_k_brucks:
            {
                if (!strncmp(ckey, "k=", strlen("k=")))
                    cnt->u.alltoall.intra_k_brucks.k = atoi(ckey + strlen("k="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_intra_k_dissemination:
            {
                if (!strncmp(ckey, "k=", strlen("k=")))
                    cnt->u.barrier.intra_k_dissemination.k = atoi(ckey + strlen("k="));
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_intra_recexch:
            {
                if (!strncmp(ckey, "k=", strlen("k=")))
                    cnt->u.barrier.intra_recexch.k = atoi(ckey + strlen("k="));
            }
            break;

        default:
            /* Algorithm does not have parameters */
            break;
    }
}

static void parse_container_params(struct json_stream *json_stream, MPII_Csel_container_s * cnt)
{
    char *ckey;

    JSON_FOREACH_START(json_stream);
    JSON_FOREACH(json_stream, ckey) {
        parse_container_param(ckey, cnt);
        MPL_free(ckey);
        json_skip_object(json_stream);
    }
    JSON_FOREACH_WRAP(json_stream);
}

void *MPII_Create_container(const char *ckey, struct json_stream *json_stream)
{
    MPII_Csel_container_s *cnt = MPL_malloc(sizeof(MPII_Csel_container_s), MPL_MEM_COLL);
    cnt->id = get_container_id(ckey);

    /* process algorithm parameters */
    parse_container_params(json_stream, cnt);

    return (void *) cnt;
}
