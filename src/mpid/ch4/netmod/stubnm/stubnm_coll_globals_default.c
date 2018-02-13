/* Barrier default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t STUBNM_Barrier_intra_dissemination_cnt = {
    .id = MPIDI_STUBNM_Barrier_intra_dissemination_id
};

/* Bcast default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t STUBNM_Bcast_intra_binomial_cnt = {
    .id = MPIDI_STUBNM_Bcast_intra_binomial_id
};

const MPIDI_STUBNM_coll_algo_container_t STUBNM_Bcast_intra_scatter_recursive_doubling_allgather_cnt
    = {
    .id = MPIDI_STUBNM_Bcast_intra_scatter_recursive_doubling_allgather_id
};

const MPIDI_STUBNM_coll_algo_container_t STUBNM_Bcast_intra_scatter_ring_allgather_cnt = {
    .id = MPIDI_STUBNM_Bcast_intra_scatter_ring_allgather_id
};

/* Reduce default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t STUBNM_Reduce_intra_reduce_scatter_gather_cnt = {
    .id = MPIDI_STUBNM_Reduce_intra_reduce_scatter_gather_id
};

const MPIDI_STUBNM_coll_algo_container_t STUBNM_Reduce_intra_binomial_cnt = {
    .id = MPIDI_STUBNM_Reduce_intra_binomial_id
};

/* Allreduce default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t STUBNM_Allreduce_intra_recursive_doubling_cnt = {
    .id = MPIDI_STUBNM_Allreduce_intra_recursive_doubling_id
};

const MPIDI_STUBNM_coll_algo_container_t STUBNM_Allreduce_intra_reduce_scatter_allgather_cnt = {
    .id = MPIDI_STUBNM_Allreduce_intra_reduce_scatter_allgather_id
};
