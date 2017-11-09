#ifndef POSIX_COLL_PARAMS_H_INCLUDED
#define POSIX_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_POSIX_Barrier_intra_recursive_doubling_id,
} MPIDI_POSIX_Barrier_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_Barrier_empty_parameters {
        int empty;
    } POSIX_barrier_empty_parameters;
} MPIDI_POSIX_Barrier_params_t;

typedef enum {
    MPIDI_POSIX_Bcast_intra_binomial_id,
    MPIDI_POSIX_Bcast_intra_scatter_doubling_allgather_id,
    MPIDI_POSIX_Bcast_intra_scatter_ring_allgather_id,
} MPIDI_POSIX_Bcast_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_Bcast_knomial_parameters {
        int radix;
        int block_size;
    } POSIX_bcast_knomial_parameters;
    struct MPIDI_POSIX_Bcast_empty_parameters {
        int empty;
    } POSIX_bcast_empty_parameters;
} MPIDI_POSIX_Bcast_params_t;

typedef enum {
    MPIDI_POSIX_Allreduce_intra_recursive_doubling_id,
    MPIDI_POSIX_Allreduce_intra_reduce_scatter_allgather_id
} MPIDI_POSIX_Allreduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_Allreduce_empty_parameters {
        int empty;
    } POSIX_allreduce_empty_parameters;
} MPIDI_POSIX_Allreduce_params_t;

typedef enum {
    MPIDI_POSIX_Reduce_intra_redscat_gather_id,
    MPIDI_POSIX_Reduce_intra_binomial_id
} MPIDI_POSIX_Reduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_Reduce_empty_parameters {
        int empty;
    } POSIX_reduce_empty_parameters;
} MPIDI_POSIX_Reduce_params_t;

typedef enum {
    MPIDI_POSIX_Alltoall_intra_brucks_id,
    MPIDI_POSIX_Alltoall_intra_scattered_id,
    MPIDI_POSIX_Alltoall_intra_pairwise_id,
    MPIDI_POSIX_Alltoall_intra_pairwise_sendrecv_replace_id
} MPIDI_POSIX_Alltoall_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_Alltoall_empty_parameters {
        int empty;
    } POSIX_alltoall_empty_parameters;
} MPIDI_POSIX_Alltoall_params_t;

typedef enum {
    MPIDI_POSIX_Alltoallv_intra_pairwise_sendrecv_replace_id,
    MPIDI_POSIX_Alltoallv_intra_scattered_id
} MPIDI_POSIX_Alltoallv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_Alltoallv_empty_parameters {
        int empty;
    } POSIX_alltoallv_empty_parameters;
} MPIDI_POSIX_Alltoallv_params_t;

typedef enum {
    MPIDI_POSIX_Alltoallw_intra_pairwise_sendrecv_replace_id,
    MPIDI_POSIX_Alltoallw_intra_scattered_id
} MPIDI_POSIX_Alltoallw_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_Alltoallw_empty_parameters {
        int empty;
    } POSIX_alltoallw_empty_parameters;
} MPIDI_POSIX_Alltoallw_params_t;

typedef enum {
    MPIDI_POSIX_Allgather_intra_recursive_doubling_id,
    MPIDI_POSIX_Allgather_intra_brucks_id,
    MPIDI_POSIX_Allgather_intra_ring_id
} MPIDI_POSIX_Allgather_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_Allgather_empty_parameters {
        int empty;
    } POSIX_allgather_empty_parameters;
} MPIDI_POSIX_Allgather_params_t;

typedef enum {
    MPIDI_POSIX_Allgatherv_intra_recursive_doubling_id,
    MPIDI_POSIX_Allgatherv_intra_brucks_id,
    MPIDI_POSIX_Allgatherv_intra_ring_id
} MPIDI_POSIX_Allgatherv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_Allgatherv_empty_parameters {
        int empty;
    } POSIX_allgatherv_empty_parameters;
} MPIDI_POSIX_Allgatherv_params_t;

