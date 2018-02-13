#ifndef POSIX_COLL_CONTAINERS_H_INCLUDED
#define POSIX_COLL_CONTAINERS_H_INCLUDED

/* Barrier POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Barrier_intra_dissemination_cnt;

/* Bcast POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Bcast_intra_binomial_cnt;
extern const MPIDI_POSIX_coll_algo_container_t
    POSIX_Bcast_intra_scatter_recursive_doubling_allgather_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Bcast_intra_scatter_ring_allgather_cnt;

/* Reduce POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_intra_binomial_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_intra_reduce_scatter_gather_cnt;

/* Allreduce POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Allreduce_intra_recursive_doubling_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Allreduce_intra_reduce_scatter_allgather_cnt;

#endif /* POSIX_COLL_CONTAINERS_H_INCLUDED */
