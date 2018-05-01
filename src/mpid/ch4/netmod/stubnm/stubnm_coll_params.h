#ifndef STUBNM_COLL_PARAMS_H_INCLUDED
#define STUBNM_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_STUBNM_Barrier_intra_dissemination_id,
} MPIDI_STUBNM_Barrier_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Barrier_empty_parameters {
        int empty;
    } stubnm_barrier_empty_parameters;
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
    } stubnm_bcast_knomial_parameters;
    struct MPIDI_STUBNM_Bcast_empty_parameters {
        int empty;
    } stubnm_bcast_empty_parameters;
} MPIDI_STUBNM_Bcast_params_t;

typedef enum {
    MPIDI_STUBNM_Allreduce_intra_recursive_doubling_id,
    MPIDI_STUBNM_Allreduce_intra_reduce_scatter_allgather_id
} MPIDI_STUBNM_Allreduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Allreduce_empty_parameters {
        int empty;
    } stubnm_allreduce_empty_parameters;
} MPIDI_STUBNM_Allreduce_params_t;

typedef enum {
    MPIDI_STUBNM_Reduce_intra_reduce_scatter_gather_id,
    MPIDI_STUBNM_Reduce_intra_binomial_id
} MPIDI_STUBNM_Reduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Reduce_empty_parameters {
        int empty;
    } stubnm_reduce_empty_parameters;
} MPIDI_STUBNM_Reduce_params_t;

typedef enum {
    MPIDI_STUBNM_Alltoall_intra_brucks_id,
    MPIDI_STUBNM_Alltoall_intra_scattered_id,
    MPIDI_STUBNM_Alltoall_intra_pairwise_id,
    MPIDI_STUBNM_Alltoall_intra_pairwise_sendrecv_replace_id
} MPIDI_STUBNM_Alltoall_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Alltoall_empty_parameters {
        int empty;
    } stubnm_alltoall_empty_parameters;
} MPIDI_STUBNM_Alltoall_params_t;

typedef enum {
    MPIDI_STUBNM_Alltoallv_intra_pairwise_sendrecv_replace_id,
    MPIDI_STUBNM_Alltoallv_intra_scattered_id
} MPIDI_STUBNM_Alltoallv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Alltoallv_empty_parameters {
        int empty;
    } stubnm_alltoallv_empty_parameters;
} MPIDI_STUBNM_Alltoallv_params_t;

typedef enum {
    MPIDI_STUBNM_Alltoallw_intra_pairwise_sendrecv_replace_id,
    MPIDI_STUBNM_Alltoallw_intra_scattered_id
} MPIDI_STUBNM_Alltoallw_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Alltoallw_empty_parameters {
        int empty;
    } stubnm_alltoallw_empty_parameters;
} MPIDI_STUBNM_Alltoallw_params_t;

typedef enum {
    MPIDI_STUBNM_Allgather_intra_recursive_doubling_id,
    MPIDI_STUBNM_Allgather_intra_brucks_id,
    MPIDI_STUBNM_Allgather_intra_ring_id
} MPIDI_STUBNM_Allgather_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Allgather_empty_parameters {
        int empty;
    } stubnm_allgather_empty_parameters;
} MPIDI_STUBNM_Allgather_params_t;

typedef enum {
    MPIDI_STUBNM_Allgatherv_intra_recursive_doubling_id,
    MPIDI_STUBNM_Allgatherv_intra_brucks_id,
    MPIDI_STUBNM_Allgatherv_intra_ring_id
} MPIDI_STUBNM_Allgatherv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Allgatherv_empty_parameters {
        int empty;
    } stubnm_allgatherv_empty_parameters;
} MPIDI_STUBNM_Allgatherv_params_t;

typedef enum {
    MPIDI_STUBNM_Gather_intra_binomial_id,
} MPIDI_STUBNM_Gather_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Gather_empty_parameters {
        int empty;
    } stubnm_gather_empty_parameters;
} MPIDI_STUBNM_Gather_params_t;

typedef enum {
    MPIDI_STUBNM_Gatherv_allcomm_linear_id,
} MPIDI_STUBNM_Gatherv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Gatherv_empty_parameters {
        int empty;
    } stubnm_gatherv_empty_parameters;
} MPIDI_STUBNM_Gatherv_params_t;

typedef enum {
    MPIDI_STUBNM_Scatter_intra_binomial_id,
} MPIDI_STUBNM_Scatter_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Scatter_empty_parameters {
        int empty;
    } stubnm_scatter_empty_parameters;
} MPIDI_STUBNM_Scatter_params_t;

typedef enum {
    MPIDI_STUBNM_Scatterv_allcomm_linear_id,
} MPIDI_STUBNM_Scatterv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Scatterv_empty_parameters {
        int empty;
    } stubnm_scatterv_empty_parameters;
} MPIDI_STUBNM_Scatterv_params_t;

typedef enum {
    MPIDI_STUBNM_Reduce_scatter_intra_noncommutative_id,
    MPIDI_STUBNM_Reduce_scatter_intra_pairwise_id,
    MPIDI_STUBNM_Reduce_scatter_intra_recursive_doubling_id,
    MPIDI_STUBNM_Reduce_scatter_intra_recursive_halving_id,
} MPIDI_STUBNM_Reduce_scatter_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Reduce_scatter_empty_parameters {
        int empty;
    } stubnm_reduce_scatter_empty_parameters;
} MPIDI_STUBNM_Reduce_scatter_params_t;

