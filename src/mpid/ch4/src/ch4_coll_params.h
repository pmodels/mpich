#ifndef CH4_COLL_PARAMS_H_INCLUDED
#define CH4_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_Barrier_intra_composition_alpha_id,
    MPIDI_Barrier_intra_composition_beta_id,
    MPIDI_Barrier_inter_composition_alpha_id,
} MPIDI_Barrier_id_t;

typedef union {
    struct MPIDI_Barrier_alpha {
        int node_barrier;
        int roots_barrier;
        int node_bcast;
    } ch4_barrier_alpha;
    struct MPIDI_Barrier_beta {
        int barrier;
    } ch4_barrier_beta;
} MPIDI_Barrier_params_t;

typedef enum {
    MPIDI_Bcast_intra_composition_alpha_id,
    MPIDI_Bcast_intra_composition_beta_id,
    MPIDI_Bcast_intra_composition_gamma_id,
    MPIDI_Bcast_inter_composition_alpha_id,
} MPIDI_Bcast_id_t;

typedef union {
    struct MPIDI_Bcast_alpha {
        int roots_bcast;
        int node_bcast;
    } ch4_bcast_alpha;
    struct MPIDI_Bcast_beta {
        int node_bcast_first;
        int roots_bcast;
        int node_bcast_second;
    } ch4_bcast_beta;
    struct MPIDI_Bcast_gamma {
        int bcast;
    } ch4_bcast_gamma;
} MPIDI_Bcast_params_t;

typedef enum {
    MPIDI_Reduce_intra_composition_alpha_id,
    MPIDI_Reduce_intra_composition_beta_id,
    MPIDI_Reduce_inter_composition_alpha_id,
} MPIDI_Reduce_id_t;

typedef union {
    struct MPIDI_Reduce_alpha {
        int node_reduce;
        int roots_reduce;
    } ch4_reduce_alpha;
    struct MPIDI_Reduce_beta {
        int reduce;
    } ch4_reduce_beta;
} MPIDI_Reduce_params_t;

typedef enum {
    MPIDI_Allreduce_intra_composition_alpha_id,
    MPIDI_Allreduce_intra_composition_beta_id,
    MPIDI_Allreduce_inter_composition_alpha_id,
} MPIDI_Allreduce_id_t;

typedef union {
    struct MPIDI_Allreduce_alpha {
        int node_reduce;
        int roots_allreduce;
        int node_bcast;
    } ch4_allreduce_alpha;
    struct MPIDI_Allreduce_beta {
        int allreduce;
    } ch4_allreduce_beta;
} MPIDI_Allreduce_params_t;

typedef enum {
    MPIDI_Alltoall_intra_composition_alpha_id,
    MPIDI_Alltoall_inter_composition_alpha_id
} MPIDI_Alltoall_id_t;

typedef union {
    struct MPIDI_Alltoall_alpha {
        int alltoall;
    } ch4_alltoall_alpha;
} MPIDI_Alltoall_params_t;

typedef enum {
    MPIDI_Alltoallv_intra_composition_alpha_id,
    MPIDI_Alltoallv_inter_composition_alpha_id
} MPIDI_Alltoallv_id_t;

typedef union {
    struct MPIDI_Alltoallv_alpha {
        int alltoallv;
    } ch4_alltoallv_alpha;
} MPIDI_Alltoallv_params_t;

typedef enum {
    MPIDI_Alltoallw_intra_composition_alpha_id,
    MPIDI_Alltoallw_inter_composition_alpha_id
} MPIDI_Alltoallw_id_t;

typedef union {
    struct MPIDI_Alltoallw_alpha {
        int alltoallw;
    } ch4_alltoallw_alpha;
} MPIDI_Alltoallw_params_t;

typedef enum {
    MPIDI_Allgather_intra_composition_alpha_id,
    MPIDI_Allgather_inter_composition_alpha_id
} MPIDI_Allgather_id_t;

typedef union {
    struct MPIDI_Allgather_alpha {
        int allgather;
    } ch4_allgather_alpha;
} MPIDI_Allgather_params_t;

typedef enum {
    MPIDI_Allgatherv_intra_composition_alpha_id,
    MPIDI_Allgatherv_inter_composition_alpha_id
} MPIDI_allgatherv_id_t;

typedef union {
    struct MPIDI_Allgatherv_alpha {
        int allgatherv;
    } ch4_allgatherv_alpha;
} MPIDI_Allgatherv_params_t;

typedef enum {
    MPIDI_Gather_intra_composition_alpha_id,
    MPIDI_Gather_inter_composition_alpha_id
} MPIDI_Gather_id_t;

typedef union {
    struct MPIDI_Gather_alpha {
        int gather;
    } ch4_gather_alpha;
} MPIDI_Gather_params_t;

typedef enum {
    MPIDI_Gatherv_intra_composition_alpha_id,
    MPIDI_Gatherv_inter_composition_alpha_id
} MPIDI_Gatherv_id_t;

typedef union {
    struct MPIDI_Gatherv_alpha {
        int gatherv;
    } ch4_gatherv_alpha;
} MPIDI_Gatherv_params_t;

typedef enum {
    MPIDI_Scatter_intra_composition_alpha_id,
    MPIDI_Scatter_inter_composition_alpha_id
} MPIDI_Scatter_id_t;

typedef union {
    struct MPIDI_Scatter_alpha {
        int scatter;
    } ch4_scatter_alpha;
} MPIDI_Scatter_params_t;

