/* Barrier default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_Barrier_intra_dissemination_cnt = {
    .id = MPIDI_OFI_Barrier_intra_dissemination_id
};

/* Bcast default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_Bcast_intra_binomial_cnt = {
    .id = MPIDI_OFI_Bcast_intra_binomial_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Bcast_intra_scatter_recursive_doubling_allgather_cnt = {
    .id = MPIDI_OFI_Bcast_intra_scatter_recursive_doubling_allgather_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Bcast_intra_scatter_ring_allgather_cnt = {
    .id = MPIDI_OFI_Bcast_intra_scatter_ring_allgather_id
};

/* Reduce default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_Reduce_intra_reduce_scatter_gather_cnt = {
    .id = MPIDI_OFI_Reduce_intra_reduce_scatter_gather_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Reduce_intra_binomial_cnt = {
    .id = MPIDI_OFI_Reduce_intra_binomial_id
};

/* Allreduce default OFI containers initialization*/
const MPIDI_OFI_coll_algo_container_t OFI_Allreduce_intra_recursive_doubling_cnt = {
    .id = MPIDI_OFI_Allreduce_intra_recursive_doubling_id
};

const MPIDI_OFI_coll_algo_container_t OFI_Allreduce_intra_reduce_scatter_allgather_cnt = {
    .id = MPIDI_OFI_Allreduce_intra_reduce_scatter_allgather_id
};
