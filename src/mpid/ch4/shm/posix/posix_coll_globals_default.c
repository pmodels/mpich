/* Barrier default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Barrier_intra_dissemination_cnt = {
    .id = MPIDI_POSIX_Barrier_intra_dissemination_id
};

/* Bcast default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Bcast_intra_binomial_cnt = {
    .id = MPIDI_POSIX_Bcast_intra_binomial_id
};

const MPIDI_POSIX_coll_algo_container_t POSIX_Bcast_intra_scatter_recursive_doubling_allgather_cnt = {
    .id = MPIDI_POSIX_Bcast_intra_scatter_recursive_doubling_allgather_id
};

const MPIDI_POSIX_coll_algo_container_t POSIX_Bcast_intra_scatter_ring_allgather_cnt = {
    .id = MPIDI_POSIX_Bcast_intra_scatter_ring_allgather_id
};

/* Reduce default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_intra_reduce_scatter_gather_cnt = {
    .id = MPIDI_POSIX_Reduce_intra_reduce_scatter_gather_id
};

const MPIDI_POSIX_coll_algo_container_t POSIX_Reduce_intra_binomial_cnt = {
    .id = MPIDI_POSIX_Reduce_intra_binomial_id
};

/* Allreduce default POSIX containers initialization*/
const MPIDI_POSIX_coll_algo_container_t POSIX_Allreduce_intra_recursive_doubling_cnt = {
    .id = MPIDI_POSIX_Allreduce_intra_recursive_doubling_id
};

const MPIDI_POSIX_coll_algo_container_t POSIX_Allreduce_intra_reduce_scatter_allgather_cnt = {
    .id = MPIDI_POSIX_Allreduce_intra_reduce_scatter_allgather_id
};
