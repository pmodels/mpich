/* Barrier default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_Barrier_intra_dissemination_cnt = {
    .id = MPIDI_STUBSHM_Barrier_intra_dissemination_id
};

/* Bcast default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_Bcast_intra_binomial_cnt = {
    .id = MPIDI_STUBSHM_Bcast_intra_binomial_id
};

const MPIDI_STUBSHM_coll_algo_container_t
    STUBSHM_Bcast_intra_scatter_recursive_doubling_allgather_cnt = {
    .id = MPIDI_STUBSHM_Bcast_intra_scatter_recursive_doubling_allgather_id
};

const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_Bcast_intra_scatter_ring_allgather_cnt = {
    .id = MPIDI_STUBSHM_Bcast_intra_scatter_ring_allgather_id
};

/* Reduce default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_Reduce_intra_reduce_scatter_gather_cnt = {
    .id = MPIDI_STUBSHM_Reduce_intra_reduce_scatter_gather_id
};

const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_Reduce_intra_binomial_cnt = {
    .id = MPIDI_STUBSHM_Reduce_intra_binomial_id
};

/* Allreduce default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_Allreduce_intra_recursive_doubling_cnt = {
    .id = MPIDI_STUBSHM_Allreduce_intra_recursive_doubling_id
};

const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_Allreduce_intra_reduce_scatter_allgather_cnt = {
    .id = MPIDI_STUBSHM_Allreduce_intra_reduce_scatter_allgather_id
};
