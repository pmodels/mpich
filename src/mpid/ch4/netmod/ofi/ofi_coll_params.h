#ifndef OFI_COLL_PARAMS_H_INCLUDED
#define OFI_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_OFI_Barrier_intra_dissemination_id,
} MPIDI_OFI_Barrier_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Barrier_empty_parameters {
        int empty;
    } ofi_barrier_empty_parameters;
} MPIDI_OFI_Barrier_params_t;

typedef enum {
    MPIDI_OFI_Bcast_intra_binomial_id,
    MPIDI_OFI_Bcast_intra_scatter_recursive_doubling_allgather_id,
    MPIDI_OFI_Bcast_intra_scatter_ring_allgather_id,
} MPIDI_OFI_Bcast_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Bcast_knomial_parameters {
        int radix;
        int block_size;
    } ofi_bcast_knomial_parameters;
    struct MPIDI_OFI_Bcast_empty_parameters {
        int empty;
    } ofi_bcast_empty_parameters;
} MPIDI_OFI_Bcast_params_t;

typedef enum {
    MPIDI_OFI_Allreduce_intra_recursive_doubling_id,
    MPIDI_OFI_Allreduce_intra_reduce_scatter_allgather_id
} MPIDI_OFI_Allreduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Allreduce_empty_parameters {
        int empty;
    } ofi_allreduce_empty_parameters;
} MPIDI_OFI_Allreduce_params_t;

typedef enum {
    MPIDI_OFI_Reduce_intra_reduce_scatter_gather_id,
    MPIDI_OFI_Reduce_intra_binomial_id
} MPIDI_OFI_Reduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Reduce_empty_parameters {
        int empty;
    } ofi_reduce_empty_parameters;
} MPIDI_OFI_Reduce_params_t;

typedef enum {
    MPIDI_OFI_Alltoall_intra_brucks_id,
    MPIDI_OFI_Alltoall_intra_scattered_id,
    MPIDI_OFI_Alltoall_intra_pairwise_id,
    MPIDI_OFI_Alltoall_intra_pairwise_sendrecv_replace_id
} MPIDI_OFI_Alltoall_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Alltoall_empty_parameters {
        int empty;
    } ofi_alltoall_empty_parameters;
} MPIDI_OFI_Alltoall_params_t;

typedef enum {
    MPIDI_OFI_Alltoallv_intra_pairwise_sendrecv_replace_id,
    MPIDI_OFI_Alltoallv_intra_scattered_id
} MPIDI_OFI_Alltoallv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Alltoallv_empty_parameters {
        int empty;
    } ofi_alltoallv_empty_parameters;
} MPIDI_OFI_Alltoallv_params_t;

typedef enum {
    MPIDI_OFI_Alltoallw_intra_pairwise_sendrecv_replace_id,
    MPIDI_OFI_Alltoallw_intra_scattered_id
} MPIDI_OFI_Alltoallw_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Alltoallw_empty_parameters {
        int empty;
    } ofi_alltoallw_empty_parameters;
} MPIDI_OFI_Alltoallw_params_t;

typedef enum {
    MPIDI_OFI_Allgather_intra_recursive_doubling_id,
    MPIDI_OFI_Allgather_intra_brucks_id,
    MPIDI_OFI_Allgather_intra_ring_id
} MPIDI_OFI_Allgather_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Allgather_empty_parameters {
        int empty;
    } ofi_allgather_empty_parameters;
} MPIDI_OFI_Allgather_params_t;

typedef enum {
    MPIDI_OFI_Allgatherv_intra_recursive_doubling_id,
    MPIDI_OFI_Allgatherv_intra_brucks_id,
    MPIDI_OFI_Allgatherv_intra_ring_id
} MPIDI_OFI_Allgatherv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Allgatherv_empty_parameters {
        int empty;
    } ofi_allgatherv_empty_parameters;
} MPIDI_OFI_Allgatherv_params_t;

typedef enum {
    MPIDI_OFI_Gather_intra_binomial_id,
} MPIDI_OFI_Gather_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Gather_empty_parameters {
        int empty;
    } ofi_gather_empty_parameters;
} MPIDI_OFI_Gather_params_t;

typedef enum {
    MPIDI_OFI_Gatherv_allcomm_linear_id,
} MPIDI_OFI_Gatherv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Gatherv_empty_parameters {
        int empty;
    } ofi_gatherv_empty_parameters;
} MPIDI_OFI_Gatherv_params_t;

typedef enum {
    MPIDI_OFI_Scatter_intra_binomial_id,
} MPIDI_OFI_Scatter_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Scatter_empty_parameters {
        int empty;
    } ofi_scatter_empty_parameters;
} MPIDI_OFI_Scatter_params_t;

typedef enum {
    MPIDI_OFI_Scatterv_allcomm_linear_id,
} MPIDI_OFI_Scatterv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Scatterv_empty_parameters {
        int empty;
    } ofi_scatterv_empty_parameters;
} MPIDI_OFI_Scatterv_params_t;

typedef enum {
    MPIDI_OFI_Reduce_scatter_intra_noncommutative_id,
    MPIDI_OFI_Reduce_scatter_intra_pairwise_id,
    MPIDI_OFI_Reduce_scatter_intra_recursive_doubling_id,
    MPIDI_OFI_Reduce_scatter_intra_recursive_halving_id,
} MPIDI_OFI_Reduce_scatter_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Reduce_scatter_empty_parameters {
        int empty;
    } ofi_reduce_scatter_empty_parameters;
} MPIDI_OFI_Reduce_scatter_params_t;

