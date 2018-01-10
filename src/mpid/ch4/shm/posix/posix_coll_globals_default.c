/* Barrier default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_barrier__recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_barrier__recursive_doubling_id
    };

/* Bcast default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_bcast__binomial_cnt =
    {
        .id = MPIDI_POSIX_bcast__binomial_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_bcast__scatter_recursive_doubling_allgather_cnt =
    {
        .id = MPIDI_POSIX_bcast__scatter_recursive_doubling_allgather_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_bcast__scatter_ring_allgather_cnt =
    {
        .id = MPIDI_POSIX_bcast__scatter_ring_allgather_id
    };

/* Reduce default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_reduce__reduce_scatter_gather_cnt =
    {
        .id = MPIDI_POSIX_reduce__reduce_scatter_gather_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_reduce__binomial_cnt =
    {
        .id = MPIDI_POSIX_reduce__binomial_id
    };

/* Allreduce default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_allreduce__recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_allreduce__recursive_doubling_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_allreduce__reduce_scatter_allgather_cnt =
    {
        .id = MPIDI_POSIX_allreduce__reduce_scatter_allgather_id
    };
