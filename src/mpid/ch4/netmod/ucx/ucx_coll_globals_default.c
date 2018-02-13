/* Barrier default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t UCX_Barrier_intra_recursive_doubling_cnt = {
    .id = MPIDI_UCX_Barrier_intra_recursive_doubling_id
};

/* Bcast default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t UCX_Bcast_intra_binomial_cnt = {
    .id = MPIDI_UCX_Bcast_intra_binomial_id
};

const MPIDI_UCX_coll_algo_container_t UCX_Bcast_intra_scatter_recursive_doubling_allgather_cnt = {
    .id = MPIDI_UCX_Bcast_intra_scatter_recursive_doubling_allgather_id
};

const MPIDI_UCX_coll_algo_container_t UCX_Bcast_intra_scatter_ring_allgather_cnt = {
    .id = MPIDI_UCX_Bcast_intra_scatter_ring_allgather_id
};

/* Reduce default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t UCX_Reduce_intra_reduce_scatter_gather_cnt = {
    .id = MPIDI_UCX_Reduce_intra_reduce_scatter_gather_id
};

const MPIDI_UCX_coll_algo_container_t UCX_Reduce_intra_binomial_cnt = {
    .id = MPIDI_UCX_Reduce_intra_binomial_id
};

/* Allreduce default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t UCX_Allreduce_intra_recursive_doubling_cnt = {
    .id = MPIDI_UCX_Allreduce_intra_recursive_doubling_id
};

const MPIDI_UCX_coll_algo_container_t UCX_Allreduce_intra_reduce_scatter_allgather_cnt = {
    .id = MPIDI_UCX_Allreduce_intra_reduce_scatter_allgather_id
};
