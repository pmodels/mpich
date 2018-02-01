/* Barrier default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t UCX_barrier__recursive_doubling_cnt = {
    .id = MPIDI_UCX_barrier__recursive_doubling_id
};

/* Bcast default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t UCX_bcast__binomial_cnt = {
    .id = MPIDI_UCX_bcast__binomial_id
};

const MPIDI_UCX_coll_algo_container_t UCX_bcast__scatter_recursive_doubling_allgather_cnt = {
    .id = MPIDI_UCX_bcast__scatter_recursive_doubling_allgather_id
};

const MPIDI_UCX_coll_algo_container_t UCX_bcast__scatter_ring_allgather_cnt = {
    .id = MPIDI_UCX_bcast__scatter_ring_allgather_id
};

/* Reduce default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t UCX_reduce__reduce_scatter_gather_cnt = {
    .id = MPIDI_UCX_reduce__reduce_scatter_gather_id
};

const MPIDI_UCX_coll_algo_container_t UCX_reduce__binomial_cnt = {
    .id = MPIDI_UCX_reduce__binomial_id
};

/* Allreduce default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t UCX_allreduce__recursive_doubling_cnt = {
    .id = MPIDI_UCX_allreduce__recursive_doubling_id
};

const MPIDI_UCX_coll_algo_container_t UCX_allreduce__reduce_scatter_allgather_cnt = {
    .id = MPIDI_UCX_allreduce__reduce_scatter_allgather_id
};
