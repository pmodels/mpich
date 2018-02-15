/* Barrier default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_barrier__recursive_doubling_cnt = {
    .id = MPIDI_STUBSHM_barrier__recursive_doubling_id
};

/* Bcast default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_bcast__binomial_cnt = {
    .id = MPIDI_STUBSHM_bcast__binomial_id
};

const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_bcast__scatter_recursive_doubling_allgather_cnt = {
    .id = MPIDI_STUBSHM_bcast__scatter_recursive_doubling_allgather_id
};

const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_bcast__scatter_ring_allgather_cnt = {
    .id = MPIDI_STUBSHM_bcast__scatter_ring_allgather_id
};

/* Reduce default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_reduce__reduce_scatter_gather_cnt = {
    .id = MPIDI_STUBSHM_reduce__reduce_scatter_gather_id
};

const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_reduce__binomial_cnt = {
    .id = MPIDI_STUBSHM_reduce__binomial_id
};

/* Allreduce default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_allreduce__recursive_doubling_cnt = {
    .id = MPIDI_STUBSHM_allreduce__recursive_doubling_id
};

const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_allreduce__reduce_scatter_allgather_cnt = {
    .id = MPIDI_STUBSHM_allreduce__reduce_scatter_allgather_id
};
