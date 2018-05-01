/* Barrier default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Barrier_intra_dissemination_cnt = {
    .id = MPIDI_OFI_Barrier_intra_dissemination_id
};

/* Bcast default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Bcast_intra_binomial_cnt = {
    .id = MPIDI_OFI_Bcast_intra_binomial_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Bcast_intra_scatter_recursive_doubling_allgather_cnt
    = {
    .id = MPIDI_OFI_Bcast_intra_scatter_recursive_doubling_allgather_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Bcast_intra_scatter_ring_allgather_cnt = {
    .id = MPIDI_OFI_Bcast_intra_scatter_ring_allgather_id
};

/* Reduce default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Reduce_intra_reduce_scatter_gather_cnt = {
    .id = MPIDI_OFI_Reduce_intra_reduce_scatter_gather_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Reduce_intra_binomial_cnt = {
    .id = MPIDI_OFI_Reduce_intra_binomial_id
};

/* Allreduce default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Allreduce_intra_recursive_doubling_cnt = {
    .id = MPIDI_OFI_Allreduce_intra_recursive_doubling_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Allreduce_intra_reduce_scatter_allgather_cnt = {
    .id = MPIDI_OFI_Allreduce_intra_reduce_scatter_allgather_id
};

/* Alltoall default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Alltoall_intra_pairwise_sendrecv_replace_cnt = {
    .id = MPIDI_OFI_Alltoall_intra_pairwise_sendrecv_replace_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Alltoall_intra_brucks_cnt = {
    .id = MPIDI_OFI_Alltoall_intra_brucks_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Alltoall_intra_scattered_cnt = {
    .id = MPIDI_OFI_Alltoall_intra_scattered_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Alltoall_intra_pairwise_cnt = {
    .id = MPIDI_OFI_Alltoall_intra_pairwise_id
};

/* Alltoallv default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Alltoallv_intra_pairwise_sendrecv_replace_cnt = {
    .id = MPIDI_OFI_Alltoallv_intra_pairwise_sendrecv_replace_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Alltoallv_intra_scattered_cnt = {
    .id = MPIDI_OFI_Alltoallv_intra_scattered_id
};

/* Alltoallw default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Alltoallw_intra_pairwise_sendrecv_replace_cnt = {
    .id = MPIDI_OFI_Alltoallw_intra_pairwise_sendrecv_replace_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Alltoallw_intra_scattered_cnt = {
    .id = MPIDI_OFI_Alltoallw_intra_scattered_id
};

/* Allgather default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Allgather_intra_recursive_doubling_cnt = {
    .id = MPIDI_OFI_Allgather_intra_recursive_doubling_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Allgather_intra_brucks_cnt = {
    .id = MPIDI_OFI_Allgather_intra_brucks_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Allgather_intra_ring_cnt = {
    .id = MPIDI_OFI_Allgather_intra_ring_id
};

/* Allgatherv default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Allgatherv_intra_recursive_doubling_cnt = {
    .id = MPIDI_OFI_Allgatherv_intra_recursive_doubling_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Allgatherv_intra_brucks_cnt = {
    .id = MPIDI_OFI_Allgatherv_intra_brucks_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Allgatherv_intra_ring_cnt = {
    .id = MPIDI_OFI_Allgatherv_intra_ring_id
};

/* Gather default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Gather_intra_binomial_cnt = {
    .id = MPIDI_OFI_Gather_intra_binomial_id
};

/* Gatherv default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Gatherv_allcomm_linear_cnt = {
    .id = MPIDI_OFI_Gatherv_allcomm_linear_id
};

/* Scatter default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Scatter_intra_binomial_cnt = {
    .id = MPIDI_OFI_Scatter_intra_binomial_id
};

/* Scatterv default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Scatterv_allcomm_linear_cnt = {
    .id = MPIDI_OFI_Scatterv_allcomm_linear_id
};

/* Reduce_scatter default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Reduce_scatter_intra_noncommutative_cnt = {
    .id = MPIDI_OFI_Reduce_scatter_intra_noncommutative_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Reduce_scatter_intra_pairwise_cnt = {
    .id = MPIDI_OFI_Reduce_scatter_intra_pairwise_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Reduce_scatter_intra_recursive_doubling_cnt = {
    .id = MPIDI_OFI_Reduce_scatter_intra_recursive_doubling_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Reduce_scatter_intra_recursive_halving_cnt = {
    .id = MPIDI_OFI_Reduce_scatter_intra_recursive_halving_id
};

/* Reduce_scatter_block default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Reduce_scatter_block_intra_noncommutative_cnt = {
    .id = MPIDI_OFI_Reduce_scatter_block_intra_noncommutative_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Reduce_scatter_block_intra_pairwise_cnt = {
    .id = MPIDI_OFI_Reduce_scatter_block_intra_pairwise_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Reduce_scatter_block_intra_recursive_doubling_cnt = {
    .id = MPIDI_OFI_Reduce_scatter_block_intra_recursive_doubling_id
};

const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Reduce_scatter_block_intra_recursive_halving_cnt = {
    .id = MPIDI_OFI_Reduce_scatter_block_intra_recursive_halving_id
};

/* Scan default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Scan_intra_recursive_doubling_cnt = {
    .id = MPIDI_OFI_Scan_intra_recursive_doubling_id
};

/* Exscan default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t MPIDI_OFI_Exscan_intra_recursive_doubling_cnt = {
    .id = MPIDI_OFI_Exscan_intra_recursive_doubling_id
};
