#ifndef STUBSHM_COLL_PARAMS_H_INCLUDED
#define STUBSHM_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_STUBSHM_Barrier_intra_dissemination_id,
} MPIDI_STUBSHM_Barrier_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Barrier_empty_parameters {
        int empty;
    } stubshm_barrier_empty_parameters;
} MPIDI_STUBSHM_Barrier_params_t;

typedef enum {
    MPIDI_STUBSHM_Bcast_intra_binomial_id,
    MPIDI_STUBSHM_Bcast_intra_scatter_recursive_doubling_allgather_id,
    MPIDI_STUBSHM_Bcast_intra_scatter_ring_allgather_id
} MPIDI_STUBSHM_Bcast_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Bcast_knomial_parameters {
        int radix;
        int block_size;
    } stubshm_bcast_knomial_parameters;
    struct MPIDI_STUBSHM_Bcast_empty_parameters {
        int empty;
    } stubshm_bcast_empty_parameters;
} MPIDI_STUBSHM_Bcast_params_t;

typedef enum {
    MPIDI_STUBSHM_Reduce_intra_reduce_scatter_gather_id,
    MPIDI_STUBSHM_Reduce_intra_binomial_id
} MPIDI_STUBSHM_Reduce_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Reduce_empty_parameters {
        int empty;
    } stubshm_reduce_empty_parameters;
} MPIDI_STUBSHM_Reduce_params_t;

typedef enum {
    MPIDI_STUBSHM_Allreduce_intra_recursive_doubling_id,
    MPIDI_STUBSHM_Allreduce_intra_reduce_scatter_allgather_id
} MPIDI_STUBSHM_Allreduce_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Allreduce_empty_parameters {
        int empty;
    } stubshm_allreduce_empty_parameters;
} MPIDI_STUBSHM_Allreduce_params_t;

#define MPIDI_STUBSHM_BARRIER_PARAMS_DECL MPIDI_STUBSHM_Barrier_params_t stubshm_barrier_params
#define MPIDI_STUBSHM_BCAST_PARAMS_DECL MPIDI_STUBSHM_Bcast_params_t stubshm_bcast_params
#define MPIDI_STUBSHM_REDUCE_PARAMS_DECL MPIDI_STUBSHM_Reduce_params_t stubshm_reduce_params
#define MPIDI_STUBSHM_ALLREDUCE_PARAMS_DECL MPIDI_STUBSHM_Allreduce_params_t stubshm_allreduce_params

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
