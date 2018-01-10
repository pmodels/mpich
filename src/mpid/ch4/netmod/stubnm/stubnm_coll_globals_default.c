/* Barrier default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t STUBNM_barrier__recursive_doubling_cnt =
    {
        .id = MPIDI_STUBNM_barrier__recursive_doubling_id
    };

/* Bcast default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t STUBNM_bcast__binomial_cnt =
    {
        .id = MPIDI_STUBNM_bcast__binomial_id
    };

const MPIDI_STUBNM_coll_algo_container_t STUBNM_bcast__scatter_recursive_doubling_allgather_cnt =
    {
        .id = MPIDI_STUBNM_bcast__scatter_recursive_doubling_allgather_id
    };

const MPIDI_STUBNM_coll_algo_container_t STUBNM_bcast__scatter_ring_allgather_cnt =
    {
        .id = MPIDI_STUBNM_bcast__scatter_ring_allgather_id
    };

/* Reduce default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t STUBNM_reduce__reduce_scatter_gather_cnt =
    {
        .id = MPIDI_STUBNM_reduce__reduce_scatter_gather_id
    };

const MPIDI_STUBNM_coll_algo_container_t STUBNM_reduce__binomial_cnt =
    {
        .id = MPIDI_STUBNM_reduce__binomial_id
    };

/* Allreduce default STUBNM containers initialization*/
const MPIDI_STUBNM_coll_algo_container_t STUBNM_allreduce__recursive_doubling_cnt =
    {
        .id = MPIDI_STUBNM_allreduce__recursive_doubling_id
    };

const MPIDI_STUBNM_coll_algo_container_t STUBNM_allreduce__reduce_scatter_allgather_cnt =
    {
        .id = MPIDI_STUBNM_allreduce__reduce_scatter_allgather_id
    };
