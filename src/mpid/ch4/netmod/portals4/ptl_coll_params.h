#ifndef PTL_COLL_PARAMS_H_INCLUDED
#define PTL_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_PTL_barrier_recursive_doubling_id,
} MPIDI_PTL_barrier_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_barrier_empty_parameters {
        int empty;
    } PTL_barrier_empty_parameters;
} MPIDI_PTL_barrier_params_t;

typedef enum {
    MPIDI_PTL_bcast_binomial_id,
    MPIDI_PTL_bcast_scatter_doubling_allgather_id,
    MPIDI_PTL_bcast_scatter_ring_allgather_id,
} MPIDI_PTL_bcast_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_bcast_knomial_parameters {
        int radix;
        int block_size;
    } PTL_bcast_knomial_parameters;
    struct MPIDI_PTL_bcast_empty_parameters {
        int empty;
    } PTL_bcast_empty_parameters;
} MPIDI_PTL_bcast_params_t;

typedef enum {
    MPIDI_PTL_allreduce_recursive_doubling_id,
    MPIDI_PTL_allreduce_reduce_scatter_allgather_id
} MPIDI_PTL_allreduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_allreduce_empty_parameters {
        int empty;
    } PTL_allreduce_empty_parameters;
} MPIDI_PTL_allreduce_params_t;

typedef enum {
    MPIDI_PTL_reduce_redscat_gather_id,
    MPIDI_PTL_reduce_binomial_id
} MPIDI_PTL_reduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_reduce_empty_parameters {
        int empty;
    } PTL_reduce_empty_parameters;
} MPIDI_PTL_reduce_params_t;

#define MPIDI_PTL_BARRIER_PARAMS_DECL MPIDI_PTL_barrier_params_t ptl_barrier_params;
#define MPIDI_PTL_BCAST_PARAMS_DECL MPIDI_PTL_bcast_params_t ptl_bcast_params;
#define MPIDI_PTL_REDUCE_PARAMS_DECL MPIDI_PTL_reduce_params_t ptl_reduce_params;
#define MPIDI_PTL_ALLREDUCE_PARAMS_DECL MPIDI_PTL_allreduce_params_t ptl_allreduce_params;

typedef union {
    MPIDI_PTL_BARRIER_PARAMS_DECL;
    MPIDI_PTL_BCAST_PARAMS_DECL;
    MPIDI_PTL_REDUCE_PARAMS_DECL;
    MPIDI_PTL_ALLREDUCE_PARAMS_DECL;
} MPIDI_PTL_coll_params_t;

typedef struct MPIDI_PTL_coll_algo_container {
    int id;
    MPIDI_PTL_coll_params_t params;
} MPIDI_PTL_coll_algo_container_t;

#endif /* PTL_COLL_PARAMS_H_INCLUDED */