typedef enum {
    MPIDI_POSIX_Gather_intra_binomial_id,
    MPIDI_POSIX_Gather_intra_binomial_indexed_id
} MPIDI_POSIX_Gather_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_Gather_empty_parameters {
        int empty;
    } POSIX_gather_empty_parameters;
} MPIDI_POSIX_Gather_params_t;

typedef enum {
    MPIDI_POSIX_Gatherv_intra_linear_ssend_id,
    MPIDI_POSIX_Gatherv_intra_linear_id
} MPIDI_POSIX_Gatherv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_Gatherv_empty_parameters {
        int empty;
    } POSIX_gatherv_empty_parameters;
} MPIDI_POSIX_Gatherv_params_t;

typedef enum {
    MPIDI_POSIX_Scatter_intra_binomial_id,
} MPIDI_POSIX_Scatter_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_Scatter_empty_parameters {
        int empty;
    } POSIX_scatter_empty_parameters;
} MPIDI_POSIX_Scatter_params_t;

typedef enum {
    MPIDI_POSIX_Scatterv_intra_linear_id,
} MPIDI_POSIX_Scatterv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_POSIX_Scatterv_empty_parameters {
        int empty;
    } POSIX_scatterv_empty_parameters;
} MPIDI_POSIX_Scatterv_params_t;

#define MPIDI_POSIX_BARRIER_PARAMS_DECL MPIDI_POSIX_Barrier_params_t posix_barrier_params;
#define MPIDI_POSIX_BCAST_PARAMS_DECL MPIDI_POSIX_Bcast_params_t posix_bcast_params;
#define MPIDI_POSIX_REDUCE_PARAMS_DECL MPIDI_POSIX_Reduce_params_t posix_reduce_params;
#define MPIDI_POSIX_ALLREDUCE_PARAMS_DECL MPIDI_POSIX_Allreduce_params_t posix_allreduce_params;
#define MPIDI_POSIX_ALLTOALL_PARAMS_DECL MPIDI_POSIX_Alltoall_params_t posix_alltoall_params;
#define MPIDI_POSIX_ALLTOALLV_PARAMS_DECL MPIDI_POSIX_Alltoallv_params_t posix_alltoallv_params;
#define MPIDI_POSIX_ALLTOALLW_PARAMS_DECL MPIDI_POSIX_Alltoallw_params_t posix_alltoallw_params;
#define MPIDI_POSIX_ALLGATHER_PARAMS_DECL MPIDI_POSIX_Allgather_params_t posix_allgather_params;
#define MPIDI_POSIX_ALLGATHERV_PARAMS_DECL MPIDI_POSIX_Allgatherv_params_t posix_allgatherv_params;
#define MPIDI_POSIX_GATHER_PARAMS_DECL MPIDI_POSIX_Gather_params_t posix_gather_params;
#define MPIDI_POSIX_GATHERV_PARAMS_DECL MPIDI_POSIX_Gatherv_params_t posix_gatherv_params;
#define MPIDI_POSIX_SCATTER_PARAMS_DECL MPIDI_POSIX_Scatter_params_t posix_scatter_params;
#define MPIDI_POSIX_SCATTERV_PARAMS_DECL MPIDI_POSIX_Scatterv_params_t posix_scatterv_params;

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
    MPIDI_POSIX_GATHER_PARAMS_DECL;
    MPIDI_POSIX_GATHERV_PARAMS_DECL;
    MPIDI_POSIX_SCATTER_PARAMS_DECL;
    MPIDI_POSIX_SCATTERV_PARAMS_DECL;
} MPIDI_POSIX_coll_params_t;

typedef struct MPIDI_POSIX_coll_algo_container {
    int id;
    MPIDI_POSIX_coll_params_t params;
} MPIDI_POSIX_coll_algo_container_t;

#endif /* POSIX_COLL_PARAMS_H_INCLUDED */
