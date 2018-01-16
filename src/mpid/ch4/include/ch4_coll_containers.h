#ifndef CH4_COLL_CONTAINERS_H_INCLUDED
#define CH4_COLL_CONTAINERS_H_INCLUDED

/* Barrier CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Barrier_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Barrier_composition_beta_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Barrier_composition_gamma_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Barrier_intercomm_cnt;

/* Bcast  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Bcast_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Bcast_composition_beta_cnt;
extern const MPIDI_coll_algo_container_t CH4_Bcast_composition_gamma_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Bcast_composition_delta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Bcast_intercomm_cnt;

/* Reduce  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Reduce_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Reduce_composition_beta_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Reduce_composition_gamma_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Reduce_intercomm_cnt;

/* Allreduce  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Allreduce_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Allreduce_composition_beta_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Allreduce_composition_gamma_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Allreduce_intercomm_cnt;

/* Alltoall  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Alltoall_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Alltoall_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Alltoall_intercomm_cnt;

/* Alltoallv  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Alltoallv_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Alltoallv_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Alltoallv_intercomm_cnt;

/* Alltoallw  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Alltoallw_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Alltoallw_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Alltoallw_intercomm_cnt;

/* Allgather  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Allgather_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Allgather_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Allgather_intercomm_cnt;

/* Allgatherv  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Allgatherv_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Allgatherv_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Allgatherv_intercomm_cnt;

/* Gather  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Gather_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Gather_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Gather_intercomm_cnt;

/* Gatherv  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Gatherv_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Gatherv_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Gatherv_intercomm_cnt;

/* Scatter CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Scatter_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Scatter_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Scatter_intercomm_cnt;

/* Scatterv CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Scatterv_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Scatterv_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Scatterv_intercomm_cnt;

/* Reduce_scatter CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_intercomm_cnt;

/* Reduce_scatter_block CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_block_composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_block_composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_block_intercomm_cnt;

#endif /* CH4_COLL_CONTAINERS_H_INCLUDED */
