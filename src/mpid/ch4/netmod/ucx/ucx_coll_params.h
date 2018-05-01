#ifndef UCX_COLL_PARAMS_H_INCLUDED
#define UCX_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_UCX_Barrier_intra_dissemination_id,
} MPIDI_UCX_Barrier_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Barrier_empty_parameters {
        int empty;
    } ucx_barrier_empty_parameters;
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
    } ucx_bcast_knomial_parameters;
    struct MPIDI_UCX_Bcast_empty_parameters {
        int empty;
    } ucx_bcast_empty_parameters;
} MPIDI_UCX_Bcast_params_t;

typedef enum {
    MPIDI_UCX_Allreduce_intra_recursive_doubling_id,
    MPIDI_UCX_Allreduce_intra_reduce_scatter_allgather_id
} MPIDI_UCX_Allreduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Allreduce_empty_parameters {
        int empty;
    } ucx_allreduce_empty_parameters;
} MPIDI_UCX_Allreduce_params_t;

typedef enum {
    MPIDI_UCX_Reduce_intra_reduce_scatter_gather_id,
    MPIDI_UCX_Reduce_intra_binomial_id
} MPIDI_UCX_Reduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Reduce_empty_parameters {
        int empty;
    } ucx_reduce_empty_parameters;
} MPIDI_UCX_Reduce_params_t;

typedef enum {
    MPIDI_UCX_Alltoall_intra_brucks_id,
    MPIDI_UCX_Alltoall_intra_scattered_id,
    MPIDI_UCX_Alltoall_intra_pairwise_id,
    MPIDI_UCX_Alltoall_intra_pairwise_sendrecv_replace_id
} MPIDI_UCX_Alltoall_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Alltoall_empty_parameters {
        int empty;
    } ucx_alltoall_empty_parameters;
} MPIDI_UCX_Alltoall_params_t;

typedef enum {
    MPIDI_UCX_Alltoallv_intra_pairwise_sendrecv_replace_id,
    MPIDI_UCX_Alltoallv_intra_scattered_id
} MPIDI_UCX_Alltoallv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Alltoallv_empty_parameters {
        int empty;
    } ucx_alltoallv_empty_parameters;
} MPIDI_UCX_Alltoallv_params_t;

typedef enum {
    MPIDI_UCX_Alltoallw_intra_pairwise_sendrecv_replace_id,
    MPIDI_UCX_Alltoallw_intra_scattered_id
} MPIDI_UCX_Alltoallw_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Alltoallw_empty_parameters {
        int empty;
    } ucx_alltoallw_empty_parameters;
} MPIDI_UCX_Alltoallw_params_t;

typedef enum {
    MPIDI_UCX_Allgather_intra_recursive_doubling_id,
    MPIDI_UCX_Allgather_intra_brucks_id,
    MPIDI_UCX_Allgather_intra_ring_id
} MPIDI_UCX_Allgather_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Allgather_empty_parameters {
        int empty;
    } ucx_allgather_empty_parameters;
} MPIDI_UCX_Allgather_params_t;

typedef enum {
    MPIDI_UCX_Allgatherv_intra_recursive_doubling_id,
    MPIDI_UCX_Allgatherv_intra_brucks_id,
    MPIDI_UCX_Allgatherv_intra_ring_id
} MPIDI_UCX_Allgatherv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Allgatherv_empty_parameters {
        int empty;
    } ucx_allgatherv_empty_parameters;
} MPIDI_UCX_Allgatherv_params_t;

typedef enum {
    MPIDI_UCX_Gather_intra_binomial_id,
} MPIDI_UCX_Gather_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Gather_empty_parameters {
        int empty;
    } ucx_gather_empty_parameters;
} MPIDI_UCX_Gather_params_t;

typedef enum {
    MPIDI_UCX_Gatherv_allcomm_linear_id,
} MPIDI_UCX_Gatherv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Gatherv_empty_parameters {
        int empty;
    } ucx_gatherv_empty_parameters;
} MPIDI_UCX_Gatherv_params_t;

typedef enum {
    MPIDI_UCX_Scatter_intra_binomial_id,
} MPIDI_UCX_Scatter_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Scatter_empty_parameters {
        int empty;
    } ucx_scatter_empty_parameters;
} MPIDI_UCX_Scatter_params_t;

typedef enum {
    MPIDI_UCX_Scatterv_allcomm_linear_id,
} MPIDI_UCX_Scatterv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Scatterv_empty_parameters {
        int empty;
    } ucx_scatterv_empty_parameters;
} MPIDI_UCX_Scatterv_params_t;

typedef enum {
    MPIDI_UCX_Reduce_scatter_intra_noncommutative_id,
    MPIDI_UCX_Reduce_scatter_intra_pairwise_id,
    MPIDI_UCX_Reduce_scatter_intra_recursive_doubling_id,
    MPIDI_UCX_Reduce_scatter_intra_recursive_halving_id,
} MPIDI_UCX_Reduce_scatter_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Reduce_scatter_empty_parameters {
        int empty;
    } ucx_reduce_scatter_empty_parameters;
} MPIDI_UCX_Reduce_scatter_params_t;

