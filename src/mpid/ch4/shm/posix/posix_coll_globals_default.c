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
