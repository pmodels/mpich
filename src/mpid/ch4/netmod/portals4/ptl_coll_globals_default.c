/* Barrier default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t PTL_barrier_recursive_doubling_cnt =
    {
        .id = MPIDI_PTL_barrier_recursive_doubling_id
    };

/* Bcast default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t PTL_bcast_binomial_cnt =
    {
        .id = MPIDI_PTL_bcast_binomial_id
    };

const MPIDI_PTL_coll_algo_container_t PTL_bcast_scatter_doubling_allgather_cnt =
    {
        .id = MPIDI_PTL_bcast_scatter_doubling_allgather_id
    };

const MPIDI_PTL_coll_algo_container_t PTL_bcast_scatter_ring_allgather_cnt =
    {
        .id = MPIDI_PTL_bcast_scatter_ring_allgather_id
    };

/* Reduce default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t PTL_reduce_redscat_gather_cnt =
    {
        .id = MPIDI_PTL_reduce_redscat_gather_id
    };

const MPIDI_PTL_coll_algo_container_t PTL_reduce_binomial_cnt =
    {
        .id = MPIDI_PTL_reduce_binomial_id
    };

/* Allreduce default PTL containers initialization*/
const MPIDI_PTL_coll_algo_container_t PTL_allreduce_recursive_doubling_cnt =
    {
        .id = MPIDI_PTL_allreduce_recursive_doubling_id
    };

const MPIDI_PTL_coll_algo_container_t PTL_allreduce_reduce_scatter_allgather_cnt =
    {
        .id = MPIDI_PTL_allreduce_reduce_scatter_allgather_id
    };
