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

/* Alltoall OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_alltoall_bruck_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_alltoall_isend_irecv_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_alltoall_pairwise_exchange_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_alltoall_sendrecv_replace_cnt;

/* Alltoallv OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_alltoallv_isend_irecv_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_alltoallv_sendrecv_replace_cnt;

/* Alltoallw OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_alltoallw_isend_irecv_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_alltoallw_pairwise_exchange_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_alltoallw_sendrecv_replace_cnt;

/* Allgather OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_allgather_recursive_doubling_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_allgather_bruck_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_allgather_ring_cnt;

/* Allgatherv OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_allgatherv_recursive_doubling_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_allgatherv_bruck_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_allgatherv_ring_cnt;

#endif /* OFI_COLL_CONTAINERS_H_INCLUDED */
