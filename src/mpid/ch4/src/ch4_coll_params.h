#ifndef CH4_COLL_PARAMS_H_INCLUDED
#define CH4_COLL_PARAMS_H_INCLUDED

typedef struct {
    enum {
        MPIDI_Barrier_intra_composition_alpha_id,
        MPIDI_Barrier_intra_composition_beta_id,
        MPIDI_Bcast_intra_composition_alpha_id,
        MPIDI_Bcast_intra_composition_beta_id,
        MPIDI_Bcast_intra_composition_gamma_id,
        MPIDI_Reduce_intra_composition_alpha_id,
        MPIDI_Reduce_intra_composition_beta_id,
        MPIDI_Reduce_intra_composition_gamma_id,
        MPIDI_Allreduce_intra_composition_alpha_id,
        MPIDI_Allreduce_intra_composition_beta_id,
        MPIDI_Allreduce_intra_composition_gamma_id,
        MPIDI_Alltoall_intra_composition_alpha_id,
        MPIDI_Alltoallv_intra_composition_alpha_id,
        MPIDI_Alltoallw_intra_composition_alpha_id,
        MPIDI_Allgather_intra_composition_alpha_id,
        MPIDI_Allgatherv_intra_composition_alpha_id,
        MPIDI_Gather_intra_composition_alpha_id,
        MPIDI_Gatherv_intra_composition_alpha_id,
        MPIDI_Scatter_intra_composition_alpha_id,
        MPIDI_Scatterv_intra_composition_alpha_id,
        MPIDI_Reduce_scatter_intra_composition_alpha_id,
        MPIDI_Reduce_scatter_block_intra_composition_alpha_id,
        MPIDI_Scan_intra_composition_alpha_id,
        MPIDI_Scan_intra_composition_beta_id,
        MPIDI_Exscan_intra_composition_alpha_id,
    } id;
} MPIDI_coll_algo_container_t;

#endif /* CH4_COLL_PARAMS_H_INCLUDED */
