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

/* Gather default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_gather_intra_binomial_cnt =
    {
        .id = MPIDI_POSIX_gather_intra_binomial_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_gather_intra_binomial_indexed_cnt =
    {
        .id = MPIDI_POSIX_gather_intra_binomial_indexed_id
    };

/* Gatherv default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_gatherv_intra_linear_ssend_cnt =
    {
        .id = MPIDI_POSIX_gatherv_intra_linear_ssend_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_gatherv_intra_linear_cnt =
    {
        .id = MPIDI_POSIX_gatherv_intra_linear_id
    };

/* Scatter default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_scatter_intra_binomial_cnt =
    {
        .id = MPIDI_POSIX_scatter_intra_binomial_id
    };

/* Scatterv default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_scatterv_intra_linear_cnt =
    {
        .id = MPIDI_POSIX_scatterv_intra_linear_id
    };

