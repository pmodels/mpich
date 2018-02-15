#ifndef POSIX_COLL_CONTAINERS_H_INCLUDED
#define POSIX_COLL_CONTAINERS_H_INCLUDED

/* Barrier POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_barrier__recursive_doubling_cnt;

/* Bcast POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_bcast__binomial_cnt;
extern const MPIDI_POSIX_coll_algo_container_t
    POSIX_bcast__scatter_recursive_doubling_allgather_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_bcast__scatter_ring_allgather_cnt;

/* Reduce POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_reduce__binomial_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_reduce__reduce_scatter_gather_cnt;

/* Allreduce POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_allreduce__recursive_doubling_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_allreduce__reduce_scatter_allgather_cnt;

#endif /* POSIX_COLL_CONTAINERS_H_INCLUDED */
