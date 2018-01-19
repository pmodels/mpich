#ifndef CH4_COLL_CONTAINERS_H_INCLUDED
#define CH4_COLL_CONTAINERS_H_INCLUDED

/* Barrier CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Barrier__composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Barrier__composition_beta_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Barrier__composition_gamma_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Barrier__intercomm_cnt;

/* Bcast  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Bcast__composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Bcast__composition_beta_cnt;
extern const MPIDI_coll_algo_container_t CH4_Bcast__composition_gamma_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Bcast__composition_delta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Bcast__intercomm_cnt;

/* Reduce  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Reduce__composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Reduce__composition_beta_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Reduce__composition_gamma_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Reduce__intercomm_cnt;

/* Allreduce  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Allreduce__composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Allreduce__composition_beta_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Allreduce__composition_gamma_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Allreduce__intercomm_cnt;

/* Alltoall  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Alltoall__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Alltoall__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Alltoall__intercomm_cnt;

/* Alltoallv  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Alltoallv__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Alltoallv__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Alltoallv__intercomm_cnt;

/* Alltoallw  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Alltoallw__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Alltoallw__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Alltoallw__intercomm_cnt;

/* Allgather  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Allgather__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Allgather__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Allgather__intercomm_cnt;

/* Allgatherv  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Allgatherv__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Allgatherv__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Allgatherv__intercomm_cnt;

/* Gather  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Gather__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Gather__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Gather__intercomm_cnt;

/* Gatherv  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Gatherv__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Gatherv__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Gatherv__intercomm_cnt;

/* Scatter CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Scatter__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Scatter__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Scatter__intercomm_cnt;

/* Scatterv CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Scatterv__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Scatterv__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Scatterv__intercomm_cnt;

/* Reduce_scatter CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter__intercomm_cnt;


/* Reduce_scatter_block CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_block__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_block__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */
extern const MPIDI_coll_algo_container_t CH4_Reduce_scatter_block__intercomm_cnt;

/* Scan CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Scan__composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_Scan__composition_beta_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Scan__composition_gamma_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */

/* Exscan CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_Exscan__composition_alpha_cnt;
#ifdef MPIDI_BUILD_CH4_SHM
extern const MPIDI_coll_algo_container_t CH4_Exscan__composition_beta_cnt;
#endif /* MPIDI_BUILD_CH4_SHM */

#endif /* CH4_COLL_CONTAINERS_H_INCLUDED */
