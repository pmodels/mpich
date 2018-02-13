#ifndef OFI_COLL_PARAMS_H_INCLUDED
#define OFI_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_OFI_Barrier_intra_recursive_doubling_id,
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

#define MPIDI_OFI_BARRIER_PARAMS_DECL MPIDI_OFI_Barrier_params_t ofi_barrier_params
#define MPIDI_OFI_BCAST_PARAMS_DECL MPIDI_OFI_Bcast_params_t ofi_bcast_params
#define MPIDI_OFI_REDUCE_PARAMS_DECL MPIDI_OFI_Reduce_params_t ofi_reduce_params
#define MPIDI_OFI_ALLREDUCE_PARAMS_DECL MPIDI_OFI_Allreduce_params_t ofi_allreduce_params

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
