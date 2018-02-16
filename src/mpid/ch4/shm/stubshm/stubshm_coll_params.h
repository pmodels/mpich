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

typedef enum {
    MPIDI_STUBSHM_Alltoall_intra_brucks_id,
    MPIDI_STUBSHM_Alltoall_intra_scattered_id,
    MPIDI_STUBSHM_Alltoall_intra_pairwise_id,
    MPIDI_STUBSHM_Alltoall_intra_pairwise_sendrecv_replace_id
} MPIDI_STUBSHM_Alltoall_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Alltoall_empty_parameters {
        int empty;
    } stubshm_alltoall_empty_parameters;
} MPIDI_STUBSHM_Alltoall_params_t;

typedef enum {
    MPIDI_STUBSHM_Alltoallv_intra_pairwise_sendrecv_replace_id,
    MPIDI_STUBSHM_Alltoallv_intra_scattered_id
} MPIDI_STUBSHM_Alltoallv_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Alltoallv_empty_parameters {
        int empty;
    } stubshm_alltoallv_empty_parameters;
} MPIDI_STUBSHM_Alltoallv_params_t;

typedef enum {
    MPIDI_STUBSHM_Alltoallw_intra_pairwise_sendrecv_replace_id,
    MPIDI_STUBSHM_Alltoallw_intra_scattered_id
} MPIDI_STUBSHM_Alltoallw_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Alltoallw_empty_parameters {
        int empty;
    } stubshm_alltoallw_empty_parameters;
} MPIDI_STUBSHM_Alltoallw_params_t;

typedef enum {
    MPIDI_STUBSHM_Allgather_intra_recursive_doubling_id,
    MPIDI_STUBSHM_Allgather_intra_brucks_id,
    MPIDI_STUBSHM_Allgather_intra_ring_id
} MPIDI_STUBSHM_Allgather_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Allgather_empty_parameters {
        int empty;
    } stubshm_allgather_empty_parameters;
} MPIDI_STUBSHM_Allgather_params_t;

typedef enum {
    MPIDI_STUBSHM_Allgatherv_intra_recursive_doubling_id,
    MPIDI_STUBSHM_Allgatherv_intra_brucks_id,
    MPIDI_STUBSHM_Allgatherv_intra_ring_id
} MPIDI_STUBSHM_Allgatherv_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Allgatherv_empty_parameters {
        int empty;
    } stubshm_allgatherv_empty_parameters;
} MPIDI_STUBSHM_Allgatherv_params_t;

typedef enum {
    MPIDI_STUBSHM_Gather_intra_binomial_id,
} MPIDI_STUBSHM_Gather_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Gather_empty_parameters {
        int empty;
    } stubshm_gather_empty_parameters;
} MPIDI_STUBSHM_Gather_params_t;

typedef enum {
    MPIDI_STUBSHM_Gatherv_allcomm_linear_id,
} MPIDI_STUBSHM_Gatherv_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Gatherv_empty_parameters {
        int empty;
    } stubshm_gatherv_empty_parameters;
} MPIDI_STUBSHM_Gatherv_params_t;

typedef enum {
    MPIDI_STUBSHM_Scatter_intra_binomial_id,
} MPIDI_STUBSHM_Scatter_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Scatter_empty_parameters {
        int empty;
    } stubshm_scatter_empty_parameters;
} MPIDI_STUBSHM_Scatter_params_t;

typedef enum {
    MPIDI_STUBSHM_Scatterv_allcomm_linear_id,
} MPIDI_STUBSHM_Scatterv_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Scatterv_empty_parameters {
        int empty;
    } stubshm_scatterv_empty_parameters;
} MPIDI_STUBSHM_Scatterv_params_t;

typedef enum {
    MPIDI_STUBSHM_Reduce_scatter_intra_noncommutative_id,
    MPIDI_STUBSHM_Reduce_scatter_intra_pairwise_id,
    MPIDI_STUBSHM_Reduce_scatter_intra_recursive_doubling_id,
    MPIDI_STUBSHM_Reduce_scatter_intra_recursive_halving_id,
} MPIDI_STUBSHM_Reduce_scatter_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Reduce_scatter_empty_parameters {
        int empty;
    } stubshm_reduce_scatter_empty_parameters;
} MPIDI_STUBSHM_Reduce_scatter_params_t;

typedef enum {
    MPIDI_STUBSHM_Reduce_scatter_block_intra_noncommutative_id,
    MPIDI_STUBSHM_Reduce_scatter_block_intra_pairwise_id,
    MPIDI_STUBSHM_Reduce_scatter_block_intra_recursive_doubling_id,
    MPIDI_STUBSHM_Reduce_scatter_block_intra_recursive_halving_id,
} MPIDI_STUBSHM_Reduce_scatter_block_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Reduce_scatter_block_empty_parameters {
        int empty;
    } stubshm_reduce_scatter_block_empty_parameters;
} MPIDI_STUBSHM_Reduce_scatter_block_params_t;

typedef enum {
    MPIDI_STUBSHM_Scan_intra_recursive_doubling_id,
} MPIDI_STUBSHM_Scan_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Scan_empty_parameters {
        int empty;
    } stubshm_scan_empty_parameters;
} MPIDI_STUBSHM_Scan_params_t;

