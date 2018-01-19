/* Barrier default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Barrier__intra__dissemination_cnt =
    {
        .id = MPIDI_POSIX_Barrier__intra__dissemination_id
    };

/* Bcast default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Bcast__intra__binomial_cnt =
    {
        .id = MPIDI_POSIX_Bcast__intra__binomial_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Bcast__intra__scatter_recursive_doubling_allgather_cnt =
    {
        .id = MPIDI_POSIX_Bcast__intra__scatter_recursive_doubling_allgather_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Bcast__intra__scatter_ring_allgather_cnt =
    {
        .id = MPIDI_POSIX_Bcast__intra__scatter_ring_allgather_id
    };

/* Reduce default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce__intra__reduce_scatter_gather_cnt =
    {
        .id = MPIDI_POSIX_Reduce__intra__reduce_scatter_gather_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce__intra__binomial_cnt =
    {
        .id = MPIDI_POSIX_Reduce__intra__binomial_id
    };

/* Allreduce default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_Allreduce__intra__recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_Allreduce__intra__recursive_doubling_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Allreduce__intra__reduce_scatter_allgather_cnt =
    {
        .id = MPIDI_POSIX_Allreduce__intra__reduce_scatter_allgather_id
    };

/* Alltoall default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoall__intra__pairwise_sendrecv_replace_cnt =
    {
        .id = MPIDI_POSIX_Alltoall__intra__pairwise_sendrecv_replace_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoall__intra__brucks_cnt =
    {
        .id = MPIDI_POSIX_Alltoall__intra__brucks_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoall__intra__scattered_cnt =
    {
        .id = MPIDI_POSIX_Alltoall__intra__scattered_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoall__intra__pairwise_cnt =
    {
        .id = MPIDI_POSIX_Alltoall__intra__pairwise_id
    };

/* Alltoallv default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoallv__intra__pairwise_sendrecv_replace_cnt =
    {
        .id = MPIDI_POSIX_Alltoallv__intra__pairwise_sendrecv_replace_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoallv__intra__scattered_cnt =
    {
        .id = MPIDI_POSIX_Alltoallv__intra__scattered_id
    };

/* Alltoallw default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoallw__intra__pairwise_sendrecv_replace_cnt =
    {
        .id = MPIDI_POSIX_Alltoallw__intra__pairwise_sendrecv_replace_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Alltoallw__intra__scattered_cnt =
    {
        .id = MPIDI_POSIX_Alltoallw__intra__scattered_id
    };

/* Allgather default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_Allgather__intra__recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_Allgather__intra__recursive_doubling_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Allgather__intra__brucks_cnt =
    {
        .id = MPIDI_POSIX_Allgather__intra__brucks_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Allgather__intra__ring_cnt =
    {
        .id = MPIDI_POSIX_Allgather__intra__ring_id
    };

/* Allgatherv default POSIX containers initialization */
const MPIDI_POSIX_coll_algo_container_t POSIX_Allgatherv__intra__recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_Allgatherv__intra__recursive_doubling_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Allgatherv__intra__brucks_cnt =
    {
        .id = MPIDI_POSIX_Allgatherv__intra__brucks_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Allgatherv__intra__ring_cnt =
    {
        .id = MPIDI_POSIX_Allgatherv__intra__ring_id
    };

/* Gather default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Gather__intra__binomial_cnt =
    {
        .id = MPIDI_POSIX_Gather__intra__binomial_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Gather__intra__binomial_indexed_cnt =
    {
        .id = MPIDI_POSIX_Gather__intra__binomial_indexed_id
    };

/* Gatherv default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Gatherv__intra__linear_ssend_cnt =
    {
        .id = MPIDI_POSIX_Gatherv__intra__linear_ssend_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Gatherv__intra__linear_cnt =
    {
        .id = MPIDI_POSIX_Gatherv__intra__linear_id
    };

/* Scatter default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Scatter__intra__binomial_cnt =
    {
        .id = MPIDI_POSIX_Scatter__intra__binomial_id
    };

/* Scatterv default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Scatterv__intra__linear_cnt =
    {
        .id = MPIDI_POSIX_Scatterv__intra__linear_id
    };

/* Reduce_scatter default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_scatter__intra__noncommutative_cnt =
    {
        .id = MPIDI_POSIX_Reduce_scatter__intra__noncommutative_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_scatter__intra__pairwise_cnt =
    {
        .id = MPIDI_POSIX_Reduce_scatter__intra__pairwise_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_scatter__intra__recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_Reduce_scatter__intra__recursive_doubling_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_scatter__intra__recursive_halving_cnt =
    {
        .id = MPIDI_POSIX_Reduce_scatter__intra__recursive_halving_id
    };

/* Reduce_scatter_block default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_scatter_block__intra__noncommutative_cnt =
    {
        .id = MPIDI_POSIX_Reduce_scatter_block__intra__noncommutative_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_scatter_block__intra__pairwise_cnt =
    {
        .id = MPIDI_POSIX_Reduce_scatter_block__intra__pairwise_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_scatter_block__intra__recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_Reduce_scatter_block__intra__recursive_doubling_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_scatter_block__intra__recursive_halving_cnt =
    {
        .id = MPIDI_POSIX_Reduce_scatter_block__intra__recursive_halving_id
    };

/* Scan default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Scan__intra__recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_Scan__intra__recursive_doubling_id
    };

/* Exscan default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Exscan__intra__recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_Exscan__intra__recursive_doubling_id
    };

