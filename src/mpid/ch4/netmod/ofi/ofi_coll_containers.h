#ifndef OFI_COLL_CONTAINERS_H_INCLUDED
#define OFI_COLL_CONTAINERS_H_INCLUDED

/* Barrier OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_Barrier_intra_dissemination_cnt;

/* Bcast OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_Bcast_intra_binomial_cnt;
extern const MPIDI_OFI_coll_algo_container_t
    OFI_Bcast_intra_scatter_recursive_doubling_allgather_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_Bcast_intra_scatter_ring_allgather_cnt;

/* Reduce OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_Reduce_intra_binomial_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_Reduce_intra_reduce_scatter_gather_cnt;

/* Allreduce OFI containers declaration */
extern const MPIDI_OFI_coll_algo_container_t OFI_Allreduce_intra_recursive_doubling_cnt;
extern const MPIDI_OFI_coll_algo_container_t OFI_Allreduce_intra_reduce_scatter_allgather_cnt;

#endif /* OFI_COLL_CONTAINERS_H_INCLUDED */
