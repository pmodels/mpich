#ifndef STUBSHM_COLL_PARAMS_H_INCLUDED
#define STUBSHM_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_STUBSHM_barrier_recursive_doubling_id,
} MPIDI_STUBSHM_barrier_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_barrier_empty_parameters {
        int empty;
    } STUBSHM_barrier_empty_parameters;
} MPIDI_STUBSHM_barrier_params_t;

typedef enum {
    MPIDI_STUBSHM_bcast_binomial_id,
    MPIDI_STUBSHM_bcast_scatter_doubling_allgather_id,
    MPIDI_STUBSHM_bcast_scatter_ring_allgather_id
} MPIDI_STUBSHM_bcast_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_bcast_knomial_parameters {
        int radix;
        int block_size;
    } STUBSHM_bcast_knomial_parameters;
    struct MPIDI_STUBSHM_bcast_empty_parameters {
        int empty;
    } STUBSHM_bcast_empty_parameters;
} MPIDI_STUBSHM_bcast_params_t;

typedef enum {
    MPIDI_STUBSHM_reduce_redscat_gather_id,
    MPIDI_STUBSHM_reduce_binomial_id
} MPIDI_STUBSHM_reduce_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_reduce_empty_parameters {
        int empty;
    } STUBSHM_reduce_empty_parameters;
} MPIDI_STUBSHM_reduce_params_t;

typedef enum {
    MPIDI_STUBSHM_allreduce_recursive_doubling_id,
    MPIDI_STUBSHM_allreduce_reduce_scatter_allgather_id
} MPIDI_STUBSHM_allreduce_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_allreduce_empty_parameters {
        int empty;
    } STUBSHM_allreduce_empty_parameters;
} MPIDI_STUBSHM_allreduce_params_t;

#define MPIDI_STUBSHM_BARRIER_PARAMS_DECL MPIDI_STUBSHM_barrier_params_t stubshm_barrier_params;
#define MPIDI_STUBSHM_BCAST_PARAMS_DECL MPIDI_STUBSHM_bcast_params_t stubshm_bcast_params;
#define MPIDI_STUBSHM_REDUCE_PARAMS_DECL MPIDI_STUBSHM_reduce_params_t stubshm_reduce_params;
#define MPIDI_STUBSHM_ALLREDUCE_PARAMS_DECL MPIDI_STUBSHM_allreduce_params_t stubshm_allreduce_params;

typedef union {
    MPIDI_STUBSHM_BARRIER_PARAMS_DECL;
    MPIDI_STUBSHM_BCAST_PARAMS_DECL;
    MPIDI_STUBSHM_REDUCE_PARAMS_DECL;
    MPIDI_STUBSHM_ALLREDUCE_PARAMS_DECL;
} MPIDI_STUBSHM_coll_params_t;

typedef struct MPIDI_STUBSHM_coll_algo_container {
    int id;
    MPIDI_STUBSHM_coll_params_t params;
} MPIDI_STUBSHM_coll_algo_container_t;

#endif /* STUBSHM_COLL_PARAMS_H_INCLUDED */
