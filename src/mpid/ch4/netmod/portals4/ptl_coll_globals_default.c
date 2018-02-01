/* Barrier default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t PTL_barrier__recursive_doubling_cnt = {
    .id = MPIDI_PTL_barrier__recursive_doubling_id
};

/* Bcast default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t PTL_bcast__binomial_cnt = {
    .id = MPIDI_PTL_bcast__binomial_id
};

const MPIDI_PTL_coll_algo_container_t PTL_bcast__scatter_recursive_doubling_allgather_cnt = {
    .id = MPIDI_PTL_bcast__scatter_recursive_doubling_allgather_id
};

const MPIDI_PTL_coll_algo_container_t PTL_bcast__scatter_ring_allgather_cnt = {
    .id = MPIDI_PTL_bcast__scatter_ring_allgather_id
};

/* Reduce default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t PTL_reduce__reduce_scatter_gather_cnt = {
    .id = MPIDI_PTL_reduce__reduce_scatter_gather_id
};

const MPIDI_PTL_coll_algo_container_t PTL_reduce__binomial_cnt = {
    .id = MPIDI_PTL_reduce__binomial_id
};

/* Allreduce default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t PTL_allreduce__recursive_doubling_cnt = {
    .id = MPIDI_PTL_allreduce__recursive_doubling_id
};

const MPIDI_PTL_coll_algo_container_t PTL_allreduce__reduce_scatter_allgather_cnt = {
    .id = MPIDI_PTL_allreduce__reduce_scatter_allgather_id
};
