/* Barrier default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_Barrier__intra__dissemination_cnt = {
    .id = MPIDI_OFI_Barrier__intra__dissemination_id
};

/* Bcast default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_Bcast__intra__binomial_cnt = {
    .id = MPIDI_OFI_Bcast__intra__binomial_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Bcast__intra__scatter_recursive_doubling_allgather_cnt = {
    .id = MPIDI_OFI_Bcast__intra__scatter_recursive_doubling_allgather_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Bcast__intra__scatter_ring_allgather_cnt = {
    .id = MPIDI_OFI_Bcast__intra__scatter_ring_allgather_id
};

/* Reduce default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_Reduce__intra__reduce_scatter_gather_cnt = {
    .id = MPIDI_OFI_Reduce__intra__reduce_scatter_gather_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Reduce__intra__binomial_cnt = {
    .id = MPIDI_OFI_Reduce__intra__binomial_id
};

/* Allreduce default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t OFI_Allreduce__intra__recursive_doubling_cnt = {
    .id = MPIDI_OFI_Allreduce__intra__recursive_doubling_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Allreduce__intra__reduce_scatter_allgather_cnt = {
    .id = MPIDI_OFI_Allreduce__intra__reduce_scatter_allgather_id
};

/* Alltoall default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t OFI_Alltoall__intra__pairwise_sendrecv_replace_cnt = {
    .id = MPIDI_OFI_Alltoall__intra__pairwise_sendrecv_replace_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Alltoall__intra__brucks_cnt = {
    .id = MPIDI_OFI_Alltoall__intra__brucks_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Alltoall__intra__scattered_cnt = {
    .id = MPIDI_OFI_Alltoall__intra__scattered_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Alltoall__intra__pairwise_cnt = {
    .id = MPIDI_OFI_Alltoall__intra__pairwise_id
};

/* Alltoallv default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t OFI_Alltoallv__intra__pairwise_sendrecv_replace_cnt = {
    .id = MPIDI_OFI_Alltoallv__intra__pairwise_sendrecv_replace_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Alltoallv__intra__scattered_cnt = {
    .id = MPIDI_OFI_Alltoallv__intra__scattered_id
};

/* Alltoallw default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t OFI_Alltoallw__intra__pairwise_sendrecv_replace_cnt = {
    .id = MPIDI_OFI_Alltoallw__intra__pairwise_sendrecv_replace_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Alltoallw__intra__scattered_cnt = {
    .id = MPIDI_OFI_Alltoallw__intra__scattered_id
};

/* Allgather default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t OFI_Allgather__intra__recursive_doubling_cnt = {
    .id = MPIDI_OFI_Allgather__intra__recursive_doubling_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Allgather__intra__brucks_cnt = {
    .id = MPIDI_OFI_Allgather__intra__brucks_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Allgather__intra__ring_cnt = {
    .id = MPIDI_OFI_Allgather__intra__ring_id
};

/* Allgatherv default OFI containers initialization */
const MPIDI_OFI_coll_algo_container_t OFI_Allgatherv__intra__recursive_doubling_cnt = {
    .id = MPIDI_OFI_Allgatherv__intra__recursive_doubling_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Allgatherv__intra__brucks_cnt = {
    .id = MPIDI_OFI_Allgatherv__intra__brucks_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Allgatherv__intra__ring_cnt = {
    .id = MPIDI_OFI_Allgatherv__intra__ring_id
};

/* Gather default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_Gather__intra__binomial_cnt = {
    .id = MPIDI_OFI_Gather__intra__binomial_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Gather__intra__binomial_indexed_cnt = {
    .id = MPIDI_OFI_Gather__intra__binomial_indexed_id
};

/* Gatherv default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_Gatherv__intra__linear_ssend_cnt = {
    .id = MPIDI_OFI_Gatherv__intra__linear_ssend_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Gatherv__intra__linear_cnt = {
    .id = MPIDI_OFI_Gatherv__intra__linear_id
};

/* Scatter default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_Scatter__intra__binomial_cnt = {
    .id = MPIDI_OFI_Scatter__intra__binomial_id
};

/* Scatterv default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_Scatterv__intra__linear_cnt = {
    .id = MPIDI_OFI_Scatterv__intra__linear_id
};

/* Reduce_scatter default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_Reduce_scatter__intra__noncommutative_cnt = {
    .id = MPIDI_OFI_Reduce_scatter__intra__noncommutative_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Reduce_scatter__intra__pairwise_cnt = {
    .id = MPIDI_OFI_Reduce_scatter__intra__pairwise_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Reduce_scatter__intra__recursive_doubling_cnt = {
    .id = MPIDI_OFI_Reduce_scatter__intra__recursive_doubling_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Reduce_scatter__intra__recursive_halving_cnt = {
    .id = MPIDI_OFI_Reduce_scatter__intra__recursive_halving_id
};

/* Reduce_scatter_block default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_Reduce_scatter_block__intra__noncommutative_cnt = {
    .id = MPIDI_OFI_Reduce_scatter_block__intra__noncommutative_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Reduce_scatter_block__intra__pairwise_cnt = {
    .id = MPIDI_OFI_Reduce_scatter_block__intra__pairwise_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Reduce_scatter_block__intra__recursive_doubling_cnt = {
    .id = MPIDI_OFI_Reduce_scatter_block__intra__recursive_doubling_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Reduce_scatter_block__intra__recursive_halving_cnt = {
    .id = MPIDI_OFI_Reduce_scatter_block__intra__recursive_halving_id
};

/* Scan default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_Scan__intra__recursive_doubling_cnt = {
    .id = MPIDI_OFI_Scan__intra__recursive_doubling_id
};

/* Exscan default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_Exscan__intra__recursive_doubling_cnt = {
    .id = MPIDI_OFI_Exscan__intra__recursive_doubling_id
};
