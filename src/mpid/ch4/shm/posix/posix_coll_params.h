#ifndef POSIX_COLL_PARAMS_H_INCLUDED
#define POSIX_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_POSIX_barrier__recursive_doubling_id,
} MPIDI_POSIX_barrier_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_POSIX_barrier_empty_parameters {
        int empty;
    } posix_barrier_empty_parameters;
} MPIDI_POSIX_barrier_params_t;

typedef enum {
    MPIDI_POSIX_bcast__binomial_id,
    MPIDI_POSIX_bcast__scatter_recursive_doubling_allgather_id,
    MPIDI_POSIX_bcast__scatter_ring_allgather_id
} MPIDI_POSIX_bcast_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_POSIX_bcast_knomial_parameters {
        int radix;
        int block_size;
    } posix_bcast_knomial_parameters;
    struct MPIDI_POSIX_bcast_empty_parameters {
        int empty;
    } posix_bcast_empty_parameters;
} MPIDI_POSIX_bcast_params_t;

typedef enum {
    MPIDI_POSIX_reduce__reduce_scatter_gather_id,
    MPIDI_POSIX_reduce__binomial_id
} MPIDI_POSIX_reduce_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_POSIX_reduce_empty_parameters {
        int empty;
    } posix_reduce_empty_parameters;
} MPIDI_POSIX_reduce_params_t;

typedef enum {
    MPIDI_POSIX_allreduce__recursive_doubling_id,
    MPIDI_POSIX_allreduce__reduce_scatter_allgather_id
} MPIDI_POSIX_allreduce_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_POSIX_allreduce_empty_parameters {
        int empty;
    } posix_allreduce_empty_parameters;
} MPIDI_POSIX_allreduce_params_t;

#define MPIDI_POSIX_BARRIER_PARAMS_DECL MPIDI_POSIX_barrier_params_t posix_barrier_params
#define MPIDI_POSIX_BCAST_PARAMS_DECL MPIDI_POSIX_bcast_params_t posix_bcast_params
#define MPIDI_POSIX_REDUCE_PARAMS_DECL MPIDI_POSIX_reduce_params_t posix_reduce_params
#define MPIDI_POSIX_ALLREDUCE_PARAMS_DECL MPIDI_POSIX_allreduce_params_t posix_allreduce_params

typedef union {
    MPIDI_POSIX_BARRIER_PARAMS_DECL;
    MPIDI_POSIX_BCAST_PARAMS_DECL;
    MPIDI_POSIX_REDUCE_PARAMS_DECL;
    MPIDI_POSIX_ALLREDUCE_PARAMS_DECL;
} MPIDI_POSIX_coll_params_t;

typedef struct MPIDI_POSIX_coll_algo_container {
    int id;
    MPIDI_POSIX_coll_params_t params;
} MPIDI_POSIX_coll_algo_container_t;

#endif /* POSIX_COLL_PARAMS_H_INCLUDED */
