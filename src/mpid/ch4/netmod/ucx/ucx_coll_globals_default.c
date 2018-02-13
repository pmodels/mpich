/* Barrier default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t UCX_barrier_intra_recursive_doubling_cnt = {
    .id = MPIDI_UCX_barrier_intra_recursive_doubling_id
};

/* Bcast default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t UCX_bcast_intra_binomial_cnt = {
    .id = MPIDI_UCX_bcast_intra_binomial_id
};

const MPIDI_UCX_coll_algo_container_t UCX_bcast_intra_scatter_recursive_doubling_allgather_cnt = {
    .id = MPIDI_UCX_bcast_intra_scatter_recursive_doubling_allgather_id
};

const MPIDI_UCX_coll_algo_container_t UCX_bcast_intra_scatter_ring_allgather_cnt = {
    .id = MPIDI_UCX_bcast_intra_scatter_ring_allgather_id
};

/* Reduce default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t UCX_reduce_intra_reduce_scatter_gather_cnt = {
    .id = MPIDI_UCX_reduce_intra_reduce_scatter_gather_id
};

const MPIDI_UCX_coll_algo_container_t UCX_reduce_intra_binomial_cnt = {
    .id = MPIDI_UCX_reduce_intra_binomial_id
};

/* Allreduce default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t UCX_allreduce_intra_recursive_doubling_cnt = {
    .id = MPIDI_UCX_allreduce_intra_recursive_doubling_id
};

const MPIDI_UCX_coll_algo_container_t UCX_allreduce_intra_reduce_scatter_allgather_cnt = {
    .id = MPIDI_UCX_allreduce_intra_reduce_scatter_allgather_id
};
