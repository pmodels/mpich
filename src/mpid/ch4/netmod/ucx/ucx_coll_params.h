#ifndef UCX_COLL_PARAMS_H_INCLUDED
#define UCX_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_UCX_Barrier_intra_dissemination_id,
} MPIDI_UCX_Barrier_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Barrier_empty_parameters {
        int empty;
    } UCX_Barrier_empty_parameters;
} MPIDI_UCX_Barrier_params_t;

typedef enum {
    MPIDI_UCX_Bcast_intra_binomial_id,
    MPIDI_UCX_Bcast_intra_scatter_recursive_doubling_allgather_id,
    MPIDI_UCX_Bcast_intra_scatter_ring_allgather_id,
} MPIDI_UCX_Bcast_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Bcast_knomial_parameters {
        int radix;
        int block_size;
    } UCX_Bcast_knomial_parameters;
    struct MPIDI_UCX_Bcast_empty_parameters {
        int empty;
    } UCX_Bcast_empty_parameters;
} MPIDI_UCX_Bcast_params_t;

typedef enum {
    MPIDI_UCX_Allreduce_intra_recursive_doubling_id,
    MPIDI_UCX_Allreduce_intra_reduce_scatter_allgather_id
} MPIDI_UCX_Allreduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Allreduce_empty_parameters {
        int empty;
    } UCX_Allreduce_empty_parameters;
} MPIDI_UCX_Allreduce_params_t;

typedef enum {
    MPIDI_UCX_Reduce_intra_reduce_scatter_gather_id,
    MPIDI_UCX_Reduce_intra_binomial_id
} MPIDI_UCX_Reduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Reduce_empty_parameters {
        int empty;
    } UCX_Reduce_empty_parameters;
} MPIDI_UCX_Reduce_params_t;

#define MPIDI_UCX_BARRIER_PARAMS_DECL MPIDI_UCX_Barrier_params_t ucx_barrier_params
#define MPIDI_UCX_BCAST_PARAMS_DECL MPIDI_UCX_Bcast_params_t ucx_bcast_params
#define MPIDI_UCX_REDUCE_PARAMS_DECL MPIDI_UCX_Reduce_params_t ucx_reduce_params
#define MPIDI_UCX_ALLREDUCE_PARAMS_DECL MPIDI_UCX_Allreduce_params_t ucx_allreduce_params

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
