/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_CSEL_CONTAINER_H_INCLUDED
#define CH4_CSEL_CONTAINER_H_INCLUDED

typedef struct {
    enum {
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Barrier_intra_composition_alpha = 0,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Barrier_intra_composition_beta,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Bcast_intra_composition_alpha,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Bcast_intra_composition_beta,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Bcast_intra_composition_gamma,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_intra_composition_alpha,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_intra_composition_beta,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_intra_composition_gamma,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_alpha,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_beta,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_gamma,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_delta,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoall_intra_composition_alpha,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoall_intra_composition_beta,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoallv_intra_composition_alpha,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoallw_intra_composition_alpha,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allgather_intra_composition_alpha,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allgather_intra_composition_beta,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allgatherv_intra_composition_alpha,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Gather_intra_composition_alpha,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Gatherv_intra_composition_alpha,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Scatter_intra_composition_alpha,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Scatterv_intra_composition_alpha,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_scatter_intra_composition_alpha,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_scatter_block_intra_composition_alpha,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Scan_intra_composition_alpha,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Scan_intra_composition_beta,
        MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Exscan_intra_composition_alpha,
    } id;
} MPIDI_Csel_container_s;

#endif /* CH4_CSEL_CONTAINER_H_INCLUDED */
