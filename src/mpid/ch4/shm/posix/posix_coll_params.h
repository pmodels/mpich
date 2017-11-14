#ifndef POSIX_COLL_PARAMS_H_INCLUDED
#define POSIX_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_POSIX_barrier_recursive_doubling_id,
} MPIDI_POSIX_barrier_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_POSIX_barrier_empty_parameters {
        int empty;
    } posix_barrier_empty_parameters;
} MPIDI_POSIX_barrier_params_t;

typedef enum {
    MPIDI_POSIX_bcast_binomial_id,
    MPIDI_POSIX_bcast_scatter_doubling_allgather_id,
    MPIDI_POSIX_bcast_scatter_ring_allgather_id
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
    MPIDI_POSIX_reduce_redscat_gather_id,
    MPIDI_POSIX_reduce_binomial_id
} MPIDI_POSIX_reduce_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_POSIX_reduce_empty_parameters {
        int empty;
    } posix_reduce_empty_parameters;
} MPIDI_POSIX_reduce_params_t;

typedef enum {
    MPIDI_POSIX_allreduce_recursive_doubling_id,
    MPIDI_POSIX_allreduce_reduce_scatter_allgather_id
} MPIDI_POSIX_allreduce_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_POSIX_allreduce_empty_parameters {
        int empty;
    } posix_allreduce_empty_parameters;
} MPIDI_POSIX_allreduce_params_t;

typedef enum {
    MPIDI_POSIX_alltoall_bruck_id,
    MPIDI_POSIX_alltoall_isend_irecv_id,
    MPIDI_POSIX_alltoall_pairwise_exchange_id,
    MPIDI_POSIX_alltoall_sendrecv_replace_id
} MPIDI_POSIX_alltoall_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_alltoall_empty_parameters {
        int empty;
    } POSIX_alltoall_empty_parameters;
} MPIDI_POSIX_alltoall_params_t;

typedef enum {
    MPIDI_POSIX_alltoallv_sendrecv_replace_id,
    MPIDI_POSIX_alltoallv_isend_irecv_id
} MPIDI_POSIX_alltoallv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_alltoallv_empty_parameters {
        int empty;
    } POSIX_alltoallv_empty_parameters;
} MPIDI_POSIX_alltoallv_params_t;

typedef enum {
    MPIDI_POSIX_alltoallw_sendrecv_replace_id,
    MPIDI_POSIX_alltoallw_isend_irecv_id,
    MPIDI_POSIX_alltoallw_pairwise_exchange_id
} MPIDI_POSIX_alltoallw_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_alltoallw_empty_parameters {
        int empty;
    } POSIX_alltoallw_empty_parameters;
} MPIDI_POSIX_alltoallw_params_t;

typedef enum {
    MPIDI_POSIX_allgather_recursive_doubling_id,
    MPIDI_POSIX_allgather_bruck_id,
    MPIDI_POSIX_allgather_ring_id
} MPIDI_POSIX_allgather_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_allgather_empty_parameters {
        int empty;
    } POSIX_allgather_empty_parameters;
} MPIDI_POSIX_allgather_params_t;

typedef enum {
    MPIDI_POSIX_allgatherv_recursive_doubling_id,
    MPIDI_POSIX_allgatherv_bruck_id,
    MPIDI_POSIX_allgatherv_ring_id
} MPIDI_POSIX_allgatherv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_allgatherv_empty_parameters {
        int empty;
    } POSIX_allgatherv_empty_parameters;
} MPIDI_POSIX_allgatherv_params_t;

#define MPIDI_POSIX_BARRIER_PARAMS_DECL MPIDI_POSIX_barrier_params_t posix_barrier_params;
#define MPIDI_POSIX_BCAST_PARAMS_DECL MPIDI_POSIX_bcast_params_t posix_bcast_params;
#define MPIDI_POSIX_REDUCE_PARAMS_DECL MPIDI_POSIX_reduce_params_t posix_reduce_params;
#define MPIDI_POSIX_ALLREDUCE_PARAMS_DECL MPIDI_POSIX_allreduce_params_t posix_allreduce_params;
#define MPIDI_POSIX_ALLTOALL_PARAMS_DECL MPIDI_POSIX_alltoall_params_t posix_alltoall_params;
#define MPIDI_POSIX_ALLTOALLV_PARAMS_DECL MPIDI_POSIX_alltoallv_params_t posix_alltoallv_params;
#define MPIDI_POSIX_ALLTOALLW_PARAMS_DECL MPIDI_POSIX_alltoallw_params_t posix_alltoallw_params;
#define MPIDI_POSIX_ALLGATHER_PARAMS_DECL MPIDI_POSIX_allgather_params_t posix_allgather_params;
#define MPIDI_POSIX_ALLGATHERV_PARAMS_DECL MPIDI_POSIX_allgatherv_params_t posix_allgatherv_params;

typedef union {
    MPIDI_POSIX_BARRIER_PARAMS_DECL;
    MPIDI_POSIX_BCAST_PARAMS_DECL;
    MPIDI_POSIX_REDUCE_PARAMS_DECL;
    MPIDI_POSIX_ALLREDUCE_PARAMS_DECL;
    MPIDI_POSIX_ALLTOALL_PARAMS_DECL;
    MPIDI_POSIX_ALLTOALLV_PARAMS_DECL;
    MPIDI_POSIX_ALLTOALLW_PARAMS_DECL;
    MPIDI_POSIX_ALLGATHER_PARAMS_DECL;
    MPIDI_POSIX_ALLGATHERV_PARAMS_DECL;
} MPIDI_POSIX_coll_params_t;

typedef struct MPIDI_POSIX_coll_algo_container {
    int id;
    MPIDI_POSIX_coll_params_t params;
} MPIDI_POSIX_coll_algo_container_t;

#endif /* POSIX_COLL_PARAMS_H_INCLUDED */
