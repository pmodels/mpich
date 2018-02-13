#ifndef STUBNM_COLL_PARAMS_H_INCLUDED
#define STUBNM_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_STUBNM_Barrier_intra_recursive_doubling_id,
} MPIDI_STUBNM_Barrier_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Barrier_empty_parameters {
        int empty;
    } STUBNM_Barrier_empty_parameters;
} MPIDI_STUBNM_Barrier_params_t;

typedef enum {
    MPIDI_STUBNM_Bcast_intra_binomial_id,
    MPIDI_STUBNM_Bcast_intra_scatter_recursive_doubling_allgather_id,
    MPIDI_STUBNM_Bcast_intra_scatter_ring_allgather_id,
} MPIDI_STUBNM_Bcast_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Bcast_knomial_parameters {
        int radix;
        int block_size;
    } STUBNM_Bcast_knomial_parameters;
    struct MPIDI_STUBNM_Bcast_empty_parameters {
        int empty;
    } STUBNM_Bcast_empty_parameters;
} MPIDI_STUBNM_Bcast_params_t;

typedef enum {
    MPIDI_STUBNM_Allreduce_intra_recursive_doubling_id,
    MPIDI_STUBNM_Allreduce_intra_reduce_scatter_allgather_id
} MPIDI_STUBNM_Allreduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Allreduce_empty_parameters {
        int empty;
    } STUBNM_Allreduce_empty_parameters;
} MPIDI_STUBNM_Allreduce_params_t;

typedef enum {
    MPIDI_STUBNM_Reduce_intra_reduce_scatter_gather_id,
    MPIDI_STUBNM_Reduce_intra_binomial_id
} MPIDI_STUBNM_Reduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Reduce_empty_parameters {
        int empty;
    } STUBNM_Reduce_empty_parameters;
} MPIDI_STUBNM_Reduce_params_t;

#define MPIDI_STUBNM_BARRIER_PARAMS_DECL MPIDI_STUBNM_Barrier_params_t stubnm_barrier_params
#define MPIDI_STUBNM_BCAST_PARAMS_DECL MPIDI_STUBNM_Bcast_params_t stubnm_bcast_params
#define MPIDI_STUBNM_REDUCE_PARAMS_DECL MPIDI_STUBNM_Reduce_params_t stubnm_reduce_params
#define MPIDI_STUBNM_ALLREDUCE_PARAMS_DECL MPIDI_STUBNM_Allreduce_params_t stubnm_allreduce_params

typedef union {
    MPIDI_STUBNM_BARRIER_PARAMS_DECL;
    MPIDI_STUBNM_BCAST_PARAMS_DECL;
    MPIDI_STUBNM_REDUCE_PARAMS_DECL;
    MPIDI_STUBNM_ALLREDUCE_PARAMS_DECL;
} MPIDI_STUBNM_coll_params_t;

typedef struct MPIDI_STUBNM_coll_algo_container {
    int id;
    MPIDI_STUBNM_coll_params_t params;
} MPIDI_STUBNM_coll_algo_container_t;

#endif /* STUBNM_COLL_PARAMS_H_INCLUDED */
