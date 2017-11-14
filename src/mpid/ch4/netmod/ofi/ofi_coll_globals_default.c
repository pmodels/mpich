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

/* Allreduce default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t OFI_allreduce_recursive_doubling_cnt =
    {
        .id = MPIDI_OFI_allreduce_recursive_doubling_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_allreduce_reduce_scatter_allgather_cnt =
    {
        .id = MPIDI_OFI_allreduce_reduce_scatter_allgather_id
    };

/* Alltoall default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t OFI_alltoall_sendrecv_replace_cnt =
    {
        .id = MPIDI_OFI_alltoall_sendrecv_replace_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_alltoall_bruck_cnt =
    {
        .id = MPIDI_OFI_alltoall_bruck_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_alltoall_isend_irecv_cnt =
    {
        .id = MPIDI_OFI_alltoall_isend_irecv_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_alltoall_pairwise_exchange_cnt =
    {
        .id = MPIDI_OFI_alltoall_pairwise_exchange_id
    };

/* Alltoallv default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t OFI_alltoallv_sendrecv_replace_cnt =
    {
        .id = MPIDI_OFI_alltoallv_sendrecv_replace_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_alltoallv_isend_irecv_cnt =
    {
        .id = MPIDI_OFI_alltoallv_isend_irecv_id
    };

/* Alltoallw default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t OFI_alltoallw_sendrecv_replace_cnt =
    {
        .id = MPIDI_OFI_alltoallw_sendrecv_replace_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_alltoallw_isend_irecv_cnt =
    {
        .id = MPIDI_OFI_alltoallw_isend_irecv_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_alltoallw_pairwise_exchange_cnt =
    {
        .id = MPIDI_OFI_alltoallw_pairwise_exchange_id
    };

/* Allgather default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t OFI_allgather_recursive_doubling_cnt =
    {
        .id = MPIDI_OFI_allgather_recursive_doubling_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_allgather_bruck_cnt =
    {
        .id = MPIDI_OFI_allgather_bruck_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_allgather_ring_cnt =
    {
        .id = MPIDI_OFI_allgather_ring_id
    };

/* Allgatherv default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t OFI_allgatherv_recursive_doubling_cnt =
    {
        .id = MPIDI_OFI_allgatherv_recursive_doubling_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_allgatherv_bruck_cnt =
    {
        .id = MPIDI_OFI_allgatherv_bruck_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_allgatherv_ring_cnt =
    {
        .id = MPIDI_OFI_allgatherv_ring_id
    };