typedef enum {
    MPIDI_STUBSHM_Exscan_intra_recursive_doubling_id,
} MPIDI_STUBSHM_Exscan_id_t;

typedef union {
    /* reserved for parameters related to SHM specific collectives */
    struct MPIDI_STUBSHM_Exscan_empty_parameters {
        int empty;
    } stubshm_exscan_empty_parameters;
} MPIDI_STUBSHM_Exscan_params_t;

#define MPIDI_STUBSHM_BARRIER_PARAMS_DECL MPIDI_STUBSHM_Barrier_params_t stubshm_barrier_params;
#define MPIDI_STUBSHM_BCAST_PARAMS_DECL MPIDI_STUBSHM_Bcast_params_t stubshm_bcast_params;
#define MPIDI_STUBSHM_REDUCE_PARAMS_DECL MPIDI_STUBSHM_Reduce_params_t stubshm_reduce_params;
#define MPIDI_STUBSHM_ALLREDUCE_PARAMS_DECL MPIDI_STUBSHM_Allreduce_params_t stubshm_allreduce_params;
#define MPIDI_STUBSHM_ALLTOALL_PARAMS_DECL MPIDI_STUBSHM_Alltoall_params_t stubshm_alltoall_params;
#define MPIDI_STUBSHM_ALLTOALLV_PARAMS_DECL MPIDI_STUBSHM_Alltoallv_params_t stubshm_alltoallv_params;
#define MPIDI_STUBSHM_ALLTOALLW_PARAMS_DECL MPIDI_STUBSHM_Alltoallw_params_t stubshm_alltoallw_params;
#define MPIDI_STUBSHM_ALLGATHER_PARAMS_DECL MPIDI_STUBSHM_Allgather_params_t stubshm_allgather_params;
#define MPIDI_STUBSHM_ALLGATHERV_PARAMS_DECL MPIDI_STUBSHM_Allgatherv_params_t stubshm_allgatherv_params;
#define MPIDI_STUBSHM_GATHER_PARAMS_DECL MPIDI_STUBSHM_Gather_params_t stubshm_gather_params;
#define MPIDI_STUBSHM_GATHERV_PARAMS_DECL MPIDI_STUBSHM_Gatherv_params_t stubshm_gatherv_params;
#define MPIDI_STUBSHM_SCATTER_PARAMS_DECL MPIDI_STUBSHM_Scatter_params_t stubshm_scatter_params;
#define MPIDI_STUBSHM_SCATTERV_PARAMS_DECL MPIDI_STUBSHM_Scatterv_params_t stubshm_scatterv_params;
#define MPIDI_STUBSHM_REDUCE_SCATTER_PARAMS_DECL MPIDI_STUBSHM_Reduce_scatter_params_t stubshm_reduce_scatter_params;
#define MPIDI_STUBSHM_REDUCE_SCATTER_BLOCK_PARAMS_DECL MPIDI_STUBSHM_Reduce_scatter_block_params_t stubshm_reduce_scatter_block_params;
#define MPIDI_STUBSHM_SCAN_PARAMS_DECL MPIDI_STUBSHM_Scan_params_t stubshm_scan_params;
#define MPIDI_STUBSHM_EXSCAN_PARAMS_DECL MPIDI_STUBSHM_Exscan_params_t stubshm_exscan_params;

typedef union {
    MPIDI_STUBSHM_BARRIER_PARAMS_DECL;
    MPIDI_STUBSHM_BCAST_PARAMS_DECL;
    MPIDI_STUBSHM_REDUCE_PARAMS_DECL;
    MPIDI_STUBSHM_ALLREDUCE_PARAMS_DECL;
    MPIDI_STUBSHM_ALLTOALL_PARAMS_DECL;
    MPIDI_STUBSHM_ALLTOALLV_PARAMS_DECL;
    MPIDI_STUBSHM_ALLTOALLW_PARAMS_DECL;
    MPIDI_STUBSHM_ALLGATHER_PARAMS_DECL;
    MPIDI_STUBSHM_ALLGATHERV_PARAMS_DECL;
    MPIDI_STUBSHM_GATHER_PARAMS_DECL;
    MPIDI_STUBSHM_GATHERV_PARAMS_DECL;
    MPIDI_STUBSHM_SCATTER_PARAMS_DECL;
    MPIDI_STUBSHM_SCATTERV_PARAMS_DECL;
    MPIDI_STUBSHM_REDUCE_SCATTER_PARAMS_DECL;
    MPIDI_STUBSHM_REDUCE_SCATTER_BLOCK_PARAMS_DECL;
    MPIDI_STUBSHM_SCAN_PARAMS_DECL;
    MPIDI_STUBSHM_EXSCAN_PARAMS_DECL;
} MPIDI_STUBSHM_coll_params_t;

typedef struct MPIDI_STUBSHM_coll_algo_container {
    int id;
    MPIDI_STUBSHM_coll_params_t params;
} MPIDI_STUBSHM_coll_algo_container_t;

#endif /* STUBSHM_COLL_PARAMS_H_INCLUDED */
