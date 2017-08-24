#ifndef SHM_COLL_CONTAINERS_H_INCLUDED
#define SHM_COLL_CONTAINERS_H_INCLUDED

/* Barrier POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_barrier_recursive_doubling_cnt;

/* Bcast POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_bcast_binomial_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_bcast_scatter_doubling_allgather_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_bcast_scatter_ring_allgather_cnt;

/* Reduce POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_reduce_binomial_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_reduce_redscat_gather_cnt;

/* Allreduce POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_allreduce_recursive_doubling_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_allreduce_reduce_scatter_allgather_cnt;

#endif /* SHM_COLL_CONTAINERS_H_INCLUDED */