typedef enum {
    MPIDI_UCX_Reduce_scatter_block_intra_noncommutative_id,
    MPIDI_UCX_Reduce_scatter_block_intra_pairwise_id,
    MPIDI_UCX_Reduce_scatter_block_intra_recursive_doubling_id,
    MPIDI_UCX_Reduce_scatter_block_intra_recursive_halving_id,
} MPIDI_UCX_Reduce_scatter_block_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Reduce_scatter_block_empty_parameters {
        int empty;
    } ucx_reduce_scatter_block_empty_parameters;
} MPIDI_UCX_Reduce_scatter_block_params_t;

typedef enum {
    MPIDI_UCX_Scan_intra_recursive_doubling_id,
} MPIDI_UCX_Scan_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Scan_empty_parameters {
        int empty;
    } ucx_scan_empty_parameters;
} MPIDI_UCX_Scan_params_t;

typedef enum {
    MPIDI_UCX_Exscan_intra_recursive_doubling_id,
} MPIDI_UCX_Exscan_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_UCX_Exscan_empty_parameters {
        int empty;
    } ucx_exscan_empty_parameters;
} MPIDI_UCX_Exscan_params_t;

#define MPIDI_UCX_BARRIER_PARAMS_DECL MPIDI_UCX_Barrier_params_t ucx_barrier_params;
#define MPIDI_UCX_BCAST_PARAMS_DECL MPIDI_UCX_Bcast_params_t ucx_bcast_params;
#define MPIDI_UCX_REDUCE_PARAMS_DECL MPIDI_UCX_Reduce_params_t ucx_reduce_params;
#define MPIDI_UCX_ALLREDUCE_PARAMS_DECL MPIDI_UCX_Allreduce_params_t ucx_allreduce_params;
#define MPIDI_UCX_ALLTOALL_PARAMS_DECL MPIDI_UCX_Alltoall_params_t ucx_alltoall_params;
#define MPIDI_UCX_ALLTOALLV_PARAMS_DECL MPIDI_UCX_Alltoallv_params_t ucx_alltoallv_params;
#define MPIDI_UCX_ALLTOALLW_PARAMS_DECL MPIDI_UCX_Alltoallw_params_t ucx_alltoallw_params;
#define MPIDI_UCX_ALLGATHER_PARAMS_DECL MPIDI_UCX_Allgather_params_t ucx_allgather_params;
#define MPIDI_UCX_ALLGATHERV_PARAMS_DECL MPIDI_UCX_Allgatherv_params_t ucx_allgatherv_params;
#define MPIDI_UCX_GATHER_PARAMS_DECL MPIDI_UCX_Gather_params_t ucx_gather_params;
#define MPIDI_UCX_GATHERV_PARAMS_DECL MPIDI_UCX_Gatherv_params_t ucx_gatherv_params;
#define MPIDI_UCX_SCATTER_PARAMS_DECL MPIDI_UCX_Scatter_params_t ucx_scatter_params;
#define MPIDI_UCX_SCATTERV_PARAMS_DECL MPIDI_UCX_Scatterv_params_t ucx_scatterv_params;
#define MPIDI_UCX_REDUCE_SCATTER_PARAMS_DECL MPIDI_UCX_Reduce_scatter_params_t ucx_reduce_scatter_params;
#define MPIDI_UCX_REDUCE_SCATTER_BLOCK_PARAMS_DECL MPIDI_UCX_Reduce_scatter_block_params_t ucx_reduce_scatter_block_params;
#define MPIDI_UCX_SCAN_PARAMS_DECL MPIDI_UCX_Scan_params_t ucx_scan_params;
#define MPIDI_UCX_EXSCAN_PARAMS_DECL MPIDI_UCX_Exscan_params_t ucx_exscan_params;

typedef union {
    MPIDI_UCX_BARRIER_PARAMS_DECL;
    MPIDI_UCX_BCAST_PARAMS_DECL;
    MPIDI_UCX_REDUCE_PARAMS_DECL;
    MPIDI_UCX_ALLREDUCE_PARAMS_DECL;
    MPIDI_UCX_ALLTOALL_PARAMS_DECL;
    MPIDI_UCX_ALLTOALLV_PARAMS_DECL;
    MPIDI_UCX_ALLTOALLW_PARAMS_DECL;
    MPIDI_UCX_ALLGATHER_PARAMS_DECL;
    MPIDI_UCX_ALLGATHERV_PARAMS_DECL;
    MPIDI_UCX_GATHER_PARAMS_DECL;
    MPIDI_UCX_GATHERV_PARAMS_DECL;
    MPIDI_UCX_SCATTER_PARAMS_DECL;
    MPIDI_UCX_SCATTERV_PARAMS_DECL;
    MPIDI_UCX_REDUCE_SCATTER_PARAMS_DECL;
    MPIDI_UCX_REDUCE_SCATTER_BLOCK_PARAMS_DECL;
    MPIDI_UCX_SCAN_PARAMS_DECL;
    MPIDI_UCX_EXSCAN_PARAMS_DECL;
} MPIDI_UCX_coll_params_t;

typedef struct MPIDI_UCX_coll_algo_container {
    int id;
    MPIDI_UCX_coll_params_t params;
} MPIDI_UCX_coll_algo_container_t;

#endif /* UCX_COLL_PARAMS_H_INCLUDED */