typedef enum {
    MPIDI_STUBNM_Reduce_scatter_block_intra_noncommutative_id,
    MPIDI_STUBNM_Reduce_scatter_block_intra_pairwise_id,
    MPIDI_STUBNM_Reduce_scatter_block_intra_recursive_doubling_id,
    MPIDI_STUBNM_Reduce_scatter_block_intra_recursive_halving_id,
} MPIDI_STUBNM_Reduce_scatter_block_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Reduce_scatter_block_empty_parameters {
        int empty;
    } stubnm_reduce_scatter_block_empty_parameters;
} MPIDI_STUBNM_Reduce_scatter_block_params_t;

typedef enum {
    MPIDI_STUBNM_Scan_intra_recursive_doubling_id,
} MPIDI_STUBNM_Scan_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Scan_empty_parameters {
        int empty;
    } stubnm_scan_empty_parameters;
} MPIDI_STUBNM_Scan_params_t;

typedef enum {
    MPIDI_STUBNM_Exscan_intra_recursive_doubling_id,
} MPIDI_STUBNM_Exscan_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_STUBNM_Exscan_empty_parameters {
        int empty;
    } stubnm_exscan_empty_parameters;
} MPIDI_STUBNM_Exscan_params_t;

#define MPIDI_STUBNM_BARRIER_PARAMS_DECL MPIDI_STUBNM_Barrier_params_t stubnm_barrier_params;
#define MPIDI_STUBNM_BCAST_PARAMS_DECL MPIDI_STUBNM_Bcast_params_t stubnm_bcast_params;
#define MPIDI_STUBNM_REDUCE_PARAMS_DECL MPIDI_STUBNM_Reduce_params_t stubnm_reduce_params;
#define MPIDI_STUBNM_ALLREDUCE_PARAMS_DECL MPIDI_STUBNM_Allreduce_params_t stubnm_allreduce_params;
#define MPIDI_STUBNM_ALLTOALL_PARAMS_DECL MPIDI_STUBNM_Alltoall_params_t stubnm_alltoall_params;
#define MPIDI_STUBNM_ALLTOALLV_PARAMS_DECL MPIDI_STUBNM_Alltoallv_params_t stubnm_alltoallv_params;
#define MPIDI_STUBNM_ALLTOALLW_PARAMS_DECL MPIDI_STUBNM_Alltoallw_params_t stubnm_alltoallw_params;
#define MPIDI_STUBNM_ALLGATHER_PARAMS_DECL MPIDI_STUBNM_Allgather_params_t stubnm_allgather_params;
#define MPIDI_STUBNM_ALLGATHERV_PARAMS_DECL MPIDI_STUBNM_Allgatherv_params_t stubnm_allgatherv_params;
#define MPIDI_STUBNM_GATHER_PARAMS_DECL MPIDI_STUBNM_Gather_params_t stubnm_gather_params;
#define MPIDI_STUBNM_GATHERV_PARAMS_DECL MPIDI_STUBNM_Gatherv_params_t stubnm_gatherv_params;
#define MPIDI_STUBNM_SCATTER_PARAMS_DECL MPIDI_STUBNM_Scatter_params_t stubnm_scatter_params;
#define MPIDI_STUBNM_SCATTERV_PARAMS_DECL MPIDI_STUBNM_Scatterv_params_t stubnm_scatterv_params;
#define MPIDI_STUBNM_REDUCE_SCATTER_PARAMS_DECL MPIDI_STUBNM_Reduce_scatter_params_t stubnm_reduce_scatter_params;
#define MPIDI_STUBNM_REDUCE_SCATTER_BLOCK_PARAMS_DECL MPIDI_STUBNM_Reduce_scatter_block_params_t stubnm_reduce_scatter_block_params;
#define MPIDI_STUBNM_SCAN_PARAMS_DECL MPIDI_STUBNM_Scan_params_t stubnm_scan_params;
#define MPIDI_STUBNM_EXSCAN_PARAMS_DECL MPIDI_STUBNM_Exscan_params_t stubnm_exscan_params;

typedef union {
    MPIDI_STUBNM_BARRIER_PARAMS_DECL;
    MPIDI_STUBNM_BCAST_PARAMS_DECL;
    MPIDI_STUBNM_REDUCE_PARAMS_DECL;
    MPIDI_STUBNM_ALLREDUCE_PARAMS_DECL;
    MPIDI_STUBNM_ALLTOALL_PARAMS_DECL;
    MPIDI_STUBNM_ALLTOALLV_PARAMS_DECL;
    MPIDI_STUBNM_ALLTOALLW_PARAMS_DECL;
    MPIDI_STUBNM_ALLGATHER_PARAMS_DECL;
    MPIDI_STUBNM_ALLGATHERV_PARAMS_DECL;
    MPIDI_STUBNM_GATHER_PARAMS_DECL;
    MPIDI_STUBNM_GATHERV_PARAMS_DECL;
    MPIDI_STUBNM_SCATTER_PARAMS_DECL;
    MPIDI_STUBNM_SCATTERV_PARAMS_DECL;
    MPIDI_STUBNM_REDUCE_SCATTER_PARAMS_DECL;
    MPIDI_STUBNM_REDUCE_SCATTER_BLOCK_PARAMS_DECL;
    MPIDI_STUBNM_SCAN_PARAMS_DECL;
    MPIDI_STUBNM_EXSCAN_PARAMS_DECL;
} MPIDI_STUBNM_coll_params_t;

typedef struct MPIDI_STUBNM_coll_algo_container {
    int id;
    MPIDI_STUBNM_coll_params_t params;
} MPIDI_STUBNM_coll_algo_container_t;

#endif /* STUBNM_COLL_PARAMS_H_INCLUDED */
