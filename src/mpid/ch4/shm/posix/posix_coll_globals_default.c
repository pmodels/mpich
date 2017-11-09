/* Barrier default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_barrier_recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_barrier_recursive_doubling_id
    };

/* Bcast default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_bcast_binomial_cnt =
    {
        .id = MPIDI_POSIX_bcast_binomial_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_bcast_scatter_doubling_allgather_cnt =
    {
        .id = MPIDI_POSIX_bcast_scatter_doubling_allgather_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_bcast_scatter_ring_allgather_cnt =
    {
        .id = MPIDI_POSIX_bcast_scatter_ring_allgather_id
    };

/* Reduce default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_reduce_redscat_gather_cnt =
    {
        .id = MPIDI_POSIX_reduce_redscat_gather_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_reduce_binomial_cnt =
    {
        .id = MPIDI_POSIX_reduce_binomial_id
    };

/* Allreduce default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_allreduce_recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_allreduce_recursive_doubling_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_allreduce_reduce_scatter_allgather_cnt =
    {
        .id = MPIDI_POSIX_allreduce_reduce_scatter_allgather_id
    };

/* Alltoall default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_alltoall_sendrecv_replace_cnt =
    {
        .id = MPIDI_POSIX_alltoall_sendrecv_replace_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_alltoall_bruck_cnt =
    {
        .id = MPIDI_POSIX_alltoall_bruck_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_alltoall_isend_irecv_cnt =
    {
        .id = MPIDI_POSIX_alltoall_isend_irecv_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_alltoall_pairwise_exchange_cnt =
    {
        .id = MPIDI_POSIX_alltoall_pairwise_exchange_id
    };

/* Alltoallv default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_alltoallv_sendrecv_replace_cnt =
    {
        .id = MPIDI_POSIX_alltoallv_sendrecv_replace_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_alltoallv_isend_irecv_cnt =
    {
        .id = MPIDI_POSIX_alltoallv_isend_irecv_id
    };

/* Alltoallw default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_alltoallw_sendrecv_replace_cnt =
    {
        .id = MPIDI_POSIX_alltoallw_sendrecv_replace_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_alltoallw_isend_irecv_cnt =
    {
        .id = MPIDI_POSIX_alltoallw_isend_irecv_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_alltoallw_pairwise_exchange_cnt =
    {
        .id = MPIDI_POSIX_alltoallw_pairwise_exchange_id
    };

/* Allgather default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_allgather_recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_allgather_recursive_doubling_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_allgather_bruck_cnt =
    {
        .id = MPIDI_POSIX_allgather_bruck_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_allgather_ring_cnt =
    {
        .id = MPIDI_POSIX_allgather_ring_id
    };

/* Allgatherv default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_allgatherv_recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_allgatherv_recursive_doubling_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_allgatherv_bruck_cnt =
    {
        .id = MPIDI_POSIX_allgatherv_bruck_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_allgatherv_ring_cnt =
    {
        .id = MPIDI_POSIX_allgatherv_ring_id
    };