typedef enum {
    MPIDI_Scatterv_intra_composition_alpha_id,
    MPIDI_Scatterv_inter_composition_alpha_id
} MPIDI_Scatterv_id_t;

typedef union {
    struct MPIDI_Scatterv_alpha {
        int scatterv;
    } ch4_scatterv_alpha;
} MPIDI_Scatterv_params_t;

typedef enum {
    MPIDI_Reduce_scatter_intra_composition_alpha_id,
    MPIDI_Reduce_scatter_inter_composition_alpha_id
} MPIDI_Reduce_scatter_id_t;

typedef union {
    struct MPIDI_Reduce_scatter_alpha {
        int reduce_scatter;
    } ch4_reduce_scatter_alpha;
} MPIDI_Reduce_scatter_params_t;

typedef enum {
    MPIDI_Reduce_scatter_block_intra_composition_alpha_id,
    MPIDI_Reduce_scatter_block_inter_composition_alpha_id
} MPIDI_Reduce_scatter_block__id_t;

typedef union {
    struct MPIDI_Reduce_scatter_block_alpha {
        int reduce_scatter_block;
    } ch4_reduce_scatter_block_alpha;
} MPIDI_Reduce_scatter_block_params_t;

typedef enum {
    MPIDI_Scan_intra_composition_alpha_id,
    MPIDI_Scan_intra_composition_beta_id
} MPIDI_Scan_id_t;

typedef union {
    struct MPIDI_Scan_alpha {
        int node_scan;
        int roots_scan;
        int node_bcast;
    } ch4_scan_alpha;
    struct MPIDI_Scan_beta {
        int scan;
    } ch4_scan_beta;
} MPIDI_Scan_params_t;

typedef enum {
    MPIDI_Exscan_intra_composition_alpha_id,
} MPIDI_Exscan_id_t;

typedef union {
    struct MPIDI_Exscan_alpha {
        int exscan;
    } ch4_exscan_alpha;
} MPIDI_Exscan_params_t;

#define MPIDI_BARRIER_PARAMS_DECL MPIDI_Barrier_params_t ch4_barrier_params;
#define MPIDI_BCAST_PARAMS_DECL MPIDI_Bcast_params_t ch4_bcast_params;
#define MPIDI_REDUCE_PARAMS_DECL MPIDI_Reduce_params_t ch4_reduce_params;
#define MPIDI_ALLREDUCE_PARAMS_DECL MPIDI_Allreduce_params_t ch4_allreduce_params;
#define MPIDI_ALLTOALL_PARAMS_DECL MPIDI_Alltoall_params_t ch4_alltoall_params;
#define MPIDI_ALLTOALLV_PARAMS_DECL MPIDI_Alltoallv_params_t ch4_alltoallv_params;
#define MPIDI_ALLTOALLW_PARAMS_DECL MPIDI_Alltoallw_params_t ch4_alltoallw_params;
#define MPIDI_ALLGATHER_PARAMS_DECL MPIDI_Allgather_params_t ch4_allgather_params;
#define MPIDI_ALLGATHERV_PARAMS_DECL MPIDI_Allgatherv_params_t ch4_allgatherv_params;
#define MPIDI_GATHER_PARAMS_DECL MPIDI_Gather_params_t ch4_gather_params;
#define MPIDI_GATHERV_PARAMS_DECL MPIDI_Gatherv_params_t ch4_gatherv_params;
#define MPIDI_SCATTER_PARAMS_DECL MPIDI_Scatter_params_t ch4_scatter_params;
#define MPIDI_SCATTERV_PARAMS_DECL MPIDI_Scatterv_params_t ch4_scatterv_params;
#define MPIDI_REDUCE_SCATTER_PARAMS_DECL MPIDI_Reduce_scatter_params_t ch4_reduce_scatter_params
#define MPIDI_REDUCE_SCATTER_BLOCK_PARAMS_DECL MPIDI_Reduce_scatter_block_params_t ch4_reduce_scatter_block_params
#define MPIDI_SCAN_PARAMS_DECL MPIDI_Scan_params_t ch4_scan_params
#define MPIDI_EXSCAN_PARAMS_DECL MPIDI_Exscan_params_t ch4_exscan_params

typedef union {
    MPIDI_BARRIER_PARAMS_DECL;
    MPIDI_BCAST_PARAMS_DECL;
    MPIDI_REDUCE_PARAMS_DECL;
    MPIDI_ALLREDUCE_PARAMS_DECL;
    MPIDI_ALLTOALL_PARAMS_DECL;
    MPIDI_ALLTOALLV_PARAMS_DECL;
    MPIDI_ALLTOALLW_PARAMS_DECL;
    MPIDI_ALLGATHER_PARAMS_DECL;
    MPIDI_ALLGATHERV_PARAMS_DECL;
    MPIDI_GATHER_PARAMS_DECL;
    MPIDI_GATHERV_PARAMS_DECL;
    MPIDI_SCATTER_PARAMS_DECL;
    MPIDI_SCATTERV_PARAMS_DECL;
    MPIDI_REDUCE_SCATTER_PARAMS_DECL;
    MPIDI_REDUCE_SCATTER_BLOCK_PARAMS_DECL;
    MPIDI_SCAN_PARAMS_DECL;
    MPIDI_EXSCAN_PARAMS_DECL;
} MPIDI_coll_params_t;

typedef struct MPIDI_coll_algo_container {
    int id;
    MPIDI_coll_params_t params;
} MPIDI_coll_algo_container_t;

#endif /* CH4_COLL_PARAMS_H_INCLUDED */
