#ifndef PTL_COLL_PARAMS_H_INCLUDED
#define PTL_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_PTL_Barrier_intra_dissemination_id,
} MPIDI_PTL_Barrier_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Barrier_empty_parameters {
        int empty;
    } ptl_barrier_empty_parameters;
} MPIDI_PTL_Barrier_params_t;

typedef enum {
    MPIDI_PTL_Bcast_intra_binomial_id,
    MPIDI_PTL_Bcast_intra_scatter_recursive_doubling_allgather_id,
    MPIDI_PTL_Bcast_intra_scatter_ring_allgather_id,
} MPIDI_PTL_Bcast_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Bcast_knomial_parameters {
        int radix;
        int block_size;
    } ptl_bcast_knomial_parameters;
    struct MPIDI_PTL_Bcast_empty_parameters {
        int empty;
    } ptl_bcast_empty_parameters;
} MPIDI_PTL_Bcast_params_t;

typedef enum {
    MPIDI_PTL_Allreduce_intra_recursive_doubling_id,
    MPIDI_PTL_Allreduce_intra_reduce_scatter_allgather_id
} MPIDI_PTL_Allreduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Allreduce_empty_parameters {
        int empty;
    } ptl_allreduce_empty_parameters;
} MPIDI_PTL_Allreduce_params_t;

typedef enum {
    MPIDI_PTL_Reduce_intra_reduce_scatter_gather_id,
    MPIDI_PTL_Reduce_intra_binomial_id
} MPIDI_PTL_Reduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Reduce_empty_parameters {
        int empty;
    } ptl_reduce_empty_parameters;
} MPIDI_PTL_Reduce_params_t;

#define MPIDI_PTL_BARRIER_PARAMS_DECL MPIDI_PTL_Barrier_params_t ptl_barrier_params
#define MPIDI_PTL_BCAST_PARAMS_DECL MPIDI_PTL_Bcast_params_t ptl_bcast_params
#define MPIDI_PTL_REDUCE_PARAMS_DECL MPIDI_PTL_Reduce_params_t ptl_reduce_params
#define MPIDI_PTL_ALLREDUCE_PARAMS_DECL MPIDI_PTL_Allreduce_params_t ptl_allreduce_params

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
