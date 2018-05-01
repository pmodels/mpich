/* Barrier default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Barrier_intra_dissemination_cnt = {
    .id = MPIDI_PTL_Barrier_intra_dissemination_id
};

/* Bcast default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Bcast_intra_binomial_cnt = {
    .id = MPIDI_PTL_Bcast_intra_binomial_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Bcast_intra_scatter_recursive_doubling_allgather_cnt
    = {
    .id = MPIDI_PTL_Bcast_intra_scatter_recursive_doubling_allgather_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Bcast_intra_scatter_ring_allgather_cnt = {
    .id = MPIDI_PTL_Bcast_intra_scatter_ring_allgather_id
};

/* Reduce default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Reduce_intra_reduce_scatter_gather_cnt = {
    .id = MPIDI_PTL_Reduce_intra_reduce_scatter_gather_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Reduce_intra_binomial_cnt = {
    .id = MPIDI_PTL_Reduce_intra_binomial_id
};

/* Allreduce default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Allreduce_intra_recursive_doubling_cnt = {
    .id = MPIDI_PTL_Allreduce_intra_recursive_doubling_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Allreduce_intra_reduce_scatter_allgather_cnt = {
    .id = MPIDI_PTL_Allreduce_intra_reduce_scatter_allgather_id
};

/* Alltoall default PTL containers initialization */
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Alltoall_intra_pairwise_sendrecv_replace_cnt = {
    .id = MPIDI_PTL_Alltoall_intra_pairwise_sendrecv_replace_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Alltoall_intra_brucks_cnt = {
    .id = MPIDI_PTL_Alltoall_intra_brucks_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Alltoall_intra_scattered_cnt = {
    .id = MPIDI_PTL_Alltoall_intra_scattered_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Alltoall_intra_pairwise_cnt = {
    .id = MPIDI_PTL_Alltoall_intra_pairwise_id
};

/* Alltoallv default PTL containers initialization */
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Alltoallv_intra_pairwise_sendrecv_replace_cnt = {
    .id = MPIDI_PTL_Alltoallv_intra_pairwise_sendrecv_replace_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Alltoallv_intra_scattered_cnt = {
    .id = MPIDI_PTL_Alltoallv_intra_scattered_id
};

/* Alltoallw default PTL containers initialization */
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Alltoallw_intra_pairwise_sendrecv_replace_cnt = {
    .id = MPIDI_PTL_Alltoallw_intra_pairwise_sendrecv_replace_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Alltoallw_intra_scattered_cnt = {
    .id = MPIDI_PTL_Alltoallw_intra_scattered_id
};

/* Allgather default PTL containers initialization */
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Allgather_intra_recursive_doubling_cnt = {
    .id = MPIDI_PTL_Allgather_intra_recursive_doubling_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Allgather_intra_brucks_cnt = {
    .id = MPIDI_PTL_Allgather_intra_brucks_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Allgather_intra_ring_cnt = {
    .id = MPIDI_PTL_Allgather_intra_ring_id
};

/* Allgatherv default PTL containers initialization */
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Allgatherv_intra_recursive_doubling_cnt = {
    .id = MPIDI_PTL_Allgatherv_intra_recursive_doubling_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Allgatherv_intra_brucks_cnt = {
    .id = MPIDI_PTL_Allgatherv_intra_brucks_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Allgatherv_intra_ring_cnt = {
    .id = MPIDI_PTL_Allgatherv_intra_ring_id
};

/* Gather default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Gather_intra_binomial_cnt = {
    .id = MPIDI_PTL_Gather_intra_binomial_id
};

/* Gatherv default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Gatherv_allcomm_linear_cnt = {
    .id = MPIDI_PTL_Gatherv_allcomm_linear_id
};

/* Scatter default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Scatter_intra_binomial_cnt = {
    .id = MPIDI_PTL_Scatter_intra_binomial_id
};

/* Scatterv default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Scatterv_allcomm_linear_cnt = {
    .id = MPIDI_PTL_Scatterv_allcomm_linear_id
};

/* Reduce_scatter default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Reduce_scatter_intra_noncommutative_cnt = {
    .id = MPIDI_PTL_Reduce_scatter_intra_noncommutative_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Reduce_scatter_intra_pairwise_cnt = {
    .id = MPIDI_PTL_Reduce_scatter_intra_pairwise_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Reduce_scatter_intra_recursive_doubling_cnt = {
    .id = MPIDI_PTL_Reduce_scatter_intra_recursive_doubling_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Reduce_scatter_intra_recursive_halving_cnt = {
    .id = MPIDI_PTL_Reduce_scatter_intra_recursive_halving_id
};

/* Reduce_scatter_block default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Reduce_scatter_block_intra_noncommutative_cnt = {
    .id = MPIDI_PTL_Reduce_scatter_block_intra_noncommutative_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Reduce_scatter_block_intra_pairwise_cnt = {
    .id = MPIDI_PTL_Reduce_scatter_block_intra_pairwise_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Reduce_scatter_block_intra_recursive_doubling_cnt = {
    .id = MPIDI_PTL_Reduce_scatter_block_intra_recursive_doubling_id
};

const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Reduce_scatter_block_intra_recursive_halving_cnt = {
    .id = MPIDI_PTL_Reduce_scatter_block_intra_recursive_halving_id
};

/* Scan default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Scan_intra_recursive_doubling_cnt = {
    .id = MPIDI_PTL_Scan_intra_recursive_doubling_id
};

/* Exscan default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t MPIDI_PTL_Exscan_intra_recursive_doubling_cnt = {
    .id = MPIDI_PTL_Exscan_intra_recursive_doubling_id
};
