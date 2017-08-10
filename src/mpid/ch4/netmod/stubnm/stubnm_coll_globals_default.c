/* Barrier default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t STUBNM_barrier_recursive_doubling_cnt =
    {
        .id = MPIDI_STUBNM_barrier_recursive_doubling_id
    };

/* Bcast default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t STUBNM_bcast_binomial_cnt =
    {
        .id = MPIDI_STUBNM_bcast_binomial_id
    };

const MPIDI_STUBNM_coll_algo_container_t STUBNM_bcast_scatter_doubling_allgather_cnt =
    {
        .id = MPIDI_STUBNM_bcast_scatter_doubling_allgather_id
    };

const MPIDI_STUBNM_coll_algo_container_t STUBNM_bcast_scatter_ring_allgather_cnt =
    {
        .id = MPIDI_STUBNM_bcast_scatter_ring_allgather_id
    };

/* Reduce default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t STUBNM_reduce_redscat_gather_cnt =
    {
        .id = MPIDI_STUBNM_reduce_redscat_gather_id
    };

const MPIDI_STUBNM_coll_algo_container_t STUBNM_reduce_binomial_cnt =
    {
        .id = MPIDI_STUBNM_reduce_binomial_id
    };

/* Allreduce default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t STUBNM_allreduce_recursive_doubling_cnt =
    {
        .id = MPIDI_STUBNM_allreduce_recursive_doubling_id
    };

const MPIDI_STUBNM_coll_algo_container_t STUBNM_allreduce_reduce_scatter_allgather_cnt =
    {
        .id = MPIDI_STUBNM_allreduce_reduce_scatter_allgather_id
    };
