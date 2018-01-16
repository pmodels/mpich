#ifndef SHM_COLL_CONTAINERS_H_INCLUDED
#define SHM_COLL_CONTAINERS_H_INCLUDED

/* Barrier POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Barrier_intra_recursive_doubling_cnt;

/* Bcast POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Bcast_intra_binomial_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Bcast_intra_scatter_doubling_allgather_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Bcast_intra_scatter_ring_allgather_cnt;

/* Reduce POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_intra_binomial_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_intra_redscat_gather_cnt;

/* Allreduce POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Allreduce_intra_recursive_doubling_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Allreduce_intra_reduce_scatter_allgather_cnt;

/* Alltoall POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoall_intra_brucks_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoall_intra_scattered_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoall_intra_pairwise_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoall_intra_pairwise_sendrecv_replace_cnt;

/* Alltoallv POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoallv_intra_scattered_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoallv_intra_pairwise_sendrecv_replace_cnt;

/* Alltoallw POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoallw_intra_scattered_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoallw_intra_pairwise_sendrecv_replace_cnt;

/* Allgather POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Allgather_intra_recursive_doubling_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Allgather_intra_brucks_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Allgather_intra_ring_cnt;

/* Allgatherv POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Allgatherv_intra_recursive_doubling_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Allgatherv_intra_brucks_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Allgatherv_intra_ring_cnt;

/* Gather POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Gather_intra_binomial_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Gather_intra_binomial_indexed_cnt;

/* Gatherv POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Gatherv_intra_linear_ssend_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Gatherv_intra_linear_cnt;

/* Scatter POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Scatter_intra_binomial_cnt;

/* Scatterv POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Scatterv_intra_linear_cnt;

/* Reduce_scatter POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_scatter_intra_noncomm_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_scatter_intra_pairwise_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_scatter_intra_recursive_doubling_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_scatter_intra_recursive_halving_cnt;

/* Reduce_scatter_block POSIX containers declaration */
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_scatter_block_intra_noncomm_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_scatter_block_intra_pairwise_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_scatter_block_intra_recursive_doubling_cnt;
extern const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_scatter_block_intra_recursive_halving_cnt;

#endif /* SHM_COLL_CONTAINERS_H_INCLUDED */
