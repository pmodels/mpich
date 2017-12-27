#ifndef OFI_COLL_CONTAINERS_H_INCLUDED
#define OFI_COLL_CONTAINERS_H_INCLUDED

/* Barrier OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_barrier_recursive_doubling_cnt;

/* Bcast OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_bcast_binomial_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_bcast_scatter_doubling_allgather_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_bcast_scatter_ring_allgather_cnt;

/* Reduce OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_reduce_binomial_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_reduce_redscat_gather_cnt;

/* Allreduce OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_allreduce_recursive_doubling_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_allreduce_reduce_scatter_allgather_cnt;

/* Gather OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_gather_intra_binomial_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_gather_intra_binomial_indexed_cnt;

/* Gatherv OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_gatherv_intra_linear_ssend_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_gatherv_intra_linear_cnt;

/* Scatter OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_scatter_intra_binomial_cnt;

/* Scatterv OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_scatterv_intra_linear_cnt;

#endif /* OFI_COLL_CONTAINERS_H_INCLUDED */
