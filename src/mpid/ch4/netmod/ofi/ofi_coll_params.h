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

#define MPIDI_OFI_BARRIER_PARAMS_DECL MPIDI_OFI_barrier_params_t ofi_barrier_params;
#define MPIDI_OFI_BCAST_PARAMS_DECL MPIDI_OFI_bcast_params_t ofi_bcast_params;
#define MPIDI_OFI_REDUCE_PARAMS_DECL MPIDI_OFI_reduce_params_t ofi_reduce_params;
#define MPIDI_OFI_ALLREDUCE_PARAMS_DECL MPIDI_OFI_allreduce_params_t ofi_allreduce_params;

typedef union {
    MPIDI_OFI_BARRIER_PARAMS_DECL;
    MPIDI_OFI_BCAST_PARAMS_DECL;
    MPIDI_OFI_REDUCE_PARAMS_DECL;
    MPIDI_OFI_ALLREDUCE_PARAMS_DECL;
} MPIDI_OFI_coll_params_t;

typedef struct MPIDI_OFI_coll_algo_container {
    int id;
    MPIDI_OFI_coll_params_t params;
} MPIDI_OFI_coll_algo_container_t;

#endif /* OFI_COLL_PARAMS_H_INCLUDED */
