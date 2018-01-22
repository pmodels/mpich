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

typedef enum {
    MPIDI_PTL_Alltoall_intra_brucks_id,
    MPIDI_PTL_Alltoall_intra_scattered_id,
    MPIDI_PTL_Alltoall_intra_pairwise_id,
    MPIDI_PTL_Alltoall_intra_pairwise_sendrecv_replace_id
} MPIDI_PTL_Alltoall_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Alltoall_empty_parameters {
        int empty;
    } ptl_alltoall_empty_parameters;
} MPIDI_PTL_Alltoall_params_t;

typedef enum {
    MPIDI_PTL_Alltoallv_intra_pairwise_sendrecv_replace_id,
    MPIDI_PTL_Alltoallv_intra_scattered_id
} MPIDI_PTL_Alltoallv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Alltoallv_empty_parameters {
        int empty;
    } ptl_alltoallv_empty_parameters;
} MPIDI_PTL_Alltoallv_params_t;

typedef enum {
    MPIDI_PTL_Alltoallw_intra_pairwise_sendrecv_replace_id,
    MPIDI_PTL_Alltoallw_intra_scattered_id
} MPIDI_PTL_Alltoallw_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Alltoallw_empty_parameters {
        int empty;
    } ptl_alltoallw_empty_parameters;
} MPIDI_PTL_Alltoallw_params_t;

typedef enum {
    MPIDI_PTL_Allgather_intra_recursive_doubling_id,
    MPIDI_PTL_Allgather_intra_brucks_id,
    MPIDI_PTL_Allgather_intra_ring_id
} MPIDI_PTL_Allgather_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Allgather_empty_parameters {
        int empty;
    } ptl_allgather_empty_parameters;
} MPIDI_PTL_Allgather_params_t;

typedef enum {
    MPIDI_PTL_Allgatherv_intra_recursive_doubling_id,
    MPIDI_PTL_Allgatherv_intra_brucks_id,
    MPIDI_PTL_Allgatherv_intra_ring_id
} MPIDI_PTL_Allgatherv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Allgatherv_empty_parameters {
        int empty;
    } ptl_allgatherv_empty_parameters;
} MPIDI_PTL_Allgatherv_params_t;

typedef enum {
    MPIDI_PTL_Gather_intra_binomial_id,
} MPIDI_PTL_Gather_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Gather_empty_parameters {
        int empty;
    } ptl_gather_empty_parameters;
} MPIDI_PTL_Gather_params_t;

typedef enum {
    MPIDI_PTL_Gatherv_allcomm_linear_id,
} MPIDI_PTL_Gatherv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Gatherv_empty_parameters {
        int empty;
    } ptl_gatherv_empty_parameters;
} MPIDI_PTL_Gatherv_params_t;

typedef enum {
    MPIDI_PTL_Scatter_intra_binomial_id,
} MPIDI_PTL_Scatter_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Scatter_empty_parameters {
        int empty;
    } ptl_scatter_empty_parameters;
} MPIDI_PTL_Scatter_params_t;

typedef enum {
    MPIDI_PTL_Scatterv_allcomm_linear_id,
} MPIDI_PTL_Scatterv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Scatterv_empty_parameters {
        int empty;
    } ptl_scatterv_empty_parameters;
} MPIDI_PTL_Scatterv_params_t;

typedef enum {
    MPIDI_PTL_Reduce_scatter_intra_noncommutative_id,
    MPIDI_PTL_Reduce_scatter_intra_pairwise_id,
    MPIDI_PTL_Reduce_scatter_intra_recursive_doubling_id,
    MPIDI_PTL_Reduce_scatter_intra_recursive_halving_id,
} MPIDI_PTL_Reduce_scatter_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Reduce_scatter_empty_parameters {
        int empty;
    } ptl_reduce_scatter_empty_parameters;
} MPIDI_PTL_Reduce_scatter_params_t;

typedef enum {
    MPIDI_PTL_Reduce_scatter_block_intra_noncommutative_id,
    MPIDI_PTL_Reduce_scatter_block_intra_pairwise_id,
    MPIDI_PTL_Reduce_scatter_block_intra_recursive_doubling_id,
    MPIDI_PTL_Reduce_scatter_block_intra_recursive_halving_id,
} MPIDI_PTL_Reduce_scatter_block_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Reduce_scatter_block_empty_parameters {
        int empty;
    } ptl_reduce_scatter_block_empty_parameters;
} MPIDI_PTL_Reduce_scatter_block_params_t;

