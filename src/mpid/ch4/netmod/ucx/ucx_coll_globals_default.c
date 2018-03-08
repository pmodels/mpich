/* Barrier default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Barrier_intra_dissemination_cnt = {
    .id = MPIDI_UCX_Barrier_intra_dissemination_id
};

/* Bcast default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Bcast_intra_binomial_cnt = {
    .id = MPIDI_UCX_Bcast_intra_binomial_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Bcast_intra_scatter_recursive_doubling_allgather_cnt
    = {
    .id = MPIDI_UCX_Bcast_intra_scatter_recursive_doubling_allgather_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Bcast_intra_scatter_ring_allgather_cnt = {
    .id = MPIDI_UCX_Bcast_intra_scatter_ring_allgather_id
};

/* Reduce default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Reduce_intra_reduce_scatter_gather_cnt = {
    .id = MPIDI_UCX_Reduce_intra_reduce_scatter_gather_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Reduce_intra_binomial_cnt = {
    .id = MPIDI_UCX_Reduce_intra_binomial_id
};

/* Allreduce default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Allreduce_intra_recursive_doubling_cnt = {
    .id = MPIDI_UCX_Allreduce_intra_recursive_doubling_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Allreduce_intra_reduce_scatter_allgather_cnt = {
    .id = MPIDI_UCX_Allreduce_intra_reduce_scatter_allgather_id
};

/* Alltoall default UCX containers initialization */
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Alltoall_intra_pairwise_sendrecv_replace_cnt = {
    .id = MPIDI_UCX_Alltoall_intra_pairwise_sendrecv_replace_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Alltoall_intra_brucks_cnt = {
    .id = MPIDI_UCX_Alltoall_intra_brucks_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Alltoall_intra_scattered_cnt = {
    .id = MPIDI_UCX_Alltoall_intra_scattered_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Alltoall_intra_pairwise_cnt = {
    .id = MPIDI_UCX_Alltoall_intra_pairwise_id
};

/* Alltoallv default UCX containers initialization */
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Alltoallv_intra_pairwise_sendrecv_replace_cnt = {
    .id = MPIDI_UCX_Alltoallv_intra_pairwise_sendrecv_replace_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Alltoallv_intra_scattered_cnt = {
    .id = MPIDI_UCX_Alltoallv_intra_scattered_id
};

/* Alltoallw default UCX containers initialization */
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Alltoallw_intra_pairwise_sendrecv_replace_cnt = {
    .id = MPIDI_UCX_Alltoallw_intra_pairwise_sendrecv_replace_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Alltoallw_intra_scattered_cnt = {
    .id = MPIDI_UCX_Alltoallw_intra_scattered_id
};

/* Allgather default UCX containers initialization */
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Allgather_intra_recursive_doubling_cnt = {
    .id = MPIDI_UCX_Allgather_intra_recursive_doubling_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Allgather_intra_brucks_cnt = {
    .id = MPIDI_UCX_Allgather_intra_brucks_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Allgather_intra_ring_cnt = {
    .id = MPIDI_UCX_Allgather_intra_ring_id
};

/* Allgatherv default UCX containers initialization */
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Allgatherv_intra_recursive_doubling_cnt = {
    .id = MPIDI_UCX_Allgatherv_intra_recursive_doubling_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Allgatherv_intra_brucks_cnt = {
    .id = MPIDI_UCX_Allgatherv_intra_brucks_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Allgatherv_intra_ring_cnt = {
    .id = MPIDI_UCX_Allgatherv_intra_ring_id
};

/* Gather default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Gather_intra_binomial_cnt = {
    .id = MPIDI_UCX_Gather_intra_binomial_id
};

/* Gatherv default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Gatherv_allcomm_linear_cnt = {
    .id = MPIDI_UCX_Gatherv_allcomm_linear_id
};

/* Scatter default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Scatter_intra_binomial_cnt = {
    .id = MPIDI_UCX_Scatter_intra_binomial_id
};

/* Scatterv default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Scatterv_allcomm_linear_cnt = {
    .id = MPIDI_UCX_Scatterv_allcomm_linear_id
};

/* Reduce_scatter default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Reduce_scatter_intra_noncommutative_cnt = {
    .id = MPIDI_UCX_Reduce_scatter_intra_noncommutative_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Reduce_scatter_intra_pairwise_cnt = {
    .id = MPIDI_UCX_Reduce_scatter_intra_pairwise_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Reduce_scatter_intra_recursive_doubling_cnt = {
    .id = MPIDI_UCX_Reduce_scatter_intra_recursive_doubling_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Reduce_scatter_intra_recursive_halving_cnt = {
    .id = MPIDI_UCX_Reduce_scatter_intra_recursive_halving_id
};

/* Reduce_scatter_block default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Reduce_scatter_block_intra_noncommutative_cnt = {
    .id = MPIDI_UCX_Reduce_scatter_block_intra_noncommutative_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Reduce_scatter_block_intra_pairwise_cnt = {
    .id = MPIDI_UCX_Reduce_scatter_block_intra_pairwise_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Reduce_scatter_block_intra_recursive_doubling_cnt = {
    .id = MPIDI_UCX_Reduce_scatter_block_intra_recursive_doubling_id
};

const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Reduce_scatter_block_intra_recursive_halving_cnt = {
    .id = MPIDI_UCX_Reduce_scatter_block_intra_recursive_halving_id
};

/* Scan default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Scan_intra_recursive_doubling_cnt = {
    .id = MPIDI_UCX_Scan_intra_recursive_doubling_id
};

/* Exscan default UCX containers initialization*/
const MPIDI_UCX_coll_algo_container_t MPIDI_UCX_Exscan_intra_recursive_doubling_cnt = {
    .id = MPIDI_UCX_Exscan_intra_recursive_doubling_id
};
