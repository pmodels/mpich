#ifndef OFI_COLL_PARAMS_H_INCLUDED
#define OFI_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_OFI_barrier_recursive_doubling_id,
} MPIDI_OFI_barrier_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_barrier_empty_parameters {
        int empty;
    } ofi_barrier_empty_parameters;
} MPIDI_OFI_barrier_params_t;

typedef enum {
    MPIDI_OFI_bcast_binomial_id,
    MPIDI_OFI_bcast_scatter_doubling_allgather_id,
    MPIDI_OFI_bcast_scatter_ring_allgather_id,
} MPIDI_OFI_bcast_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_bcast_knomial_parameters {
        int radix;
        int block_size;
    } ofi_bcast_knomial_parameters;
    struct MPIDI_OFI_bcast_empty_parameters {
        int empty;
    } ofi_bcast_empty_parameters;
} MPIDI_OFI_bcast_params_t;

typedef enum {
    MPIDI_OFI_allreduce_recursive_doubling_id,
    MPIDI_OFI_allreduce_reduce_scatter_allgather_id
} MPIDI_OFI_allreduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_allreduce_empty_parameters {
        int empty;
    } ofi_allreduce_empty_parameters;
} MPIDI_OFI_allreduce_params_t;

typedef enum {
    MPIDI_OFI_reduce_redscat_gather_id,
    MPIDI_OFI_reduce_binomial_id
} MPIDI_OFI_reduce_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_reduce_empty_parameters {
        int empty;
    } ofi_reduce_empty_parameters;
} MPIDI_OFI_reduce_params_t;

typedef enum {
    MPIDI_OFI_alltoall_bruck_id,
    MPIDI_OFI_alltoall_isend_irecv_id,
    MPIDI_OFI_alltoall_pairwise_exchange_id,
    MPIDI_OFI_alltoall_sendrecv_replace_id
} MPIDI_OFI_alltoall_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_alltoall_empty_parameters {
        int empty;
    } ofi_alltoall_empty_parameters;
} MPIDI_OFI_alltoall_params_t;

typedef enum {
    MPIDI_OFI_alltoallv_sendrecv_replace_id,
    MPIDI_OFI_alltoallv_isend_irecv_id
} MPIDI_OFI_alltoallv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_alltoallv_empty_parameters {
        int empty;
    } ofi_alltoallv_empty_parameters;
} MPIDI_OFI_alltoallv_params_t;

typedef enum {
    MPIDI_OFI_alltoallw_sendrecv_replace_id,
    MPIDI_OFI_alltoallw_isend_irecv_id,
    MPIDI_OFI_alltoallw_pairwise_exchange_id
} MPIDI_OFI_alltoallw_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_alltoallw_empty_parameters {
        int empty;
    } ofi_alltoallw_empty_parameters;
} MPIDI_OFI_alltoallw_params_t;

typedef enum {
    MPIDI_OFI_allgather_recursive_doubling_id,
    MPIDI_OFI_allgather_bruck_id,
    MPIDI_OFI_allgather_ring_id
} MPIDI_OFI_allgather_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_allgather_empty_parameters {
        int empty;
    } ofi_allgather_empty_parameters;
} MPIDI_OFI_allgather_params_t;

typedef enum {
    MPIDI_OFI_allgatherv_recursive_doubling_id,
    MPIDI_OFI_allgatherv_bruck_id,
    MPIDI_OFI_allgatherv_ring_id
} MPIDI_OFI_allgatherv_id_t;

typedef union {
    /* reserved for parameters related to NETMOD specific collectives */
    struct MPIDI_OFI_allgatherv_empty_parameters {
        int empty;
    } ofi_allgatherv_empty_parameters;
} MPIDI_OFI_allgatherv_params_t;

#define MPIDI_OFI_BARRIER_PARAMS_DECL MPIDI_OFI_barrier_params_t ofi_barrier_params;
#define MPIDI_OFI_BCAST_PARAMS_DECL MPIDI_OFI_bcast_params_t ofi_bcast_params;
#define MPIDI_OFI_REDUCE_PARAMS_DECL MPIDI_OFI_reduce_params_t ofi_reduce_params;
#define MPIDI_OFI_ALLREDUCE_PARAMS_DECL MPIDI_OFI_allreduce_params_t ofi_allreduce_params;
#define MPIDI_OFI_ALLTOALL_PARAMS_DECL MPIDI_OFI_alltoall_params_t ofi_alltoall_params;
#define MPIDI_OFI_ALLTOALLV_PARAMS_DECL MPIDI_OFI_alltoallv_params_t ofi_alltoallv_params;
#define MPIDI_OFI_ALLTOALLW_PARAMS_DECL MPIDI_OFI_alltoallw_params_t ofi_alltoallw_params;
#define MPIDI_OFI_ALLGATHER_PARAMS_DECL MPIDI_OFI_allgather_params_t ofi_allgather_params;
#define MPIDI_OFI_ALLGATHERV_PARAMS_DECL MPIDI_OFI_allgatherv_params_t ofi_allgatherv_params;

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
} MPIDI_OFI_coll_params_t;

typedef struct MPIDI_OFI_coll_algo_container {
    int id;
    MPIDI_OFI_coll_params_t params;
} MPIDI_OFI_coll_algo_container_t;

#endif /* OFI_COLL_PARAMS_H_INCLUDED */
