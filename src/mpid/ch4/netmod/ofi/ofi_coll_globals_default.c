/* Barrier default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_barrier__recursive_doubling_cnt =
    {
        .id = MPIDI_OFI_barrier__recursive_doubling_id
    };

/* Bcast default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_bcast__binomial_cnt =
    {
        .id = MPIDI_OFI_bcast__binomial_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_bcast__scatter_recursive_doubling_allgather_cnt =
    {
        .id = MPIDI_OFI_bcast__scatter_recursive_doubling_allgather_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_bcast__scatter_ring_allgather_cnt =
    {
        .id = MPIDI_OFI_bcast__scatter_ring_allgather_id
    };

/* Reduce default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_reduce__reduce_scatter_gather_cnt =
    {
        .id = MPIDI_OFI_reduce__reduce_scatter_gather_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_reduce__binomial_cnt =
    {
        .id = MPIDI_OFI_reduce__binomial_id
    };

/* Allreduce default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_allreduce__recursive_doubling_cnt =
    {
        .id = MPIDI_OFI_allreduce__recursive_doubling_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_allreduce__reduce_scatter_allgather_cnt =
    {
        .id = MPIDI_OFI_allreduce__reduce_scatter_allgather_id
    };
