#ifndef OFI_COLL_CONTAINERS_H_INCLUDED
#define OFI_COLL_CONTAINERS_H_INCLUDED

/* Barrier OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_barrier__recursive_doubling_cnt;

/* Bcast OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_bcast__binomial_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_bcast__scatter_recursive_doubling_allgather_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_bcast__scatter_ring_allgather_cnt;

/* Reduce OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_reduce__binomial_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_reduce__reduce_scatter_gather_cnt;

/* Allreduce OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_allreduce__recursive_doubling_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_allreduce__reduce_scatter_allgather_cnt;

#endif /* OFI_COLL_CONTAINERS_H_INCLUDED */
