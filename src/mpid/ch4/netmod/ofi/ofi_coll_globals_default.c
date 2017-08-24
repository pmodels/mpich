/* Barrier default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_barrier_recursive_doubling_cnt =
    {
        .id = MPIDI_OFI_barrier_recursive_doubling_id
    };

/* Bcast default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_bcast_binomial_cnt =
    {
        .id = MPIDI_OFI_bcast_binomial_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_bcast_scatter_doubling_allgather_cnt =
    {
        .id = MPIDI_OFI_bcast_scatter_doubling_allgather_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_bcast_scatter_ring_allgather_cnt =
    {
        .id = MPIDI_OFI_bcast_scatter_ring_allgather_id
    };

/* Reduce default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_reduce_redscat_gather_cnt =
    {
        .id = MPIDI_OFI_reduce_redscat_gather_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_reduce_binomial_cnt =
    {
        .id = MPIDI_OFI_reduce_binomial_id
    };

/* Allreduce default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_allreduce_recursive_doubling_cnt =
    {
        .id = MPIDI_OFI_allreduce_recursive_doubling_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_allreduce_reduce_scatter_allgather_cnt =
    {
        .id = MPIDI_OFI_allreduce_reduce_scatter_allgather_id
    };
