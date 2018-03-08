/* Barrier default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Barrier_intra_dissemination_cnt = {
    .id = MPIDI_STUBNM_Barrier_intra_dissemination_id
};

/* Bcast default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Bcast_intra_binomial_cnt = {
    .id = MPIDI_STUBNM_Bcast_intra_binomial_id
};

const MPIDI_STUBNM_coll_algo_container_t
    MPIDI_STUBNM_Bcast_intra_scatter_recursive_doubling_allgather_cnt = {
    .id = MPIDI_STUBNM_Bcast_intra_scatter_recursive_doubling_allgather_id
};

const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Bcast_intra_scatter_ring_allgather_cnt = {
    .id = MPIDI_STUBNM_Bcast_intra_scatter_ring_allgather_id
};

/* Reduce default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Reduce_intra_reduce_scatter_gather_cnt = {
    .id = MPIDI_STUBNM_Reduce_intra_reduce_scatter_gather_id
};

const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Reduce_intra_binomial_cnt = {
    .id = MPIDI_STUBNM_Reduce_intra_binomial_id
};

/* Allreduce default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Allreduce_intra_recursive_doubling_cnt = {
    .id = MPIDI_STUBNM_Allreduce_intra_recursive_doubling_id
};

const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Allreduce_intra_reduce_scatter_allgather_cnt = {
    .id = MPIDI_STUBNM_Allreduce_intra_reduce_scatter_allgather_id
};

/* Alltoall default STUBNM containers initialization */
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Alltoall_intra_pairwise_sendrecv_replace_cnt = {
    .id = MPIDI_STUBNM_Alltoall_intra_pairwise_sendrecv_replace_id
};

const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Alltoall_intra_brucks_cnt = {
    .id = MPIDI_STUBNM_Alltoall_intra_brucks_id
};

const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Alltoall_intra_scattered_cnt = {
    .id = MPIDI_STUBNM_Alltoall_intra_scattered_id
};

const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Alltoall_intra_pairwise_cnt = {
    .id = MPIDI_STUBNM_Alltoall_intra_pairwise_id
};

/* Alltoallv default STUBNM containers initialization */
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Alltoallv_intra_pairwise_sendrecv_replace_cnt
    = {
    .id = MPIDI_STUBNM_Alltoallv_intra_pairwise_sendrecv_replace_id
};

const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Alltoallv_intra_scattered_cnt = {
    .id = MPIDI_STUBNM_Alltoallv_intra_scattered_id
};

/* Alltoallw default STUBNM containers initialization */
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Alltoallw_intra_pairwise_sendrecv_replace_cnt
    = {
    .id = MPIDI_STUBNM_Alltoallw_intra_pairwise_sendrecv_replace_id
};

const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Alltoallw_intra_scattered_cnt = {
    .id = MPIDI_STUBNM_Alltoallw_intra_scattered_id
};

/* Allgather default STUBNM containers initialization */
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Allgather_intra_recursive_doubling_cnt = {
    .id = MPIDI_STUBNM_Allgather_intra_recursive_doubling_id
};

const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Allgather_intra_brucks_cnt = {
    .id = MPIDI_STUBNM_Allgather_intra_brucks_id
};

const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Allgather_intra_ring_cnt = {
    .id = MPIDI_STUBNM_Allgather_intra_ring_id
};

/* Allgatherv default STUBNM containers initialization */
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Allgatherv_intra_recursive_doubling_cnt = {
    .id = MPIDI_STUBNM_Allgatherv_intra_recursive_doubling_id
};

const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Allgatherv_intra_brucks_cnt = {
    .id = MPIDI_STUBNM_Allgatherv_intra_brucks_id
};

const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Allgatherv_intra_ring_cnt = {
    .id = MPIDI_STUBNM_Allgatherv_intra_ring_id
};

/* Gather default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Gather_intra_binomial_cnt = {
    .id = MPIDI_STUBNM_Gather_intra_binomial_id
};

/* Gatherv default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Gatherv_allcomm_linear_cnt = {
    .id = MPIDI_STUBNM_Gatherv_allcomm_linear_id
};

/* Scatter default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Scatter_intra_binomial_cnt = {
    .id = MPIDI_STUBNM_Scatter_intra_binomial_id
};

/* Scatterv default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Scatterv_allcomm_linear_cnt = {
    .id = MPIDI_STUBNM_Scatterv_allcomm_linear_id
};

/* Reduce_scatter default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Reduce_scatter_intra_noncommutative_cnt = {
    .id = MPIDI_STUBNM_Reduce_scatter_intra_noncommutative_id
};

const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Reduce_scatter_intra_pairwise_cnt = {
    .id = MPIDI_STUBNM_Reduce_scatter_intra_pairwise_id
};

const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Reduce_scatter_intra_recursive_doubling_cnt = {
    .id = MPIDI_STUBNM_Reduce_scatter_intra_recursive_doubling_id
};

const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Reduce_scatter_intra_recursive_halving_cnt = {
    .id = MPIDI_STUBNM_Reduce_scatter_intra_recursive_halving_id
};

/* Reduce_scatter_block default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Reduce_scatter_block_intra_noncommutative_cnt
    = {
    .id = MPIDI_STUBNM_Reduce_scatter_block_intra_noncommutative_id
};

const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Reduce_scatter_block_intra_pairwise_cnt = {
    .id = MPIDI_STUBNM_Reduce_scatter_block_intra_pairwise_id
};

const MPIDI_STUBNM_coll_algo_container_t
    MPIDI_STUBNM_Reduce_scatter_block_intra_recursive_doubling_cnt = {
    .id = MPIDI_STUBNM_Reduce_scatter_block_intra_recursive_doubling_id
};

const MPIDI_STUBNM_coll_algo_container_t
    MPIDI_STUBNM_Reduce_scatter_block_intra_recursive_halving_cnt = {
    .id = MPIDI_STUBNM_Reduce_scatter_block_intra_recursive_halving_id
};

/* Scan default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Scan_intra_recursive_doubling_cnt = {
    .id = MPIDI_STUBNM_Scan_intra_recursive_doubling_id
};

/* Exscan default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t MPIDI_STUBNM_Exscan_intra_recursive_doubling_cnt = {
    .id = MPIDI_STUBNM_Exscan_intra_recursive_doubling_id
};