typedef enum {
    MPIDI_OFI_Reduce_scatter_block_intra_noncommutative_id,
    MPIDI_OFI_Reduce_scatter_block_intra_pairwise_id,
    MPIDI_OFI_Reduce_scatter_block_intra_recursive_doubling_id,
    MPIDI_OFI_Reduce_scatter_block_intra_recursive_halving_id,
} MPIDI_OFI_Reduce_scatter_block_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Reduce_scatter_block_empty_parameters {
        int empty;
    } ofi_reduce_scatter_block_empty_parameters;
} MPIDI_OFI_Reduce_scatter_block_params_t;

typedef enum {
    MPIDI_OFI_Scan_intra_recursive_doubling_id,
} MPIDI_OFI_Scan_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Scan_empty_parameters {
        int empty;
    } ofi_scan_empty_parameters;
} MPIDI_OFI_Scan_params_t;

typedef enum {
    MPIDI_OFI_Exscan_intra_recursive_doubling_id,
} MPIDI_OFI_Exscan_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_Exscan_empty_parameters {
        int empty;
    } ofi_exscan_empty_parameters;
} MPIDI_OFI_Exscan_params_t;

#define MPIDI_OFI_BARRIER_PARAMS_DECL MPIDI_OFI_Barrier_params_t ofi_barrier_params;
#define MPIDI_OFI_BCAST_PARAMS_DECL MPIDI_OFI_Bcast_params_t ofi_bcast_params;
#define MPIDI_OFI_REDUCE_PARAMS_DECL MPIDI_OFI_Reduce_params_t ofi_reduce_params;
#define MPIDI_OFI_ALLREDUCE_PARAMS_DECL MPIDI_OFI_Allreduce_params_t ofi_allreduce_params;
#define MPIDI_OFI_ALLTOALL_PARAMS_DECL MPIDI_OFI_Alltoall_params_t ofi_alltoall_params;
#define MPIDI_OFI_ALLTOALLV_PARAMS_DECL MPIDI_OFI_Alltoallv_params_t ofi_alltoallv_params;
#define MPIDI_OFI_ALLTOALLW_PARAMS_DECL MPIDI_OFI_Alltoallw_params_t ofi_alltoallw_params;
#define MPIDI_OFI_ALLGATHER_PARAMS_DECL MPIDI_OFI_Allgather_params_t ofi_allgather_params;
#define MPIDI_OFI_ALLGATHERV_PARAMS_DECL MPIDI_OFI_Allgatherv_params_t ofi_allgatherv_params;
#define MPIDI_OFI_GATHER_PARAMS_DECL MPIDI_OFI_Gather_params_t ofi_gather_params;
#define MPIDI_OFI_GATHERV_PARAMS_DECL MPIDI_OFI_Gatherv_params_t ofi_gatherv_params;
#define MPIDI_OFI_SCATTER_PARAMS_DECL MPIDI_OFI_Scatter_params_t ofi_scatter_params;
#define MPIDI_OFI_SCATTERV_PARAMS_DECL MPIDI_OFI_Scatterv_params_t ofi_scatterv_params;
#define MPIDI_OFI_REDUCE_SCATTER_PARAMS_DECL MPIDI_OFI_Reduce_scatter_params_t ofi_reduce_scatter_params;
#define MPIDI_OFI_REDUCE_SCATTER_BLOCK_PARAMS_DECL MPIDI_OFI_Reduce_scatter_block_params_t ofi_reduce_scatter_block_params;
#define MPIDI_OFI_SCAN_PARAMS_DECL MPIDI_OFI_Scan_params_t ofi_scan_params;
#define MPIDI_OFI_EXSCAN_PARAMS_DECL MPIDI_OFI_Exscan_params_t ofi_exscan_params;

typedef union {
    MPIDI_OFI_BARRIER_PARAMS_DECL;
    MPIDI_OFI_BCAST_PARAMS_DECL;
    MPIDI_OFI_REDUCE_PARAMS_DECL;
    MPIDI_OFI_ALLREDUCE_PARAMS_DECL;
    MPIDI_OFI_ALLTOALL_PARAMS_DECL;
    MPIDI_OFI_ALLTOALLV_PARAMS_DECL;
    MPIDI_OFI_ALLTOALLW_PARAMS_DECL;
    MPIDI_OFI_ALLGATHER_PARAMS_DECL;
    MPIDI_OFI_ALLGATHERV_PARAMS_DECL;
    MPIDI_OFI_GATHER_PARAMS_DECL;
    MPIDI_OFI_GATHERV_PARAMS_DECL;
    MPIDI_OFI_SCATTER_PARAMS_DECL;
    MPIDI_OFI_SCATTERV_PARAMS_DECL;
    MPIDI_OFI_REDUCE_SCATTER_PARAMS_DECL;
    MPIDI_OFI_REDUCE_SCATTER_BLOCK_PARAMS_DECL;
    MPIDI_OFI_SCAN_PARAMS_DECL;
    MPIDI_OFI_EXSCAN_PARAMS_DECL;
} MPIDI_OFI_coll_params_t;

typedef struct MPIDI_OFI_coll_algo_container {
    int id;
    MPIDI_OFI_coll_params_t params;
} MPIDI_OFI_coll_algo_container_t;

#endif /* OFI_COLL_PARAMS_H_INCLUDED */
