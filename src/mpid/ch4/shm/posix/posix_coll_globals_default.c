/* Barrier default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Barrier_intra_recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_Barrier_intra_recursive_doubling_id
    };

/* Bcast default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Bcast_intra_binomial_cnt =
    {
        .id = MPIDI_POSIX_Bcast_intra_binomial_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Bcast_intra_scatter_doubling_allgather_cnt =
    {
        .id = MPIDI_POSIX_Bcast_intra_scatter_doubling_allgather_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Bcast_intra_scatter_ring_allgather_cnt =
    {
        .id = MPIDI_POSIX_Bcast_intra_scatter_ring_allgather_id
    };

/* Reduce default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_intra_redscat_gather_cnt =
    {
        .id = MPIDI_POSIX_Reduce_intra_redscat_gather_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_intra_binomial_cnt =
    {
        .id = MPIDI_POSIX_Reduce_intra_binomial_id
    };

/* Allreduce default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_Allreduce_intra_recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_Allreduce_intra_recursive_doubling_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Allreduce_intra_reduce_scatter_allgather_cnt =
    {
        .id = MPIDI_POSIX_Allreduce_intra_reduce_scatter_allgather_id
    };

/* Alltoall default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoall_intra_pairwise_sendrecv_replace_cnt =
    {
        .id = MPIDI_POSIX_Alltoall_intra_pairwise_sendrecv_replace_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoall_intra_brucks_cnt =
    {
        .id = MPIDI_POSIX_Alltoall_intra_brucks_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoall_intra_scattered_cnt =
    {
        .id = MPIDI_POSIX_Alltoall_intra_scattered_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoall_intra_pairwise_cnt =
    {
        .id = MPIDI_POSIX_Alltoall_intra_pairwise_id
    };

/* Alltoallv default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoallv_intra_pairwise_sendrecv_replace_cnt =
    {
        .id = MPIDI_POSIX_Alltoallv_intra_pairwise_sendrecv_replace_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoallv_intra_scattered_cnt =
    {
        .id = MPIDI_POSIX_Alltoallv_intra_scattered_id
    };

/* Alltoallw default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoallw_intra_pairwise_sendrecv_replace_cnt =
    {
        .id = MPIDI_POSIX_Alltoallw_intra_pairwise_sendrecv_replace_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoallw_intra_scattered_cnt =
    {
        .id = MPIDI_POSIX_Alltoallw_intra_scattered_id
    };

/* Allgather default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_Allgather_intra_recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_Allgather_intra_recursive_doubling_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Allgather_intra_brucks_cnt =
    {
        .id = MPIDI_POSIX_Allgather_intra_brucks_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Allgather_intra_ring_cnt =
    {
        .id = MPIDI_POSIX_Allgather_intra_ring_id
    };

/* Allgatherv default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_Allgatherv_intra_recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_Allgatherv_intra_recursive_doubling_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Allgatherv_intra_brucks_cnt =
    {
        .id = MPIDI_POSIX_Allgatherv_intra_brucks_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Allgatherv_intra_ring_cnt =
    {
        .id = MPIDI_POSIX_Allgatherv_intra_ring_id
    };

/* Gather default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Gather_intra_binomial_cnt =
    {
        .id = MPIDI_POSIX_Gather_intra_binomial_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Gather_intra_binomial_indexed_cnt =
    {
        .id = MPIDI_POSIX_Gather_intra_binomial_indexed_id
    };

/* Gatherv default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Gatherv_intra_linear_ssend_cnt =
    {
        .id = MPIDI_POSIX_Gatherv_intra_linear_ssend_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Gatherv_intra_linear_cnt =
    {
        .id = MPIDI_POSIX_Gatherv_intra_linear_id
    };

/* Scatter default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Scatter_intra_binomial_cnt =
    {
        .id = MPIDI_POSIX_Scatter_intra_binomial_id
    };

/* Scatterv default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Scatterv_intra_linear_cnt =
    {
        .id = MPIDI_POSIX_Scatterv_intra_linear_id
    };