typedef enum {
    MPIDI_PTL_Scan_intra_recursive_doubling_id,
} MPIDI_PTL_Scan_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Scan_empty_parameters {
        int empty;
    } ptl_scan_empty_parameters;
} MPIDI_PTL_Scan_params_t;

typedef enum {
    MPIDI_PTL_Exscan_intra_recursive_doubling_id,
} MPIDI_PTL_Exscan_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_PTL_Exscan_empty_parameters {
        int empty;
    } ptl_exscan_empty_parameters;
} MPIDI_PTL_Exscan_params_t;

#define MPIDI_PTL_BARRIER_PARAMS_DECL MPIDI_PTL_Barrier_params_t ptl_barrier_params;
#define MPIDI_PTL_BCAST_PARAMS_DECL MPIDI_PTL_Bcast_params_t ptl_bcast_params;
#define MPIDI_PTL_REDUCE_PARAMS_DECL MPIDI_PTL_Reduce_params_t ptl_reduce_params;
#define MPIDI_PTL_ALLREDUCE_PARAMS_DECL MPIDI_PTL_Allreduce_params_t ptl_allreduce_params;
#define MPIDI_PTL_ALLTOALL_PARAMS_DECL MPIDI_PTL_Alltoall_params_t ptl_alltoall_params;
#define MPIDI_PTL_ALLTOALLV_PARAMS_DECL MPIDI_PTL_Alltoallv_params_t ptl_alltoallv_params;
#define MPIDI_PTL_ALLTOALLW_PARAMS_DECL MPIDI_PTL_Alltoallw_params_t ptl_alltoallw_params;
#define MPIDI_PTL_ALLGATHER_PARAMS_DECL MPIDI_PTL_Allgather_params_t ptl_allgather_params;
#define MPIDI_PTL_ALLGATHERV_PARAMS_DECL MPIDI_PTL_Allgatherv_params_t ptl_allgatherv_params;
#define MPIDI_PTL_GATHER_PARAMS_DECL MPIDI_PTL_Gather_params_t ptl_gather_params;
#define MPIDI_PTL_GATHERV_PARAMS_DECL MPIDI_PTL_Gatherv_params_t ptl_gatherv_params;
#define MPIDI_PTL_SCATTER_PARAMS_DECL MPIDI_PTL_Scatter_params_t ptl_scatter_params;
#define MPIDI_PTL_SCATTERV_PARAMS_DECL MPIDI_PTL_Scatterv_params_t ptl_scatterv_params;
#define MPIDI_PTL_REDUCE_SCATTER_PARAMS_DECL MPIDI_PTL_Reduce_scatter_params_t ptl_reduce_scatter_params;
#define MPIDI_PTL_REDUCE_SCATTER_BLOCK_PARAMS_DECL MPIDI_PTL_Reduce_scatter_block_params_t ptl_reduce_scatter_block_params;
#define MPIDI_PTL_SCAN_PARAMS_DECL MPIDI_PTL_Scan_params_t ptl_scan_params;
#define MPIDI_PTL_EXSCAN_PARAMS_DECL MPIDI_PTL_Exscan_params_t ptl_exscan_params;

typedef union {
    MPIDI_PTL_BARRIER_PARAMS_DECL;
    MPIDI_PTL_BCAST_PARAMS_DECL;
    MPIDI_PTL_REDUCE_PARAMS_DECL;
    MPIDI_PTL_ALLREDUCE_PARAMS_DECL;
    MPIDI_PTL_ALLTOALL_PARAMS_DECL;
    MPIDI_PTL_ALLTOALLV_PARAMS_DECL;
    MPIDI_PTL_ALLTOALLW_PARAMS_DECL;
    MPIDI_PTL_ALLGATHER_PARAMS_DECL;
    MPIDI_PTL_ALLGATHERV_PARAMS_DECL;
    MPIDI_PTL_GATHER_PARAMS_DECL;
    MPIDI_PTL_GATHERV_PARAMS_DECL;
    MPIDI_PTL_SCATTER_PARAMS_DECL;
    MPIDI_PTL_SCATTERV_PARAMS_DECL;
    MPIDI_PTL_REDUCE_SCATTER_PARAMS_DECL;
    MPIDI_PTL_REDUCE_SCATTER_BLOCK_PARAMS_DECL;
    MPIDI_PTL_SCAN_PARAMS_DECL;
    MPIDI_PTL_EXSCAN_PARAMS_DECL;
} MPIDI_PTL_coll_params_t;

typedef struct MPIDI_PTL_coll_algo_container {
    int id;
    MPIDI_PTL_coll_params_t params;
} MPIDI_PTL_coll_algo_container_t;

#endif /* PTL_COLL_PARAMS_H_INCLUDED */
