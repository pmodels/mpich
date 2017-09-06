#ifndef UCX_COLL_PARAMS_H_INCLUDED
#define UCX_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_UCX_barrier_recursive_doubling_id,
} MPIDI_UCX_barrier_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_barrier_empty_parameters {
        int empty;
    } UCX_barrier_empty_parameters;
} MPIDI_UCX_barrier_params_t;

typedef enum {
    MPIDI_UCX_bcast_binomial_id,
    MPIDI_UCX_bcast_scatter_doubling_allgather_id,
    MPIDI_UCX_bcast_scatter_ring_allgather_id,
} MPIDI_UCX_bcast_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_bcast_knomial_parameters {
        int radix;
        int block_size;
    } UCX_bcast_knomial_parameters;
    struct MPIDI_UCX_bcast_empty_parameters {
        int empty;
    } UCX_bcast_empty_parameters;
} MPIDI_UCX_bcast_params_t;

typedef enum {
    MPIDI_UCX_allreduce_recursive_doubling_id,
    MPIDI_UCX_allreduce_reduce_scatter_allgather_id
} MPIDI_UCX_allreduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_allreduce_empty_parameters {
        int empty;
    } UCX_allreduce_empty_parameters;
} MPIDI_UCX_allreduce_params_t;

typedef enum {
    MPIDI_UCX_reduce_redscat_gather_id,
    MPIDI_UCX_reduce_binomial_id
} MPIDI_UCX_reduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_reduce_empty_parameters {
        int empty;
    } UCX_reduce_empty_parameters;
} MPIDI_UCX_reduce_params_t;

#define MPIDI_UCX_BARRIER_PARAMS_DECL MPIDI_UCX_barrier_params_t ucx_barrier_params;
#define MPIDI_UCX_BCAST_PARAMS_DECL MPIDI_UCX_bcast_params_t ucx_bcast_params;
#define MPIDI_UCX_REDUCE_PARAMS_DECL MPIDI_UCX_reduce_params_t ucx_reduce_params;
#define MPIDI_UCX_ALLREDUCE_PARAMS_DECL MPIDI_UCX_allreduce_params_t ucx_allreduce_params;

typedef union {
    MPIDI_UCX_BARRIER_PARAMS_DECL;
    MPIDI_UCX_BCAST_PARAMS_DECL;
    MPIDI_UCX_REDUCE_PARAMS_DECL;
    MPIDI_UCX_ALLREDUCE_PARAMS_DECL;
} MPIDI_UCX_coll_params_t;

typedef struct MPIDI_UCX_coll_algo_container {
    int id;
    MPIDI_UCX_coll_params_t params;
} MPIDI_UCX_coll_algo_container_t;

#endif /* UCX_COLL_PARAMS_H_INCLUDED */
