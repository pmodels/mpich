#ifndef CH4_COLL_CONTAINERS_H_INCLUDED
#define CH4_COLL_CONTAINERS_H_INCLUDED

/* Barrier CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_barrier_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_barrier_composition_beta_cnt;
extern const MPIDI_coll_algo_container_t CH4_barrier_intercomm_cnt;

/* Bcast  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_bcast_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_bcast_composition_beta_cnt;
extern const MPIDI_coll_algo_container_t CH4_bcast_composition_gamma_cnt;
extern const MPIDI_coll_algo_container_t CH4_bcast_intercomm_cnt;

/* Reduce  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_reduce_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_reduce_composition_beta_cnt;
extern const MPIDI_coll_algo_container_t CH4_reduce_intercomm_cnt;

/* Allreduce  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t CH4_allreduce_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t CH4_allreduce_composition_beta_cnt;
extern const MPIDI_coll_algo_container_t CH4_allreduce_composition_gamma_cnt;
extern const MPIDI_coll_algo_container_t CH4_allreduce_intercomm_cnt;

#endif /* CH4_COLL_CONTAINERS_H_INCLUDED */
