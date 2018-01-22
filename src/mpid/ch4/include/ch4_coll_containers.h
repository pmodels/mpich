#ifndef CH4_COLL_CONTAINERS_H_INCLUDED
#define CH4_COLL_CONTAINERS_H_INCLUDED

/* Barrier CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Barrier_intra_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Barrier_intra_composition_beta_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Barrier_intra_composition_gamma_cnt;
#endif /*  MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Barrier_inter_composition_alpha_cnt;

/* Bcast  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Bcast_intra_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Bcast_intra_composition_beta_cnt;
extern const MPIDI_coll_algo_container_t CH4_Bcast_intra_composition_gamma_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Bcast_intra_composition_delta_cnt;
#endif /*  MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Bcast_inter_composition_alpha_cnt;

/* Reduce  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Reduce_intra_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Reduce_intra_composition_beta_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Reduce_intra_composition_gamma_cnt;
#endif /*  MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Reduce_inter_composition_alpha_cnt;

/* Allreduce  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Allreduce_intra_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Allreduce_intra_composition_beta_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Allreduce_intra_composition_gamma_cnt;
#endif /*  MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Allreduce_inter_composition_alpha_cnt;

/* Alltoall  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Alltoall_intra_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Alltoall_intra_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Alltoall_inter_composition_alpha_cnt;

/* Alltoallv  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Alltoallv_intra_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Alltoallv_intra_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Alltoallv_inter_composition_alpha_cnt;

/* Alltoallw  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Alltoallw_intra_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Alltoallw_intra_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Alltoallw_inter_composition_alpha_cnt;

/* Allgather  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Allgather_intra_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Allgather_intra_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Allgather_inter_composition_alpha_cnt;

/* Allgatherv  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Allgatherv_intra_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Allgatherv_intra_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Allgatherv_inter_composition_alpha_cnt;

/* Gather  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Gather_intra_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Gather_intra_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Gather_inter_composition_alpha_cnt;

/* Gatherv  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Gatherv_intra_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Gatherv_intra_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Gatherv_inter_composition_alpha_cnt;

/* Scatter CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Scatter_intra_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Scatter_intra_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Scatter_inter_composition_alpha_cnt;

/* Scatterv CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Scatterv_intra_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Scatterv_intra_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Scatterv_inter_composition_alpha_cnt;

/* Reduce_scatter CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_intra_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_intra_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_inter_composition_alpha_cnt;

/* Reduce_scatter_block CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_block_intra_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_block_intra_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_block_inter_composition_alpha_cnt;

/* Scan CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Scan_intra_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Scan_intra_composition_beta_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Scan_intra_composition_gamma_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */

/* Exscan CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Exscan_intra_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Exscan_intra_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */

#endif /* CH4_COLL_CONTAINERS_H_INCLUDED */
