/* Barrier default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_barrier_intra_recursive_doubling_cnt = {
    .id = MPIDI_POSIX_barrier_intra_recursive_doubling_id
};

/* Bcast default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_bcast_intra_binomial_cnt = {
    .id = MPIDI_POSIX_bcast_intra_binomial_id
};

const MPIDI_POSIX_coll_algo_container_t POSIX_bcast_intra_scatter_recursive_doubling_allgather_cnt = {
    .id = MPIDI_POSIX_bcast_intra_scatter_recursive_doubling_allgather_id
};

const MPIDI_POSIX_coll_algo_container_t POSIX_bcast_intra_scatter_ring_allgather_cnt = {
    .id = MPIDI_POSIX_bcast_intra_scatter_ring_allgather_id
};

/* Reduce default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_reduce_intra_reduce_scatter_gather_cnt = {
    .id = MPIDI_POSIX_reduce_intra_reduce_scatter_gather_id
};

const MPIDI_POSIX_coll_algo_container_t POSIX_reduce_intra_binomial_cnt = {
    .id = MPIDI_POSIX_reduce_intra_binomial_id
};

/* Allreduce default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_allreduce_intra_recursive_doubling_cnt = {
    .id = MPIDI_POSIX_allreduce_intra_recursive_doubling_id
};

const MPIDI_POSIX_coll_algo_container_t POSIX_allreduce_intra_reduce_scatter_allgather_cnt = {
    .id = MPIDI_POSIX_allreduce_intra_reduce_scatter_allgather_id
};
