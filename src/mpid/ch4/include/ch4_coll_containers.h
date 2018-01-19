#ifndef CH4_COLL_CONTAINERS_H_INCLUDED
#define CH4_COLL_CONTAINERS_H_INCLUDED

/* Barrier CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Barrier__intra__composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Barrier__intra__composition_beta_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Barrier__intra__composition_gamma_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Barrier__inter__composition_alpha_cnt;

/* Bcast  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Bcast__intra__composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Bcast__intra__composition_beta_cnt;
extern const MPIDI_coll_algo_container_t CH4_Bcast__intra__composition_gamma_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Bcast__intra__composition_delta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Bcast__inter__composition_alpha_cnt;

/* Reduce  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Reduce__intra__composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Reduce__intra__composition_beta_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Reduce__intra__composition_gamma_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Reduce__inter__composition_alpha_cnt;

/* Allreduce  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Allreduce__intra__composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Allreduce__intra__composition_beta_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Allreduce__intra__composition_gamma_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Allreduce__inter__composition_alpha_cnt;

/* Alltoall  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Alltoall__intra__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Alltoall__intra__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Alltoall__inter__composition_alpha_cnt;

/* Alltoallv  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Alltoallv__intra__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Alltoallv__intra__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Alltoallv__inter__composition_alpha_cnt;

/* Alltoallw  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Alltoallw__intra__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Alltoallw__intra__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Alltoallw__inter__composition_alpha_cnt;

/* Allgather  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Allgather__intra__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Allgather__intra__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Allgather__inter__composition_alpha_cnt;

/* Allgatherv  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Allgatherv__intra__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Allgatherv__intra__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Allgatherv__inter__composition_alpha_cnt;

/* Gather  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Gather__intra__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Gather__intra__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Gather__inter__composition_alpha_cnt;

/* Gatherv  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Gatherv__intra__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Gatherv__intra__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Gatherv__inter__composition_alpha_cnt;

/* Scatter CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Scatter__intra__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Scatter__intra__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Scatter__inter__composition_alpha_cnt;

/* Scatterv CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Scatterv__intra__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Scatterv__intra__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Scatterv__inter__composition_alpha_cnt;

/* Reduce_scatter CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter__intra__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter__intra__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter__inter__composition_alpha_cnt;


/* Reduce_scatter_block CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_block__intra__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_block__intra__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_block__inter__composition_alpha_cnt;

/* Scan CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Scan__intra__composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Scan__intra__composition_beta_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Scan__intra__composition_gamma_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */

/* Exscan CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Exscan__intra__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Exscan__intra__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */

#endif /* CH4_COLL_CONTAINERS_H_INCLUDED */
