/* Barrier default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Barrier_intra_dissemination_cnt = {
    .id = MPIDI_STUBSHM_Barrier_intra_dissemination_id
};

/* Bcast default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Bcast_intra_binomial_cnt = {
    .id = MPIDI_STUBSHM_Bcast_intra_binomial_id
};

const MPIDI_STUBSHM_coll_algo_container_t
    MPIDI_STUBSHM_Bcast_intra_scatter_recursive_doubling_allgather_cnt = {
    .id = MPIDI_STUBSHM_Bcast_intra_scatter_recursive_doubling_allgather_id
};

const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Bcast_intra_scatter_ring_allgather_cnt = {
    .id = MPIDI_STUBSHM_Bcast_intra_scatter_ring_allgather_id
};

/* Reduce default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Reduce_intra_reduce_scatter_gather_cnt = {
    .id = MPIDI_STUBSHM_Reduce_intra_reduce_scatter_gather_id
};

const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Reduce_intra_binomial_cnt = {
    .id = MPIDI_STUBSHM_Reduce_intra_binomial_id
};

/* Allreduce default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Allreduce_intra_recursive_doubling_cnt = {
    .id = MPIDI_STUBSHM_Allreduce_intra_recursive_doubling_id
};

const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Allreduce_intra_reduce_scatter_allgather_cnt
    = {
    .id = MPIDI_STUBSHM_Allreduce_intra_reduce_scatter_allgather_id
};

/* Alltoall default STUBSHM containers initialization */
const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Alltoall_intra_pairwise_sendrecv_replace_cnt
    = {
    .id = MPIDI_STUBSHM_Alltoall_intra_pairwise_sendrecv_replace_id
};

const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Alltoall_intra_brucks_cnt = {
    .id = MPIDI_STUBSHM_Alltoall_intra_brucks_id
};

const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Alltoall_intra_scattered_cnt = {
    .id = MPIDI_STUBSHM_Alltoall_intra_scattered_id
};

const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Alltoall_intra_pairwise_cnt = {
    .id = MPIDI_STUBSHM_Alltoall_intra_pairwise_id
};

/* Alltoallv default STUBSHM containers initialization */
const MPIDI_STUBSHM_coll_algo_container_t
    MPIDI_STUBSHM_Alltoallv_intra_pairwise_sendrecv_replace_cnt = {
    .id = MPIDI_STUBSHM_Alltoallv_intra_pairwise_sendrecv_replace_id
};

const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Alltoallv_intra_scattered_cnt = {
    .id = MPIDI_STUBSHM_Alltoallv_intra_scattered_id
};

/* Alltoallw default STUBSHM containers initialization */
const MPIDI_STUBSHM_coll_algo_container_t
    MPIDI_STUBSHM_Alltoallw_intra_pairwise_sendrecv_replace_cnt = {
    .id = MPIDI_STUBSHM_Alltoallw_intra_pairwise_sendrecv_replace_id
};

const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Alltoallw_intra_scattered_cnt = {
    .id = MPIDI_STUBSHM_Alltoallw_intra_scattered_id
};

/* Allgather default STUBSHM containers initialization */
const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Allgather_intra_recursive_doubling_cnt = {
    .id = MPIDI_STUBSHM_Allgather_intra_recursive_doubling_id
};

const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Allgather_intra_brucks_cnt = {
    .id = MPIDI_STUBSHM_Allgather_intra_brucks_id
};

const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Allgather_intra_ring_cnt = {
    .id = MPIDI_STUBSHM_Allgather_intra_ring_id
};

/* Allgatherv default STUBSHM containers initialization */
const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Allgatherv_intra_recursive_doubling_cnt = {
    .id = MPIDI_STUBSHM_Allgatherv_intra_recursive_doubling_id
};

const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Allgatherv_intra_brucks_cnt = {
    .id = MPIDI_STUBSHM_Allgatherv_intra_brucks_id
};

const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Allgatherv_intra_ring_cnt = {
    .id = MPIDI_STUBSHM_Allgatherv_intra_ring_id
};

/* Gather default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Gather_intra_binomial_cnt = {
    .id = MPIDI_STUBSHM_Gather_intra_binomial_id
};

/* Gatherv default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Gatherv_allcomm_linear_cnt = {
    .id = MPIDI_STUBSHM_Gatherv_allcomm_linear_id
};

/* Scatter default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Scatter_intra_binomial_cnt = {
    .id = MPIDI_STUBSHM_Scatter_intra_binomial_id
};

/* Scatterv default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Scatterv_allcomm_linear_cnt = {
    .id = MPIDI_STUBSHM_Scatterv_allcomm_linear_id
};

/* Reduce_scatter default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Reduce_scatter_intra_noncommutative_cnt = {
    .id = MPIDI_STUBSHM_Reduce_scatter_intra_noncommutative_id
};

const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Reduce_scatter_intra_pairwise_cnt = {
    .id = MPIDI_STUBSHM_Reduce_scatter_intra_pairwise_id
};

const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Reduce_scatter_intra_recursive_doubling_cnt
    = {
    .id = MPIDI_STUBSHM_Reduce_scatter_intra_recursive_doubling_id
};

const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Reduce_scatter_intra_recursive_halving_cnt = {
    .id = MPIDI_STUBSHM_Reduce_scatter_intra_recursive_halving_id
};

/* Reduce_scatter_block default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t
    MPIDI_STUBSHM_Reduce_scatter_block_intra_noncommutative_cnt = {
    .id = MPIDI_STUBSHM_Reduce_scatter_block_intra_noncommutative_id
};

const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Reduce_scatter_block_intra_pairwise_cnt = {
    .id = MPIDI_STUBSHM_Reduce_scatter_block_intra_pairwise_id
};

const MPIDI_STUBSHM_coll_algo_container_t
    MPIDI_STUBSHM_Reduce_scatter_block_intra_recursive_doubling_cnt = {
    .id = MPIDI_STUBSHM_Reduce_scatter_block_intra_recursive_doubling_id
};

const MPIDI_STUBSHM_coll_algo_container_t
    MPIDI_STUBSHM_Reduce_scatter_block_intra_recursive_halving_cnt = {
    .id = MPIDI_STUBSHM_Reduce_scatter_block_intra_recursive_halving_id
};

/* Scan default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Scan_intra_recursive_doubling_cnt = {
    .id = MPIDI_STUBSHM_Scan_intra_recursive_doubling_id
};

/* Exscan default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t MPIDI_STUBSHM_Exscan_intra_recursive_doubling_cnt = {
    .id = MPIDI_STUBSHM_Exscan_intra_recursive_doubling_id
};
