/* Barrier default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_barrier_recursive_doubling_cnt =
    {
        .id = MPIDI_STUBSHM_barrier_recursive_doubling_id
    };

/* Bcast default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_bcast_binomial_cnt =
    {
        .id = MPIDI_STUBSHM_bcast_binomial_id
    };

const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_bcast_scatter_doubling_allgather_cnt =
    {
        .id = MPIDI_STUBSHM_bcast_scatter_doubling_allgather_id
    };

const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_bcast_scatter_ring_allgather_cnt =
    {
        .id = MPIDI_STUBSHM_bcast_scatter_ring_allgather_id
    };

/* Reduce default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_reduce_redscat_gather_cnt =
    {
        .id = MPIDI_STUBSHM_reduce_redscat_gather_id
    };

const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_reduce_binomial_cnt =
    {
        .id = MPIDI_STUBSHM_reduce_binomial_id
    };

/* Allreduce default STUBSHM containers initialization*/
const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_allreduce_recursive_doubling_cnt =
    {
        .id = MPIDI_STUBSHM_allreduce_recursive_doubling_id
    };

const MPIDI_STUBSHM_coll_algo_container_t STUBSHM_allreduce_reduce_scatter_allgather_cnt =
    {
        .id = MPIDI_STUBSHM_allreduce_reduce_scatter_allgather_id
    };
