#ifndef CH4_COLL_PARAMS_H_INCLUDED
#define CH4_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_CH4_Barrier_intra_composition_alpha_id,
    MPIDI_CH4_Barrier_intra_composition_beta_id,
    MPIDI_CH4_Barrier_intra_composition_gamma_id,
    MPIDI_CH4_Barrier_inter_composition_alpha_id,
} MPIDI_CH4_Barrier_id_t;

typedef union {
    struct MPIDI_CH4_Barrier_alpha {
        int node_barrier;
        int roots_barrier;
        int node_bcast;
    } ch4_barrier_alpha;
    struct MPIDI_CH4_Barrier_beta {
        int barrier;
    } ch4_barrier_beta;
    struct MPIDI_CH4_Barrier_gamma {
        int barrier;
    } ch4_barrier_gamma;
} MPIDI_CH4_Barrier_params_t;

typedef enum {
    MPIDI_CH4_Bcast_intra_composition_alpha_id,
    MPIDI_CH4_Bcast_intra_composition_beta_id,
    MPIDI_CH4_Bcast_intra_composition_gamma_id,
    MPIDI_CH4_Bcast_intra_composition_delta_id,
    MPIDI_CH4_Bcast_inter_composition_alpha_id,
} MPIDI_CH4_Bcast_id_t;

typedef union {
    struct MPIDI_CH4_Bcast_alpha {
        int roots_bcast;
        int node_bcast;
    } ch4_bcast_alpha;
    struct MPIDI_CH4_Bcast_beta {
        int node_bcast_first;
        int roots_bcast;
        int node_bcast_second;
    } ch4_bcast_beta;
    struct MPIDI_CH4_Bcast_gamma {
        int bcast;
    } ch4_bcast_gamma;
    struct MPIDI_CH4_Bcast_delta {
        int bcast;
    } ch4_bcast_delta;
} MPIDI_CH4_Bcast_params_t;

typedef enum {
    MPIDI_CH4_Reduce_intra_composition_alpha_id,
    MPIDI_CH4_Reduce_intra_composition_beta_id,
    MPIDI_CH4_Reduce_intra_composition_gamma_id,
    MPIDI_CH4_Reduce_inter_composition_alpha_id,
} MPIDI_CH4_Reduce_id_t;

typedef union {
    struct MPIDI_CH4_Reduce_alpha {
        int node_reduce;
        int roots_reduce;
    } ch4_reduce_alpha;
    struct MPIDI_CH4_Reduce_beta {
        int reduce;
    } ch4_reduce_beta;
    struct MPIDI_CH4_Reduce_gamma {
        int reduce;
    } ch4_reduce_gamma;
} MPIDI_CH4_Reduce_params_t;

typedef enum {
    MPIDI_CH4_Allreduce_intra_composition_alpha_id,
    MPIDI_CH4_Allreduce_intra_composition_beta_id,
    MPIDI_CH4_Allreduce_intra_composition_gamma_id,
    MPIDI_CH4_Allreduce_inter_composition_alpha_id,
} MPIDI_CH4_Allreduce_id_t;

typedef union {
    struct MPIDI_CH4_Allreduce_alpha {
        int node_reduce;
        int roots_allreduce;
        int node_bcast;
    } ch4_allreduce_alpha;
    struct MPIDI_CH4_Allreduce_beta {
        int allreduce;
    } ch4_allreduce_beta;
    struct MPIDI_CH4_Allreduce_gamma {
        int allreduce;
    } ch4_allreduce_gamma;
} MPIDI_CH4_Allreduce_params_t;

typedef enum {
    MPIDI_CH4_Alltoall_intra_composition_alpha_id,
    MPIDI_CH4_Alltoall_intra_composition_beta_id,
    MPIDI_CH4_Alltoall_inter_composition_alpha_id
} MPIDI_CH4_Alltoall_id_t;

typedef union {
    struct MPIDI_CH4_Alltoall_alpha {
        int alltoall;
    } ch4_alltoall_alpha;
    struct MPIDI_CH4_Alltoall_beta {
        int alltoall;
    } ch4_alltoall_beta;
} MPIDI_CH4_Alltoall_params_t;

typedef enum {
    MPIDI_CH4_Alltoallv_intra_composition_alpha_id,
    MPIDI_CH4_Alltoallv_intra_composition_beta_id,
    MPIDI_CH4_Alltoallv_inter_composition_alpha_id
} MPIDI_CH4_Alltoallv_id_t;

typedef union {
    struct MPIDI_CH4_Alltoallv_alpha {
        int alltoallv;
    } ch4_alltoallv_alpha;
    struct MPIDI_CH4_Alltoallv_beta {
        int alltoallv;
    } ch4_alltoallv_beta;
} MPIDI_CH4_Alltoallv_params_t;

typedef enum {
    MPIDI_CH4_Alltoallw_intra_composition_alpha_id,
    MPIDI_CH4_Alltoallw_intra_composition_beta_id,
    MPIDI_CH4_Alltoallw_inter_composition_alpha_id
} MPIDI_CH4_Alltoallw_id_t;

typedef union {
    struct MPIDI_CH4_Alltoallw_alpha {
        int alltoallw;
    } ch4_alltoallw_alpha;
    struct MPIDI_CH4_Alltoallw_beta {
        int alltoallw;
    } ch4_alltoallw_beta;
} MPIDI_CH4_Alltoallw_params_t;

typedef enum {
    MPIDI_CH4_Allgather_intra_composition_alpha_id,
    MPIDI_CH4_Allgather_intra_composition_beta_id,
    MPIDI_CH4_Allgather_inter_composition_alpha_id
} MPIDI_CH4_Allgather_id_t;

typedef union {
    struct MPIDI_CH4_Allgather_alpha {
        int allgather;
    } ch4_allgather_alpha;
    struct MPIDI_CH4_Allgather_beta {
        int allgather;
    } ch4_allgather_beta;
} MPIDI_CH4_Allgather_params_t;

typedef enum {
    MPIDI_CH4_Allgatherv_intra_composition_alpha_id,
    MPIDI_CH4_Allgatherv_intra_composition_beta_id,
    MPIDI_CH4_Allgatherv_inter_composition_alpha_id
} MPIDI_CH4_allgatherv_id_t;

typedef union {
    struct MPIDI_CH4_Allgatherv_alpha {
        int allgatherv;
    } ch4_allgatherv_alpha;
    struct MPIDI_CH4_Allgatherv_beta {
        int allgatherv;
    } ch4_allgatherv_beta;
} MPIDI_CH4_Allgatherv_params_t;

typedef enum {
    MPIDI_CH4_Gather_intra_composition_alpha_id,
    MPIDI_CH4_Gather_intra_composition_beta_id,
    MPIDI_CH4_Gather_inter_composition_alpha_id
} MPIDI_CH4_Gather_id_t;

typedef union {
    struct MPIDI_CH4_Gather_alpha {
        int gather;
    } ch4_gather_alpha;
    struct MPIDI_CH4_Gather_beta {
        int gather;
    } ch4_gather_beta;
} MPIDI_CH4_Gather_params_t;

typedef enum {
    MPIDI_CH4_Gatherv_intra_composition_alpha_id,
    MPIDI_CH4_Gatherv_intra_composition_beta_id,
    MPIDI_CH4_Gatherv_inter_composition_alpha_id
} MPIDI_CH4_Gatherv_id_t;

typedef union {
    struct MPIDI_CH4_Gatherv_alpha {
        int gatherv;
    } ch4_gatherv_alpha;
    struct MPIDI_CH4_Gatherv_beta {
        int gatherv;
    } ch4_gatherv_beta;
} MPIDI_CH4_Gatherv_params_t;

typedef enum {
    MPIDI_CH4_Scatter_intra_composition_alpha_id,
    MPIDI_CH4_Scatter_intra_composition_beta_id,
    MPIDI_CH4_Scatter_inter_composition_alpha_id
} MPIDI_CH4_Scatter_id_t;

typedef union {
    struct MPIDI_CH4_Scatter_alpha {
        int scatter;
    } ch4_scatter_alpha;
    struct MPIDI_CH4_Scatter_beta {
        int scatter;
    } ch4_scatter_beta;
} MPIDI_CH4_Scatter_params_t;

typedef enum {
    MPIDI_CH4_Scatterv_intra_composition_alpha_id,
    MPIDI_CH4_Scatterv_intra_composition_beta_id,
    MPIDI_CH4_Scatterv_inter_composition_alpha_id
} MPIDI_CH4_Scatterv_id_t;

typedef union {
    struct MPIDI_CH4_Scatterv_alpha {
        int scatterv;
    } ch4_scatterv_alpha;
    struct MPIDI_CH4_Scatterv_beta {
        int scatterv;
    } ch4_scatterv_beta;
} MPIDI_CH4_Scatterv_params_t;

typedef enum {
    MPIDI_CH4_Reduce_scatter_intra_composition_alpha_id,
    MPIDI_CH4_Reduce_scatter_intra_composition_beta_id,
    MPIDI_CH4_Reduce_scatter_inter_composition_alpha_id
} MPIDI_CH4_Reduce_scatter__id_t;

typedef union {
    struct MPIDI_CH4_Reduce_scatter_alpha {
        int reduce_scatter;
    } ch4_reduce_scatter_alpha;
    struct MPIDI_CH4_Reduce_scatter_beta {
        int reduce_scatter;
    } ch4_reduce_scatter_beta;
} MPIDI_CH4_Reduce_scatter_params_t;

typedef enum {
    MPIDI_CH4_Reduce_scatter_block_intra_composition_alpha_id,
    MPIDI_CH4_Reduce_scatter_block_intra_composition_beta_id,
    MPIDI_CH4_Reduce_scatter_block_inter_composition_alpha_id
} MPIDI_CH4_Reduce_scatter_block__id_t;

typedef union {
    struct MPIDI_CH4_Reduce_scatter_block_alpha {
        int reduce_scatter_block;
    } ch4_reduce_scatter_block_alpha;
    struct MPIDI_CH4_Reduce_scatter_block_beta {
        int reduce_scatter_block;
    } ch4_reduce_scatter_block_beta;
} MPIDI_CH4_Reduce_scatter_block_params_t;

typedef enum {
    MPIDI_CH4_Scan_intra_composition_alpha_id,
    MPIDI_CH4_Scan_intra_composition_beta_id,
    MPIDI_CH4_Scan_intra_composition_gamma_id,
} MPIDI_CH4_Scan_id_t;

typedef union {
    struct MPIDI_CH4_Scan_alpha {
        int node_scan;
        int roots_scan;
        int node_bcast;
    } ch4_scan_alpha;
    struct MPIDI_CH4_Scan_beta {
        int scan;
    } ch4_scan_beta;
    struct MPIDI_CH4_Scan_gamma {
        int scan;
    } ch4_scan_gamma;
} MPIDI_CH4_Scan_params_t;

typedef enum {
    MPIDI_CH4_Exscan_intra_composition_alpha_id,
    MPIDI_CH4_Exscan_intra_composition_beta_id,
} MPIDI_CH4_Exscan_id_t;

typedef union {
    struct MPIDI_CH4_Exscan_alpha {
        int exscan;
    } ch4_exscan_alpha;
    struct MPIDI_CH4_Exscan_beta {
        int exscan;
    } ch4_exscan_beta;
} MPIDI_CH4_Exscan_params_t;

#define MPIDI_CH4_BARRIER_PARAMS_DECL MPIDI_CH4_Barrier_params_t ch4_barrier_params;
#define MPIDI_CH4_BCAST_PARAMS_DECL MPIDI_CH4_Bcast_params_t ch4_bcast_params;
#define MPIDI_CH4_REDUCE_PARAMS_DECL MPIDI_CH4_Reduce_params_t ch4_reduce_params;
#define MPIDI_CH4_ALLREDUCE_PARAMS_DECL MPIDI_CH4_Allreduce_params_t ch4_allreduce_params;
#define MPIDI_CH4_ALLTOALL_PARAMS_DECL MPIDI_CH4_Alltoall_params_t ch4_alltoall_params;
#define MPIDI_CH4_ALLTOALLV_PARAMS_DECL MPIDI_CH4_Alltoallv_params_t ch4_alltoallv_params;
#define MPIDI_CH4_ALLTOALLW_PARAMS_DECL MPIDI_CH4_Alltoallw_params_t ch4_alltoallw_params;
#define MPIDI_CH4_ALLGATHER_PARAMS_DECL MPIDI_CH4_Allgather_params_t ch4_allgather_params;
#define MPIDI_CH4_ALLGATHERV_PARAMS_DECL MPIDI_CH4_Allgatherv_params_t ch4_allgatherv_params;
#define MPIDI_CH4_GATHER_PARAMS_DECL MPIDI_CH4_Gather_params_t ch4_gather_params;
#define MPIDI_CH4_GATHERV_PARAMS_DECL MPIDI_CH4_Gatherv_params_t ch4_gatherv_params;
#define MPIDI_CH4_SCATTER_PARAMS_DECL MPIDI_CH4_Scatter_params_t ch4_scatter_params;
#define MPIDI_CH4_SCATTERV_PARAMS_DECL MPIDI_CH4_Scatterv_params_t ch4_scatterv_params;
#define MPIDI_CH4_REDUCE_SCATTER_PARAMS_DECL MPIDI_CH4_Reduce_scatter_params_t ch4_reduce_scatter_params
#define MPIDI_CH4_REDUCE_SCATTER_BLOCK_PARAMS_DECL MPIDI_CH4_Reduce_scatter_block_params_t ch4_reduce_scatter_block_params
#define MPIDI_CH4_SCAN_PARAMS_DECL MPIDI_CH4_Scan_params_t ch4_scan_params
#define MPIDI_CH4_EXSCAN_PARAMS_DECL MPIDI_CH4_Exscan_params_t ch4_exscan_params

typedef union {
    MPIDI_CH4_BARRIER_PARAMS_DECL;
    MPIDI_CH4_BCAST_PARAMS_DECL;
    MPIDI_CH4_REDUCE_PARAMS_DECL;
    MPIDI_CH4_ALLREDUCE_PARAMS_DECL;
    MPIDI_CH4_ALLTOALL_PARAMS_DECL;
    MPIDI_CH4_ALLTOALLV_PARAMS_DECL;
    MPIDI_CH4_ALLTOALLW_PARAMS_DECL;
    MPIDI_CH4_ALLGATHER_PARAMS_DECL;
    MPIDI_CH4_ALLGATHERV_PARAMS_DECL;
    MPIDI_CH4_GATHER_PARAMS_DECL;
    MPIDI_CH4_GATHERV_PARAMS_DECL;
    MPIDI_CH4_SCATTER_PARAMS_DECL;
    MPIDI_CH4_SCATTERV_PARAMS_DECL;
    MPIDI_CH4_REDUCE_SCATTER_PARAMS_DECL;
    MPIDI_CH4_REDUCE_SCATTER_BLOCK_PARAMS_DECL;
    MPIDI_CH4_SCAN_PARAMS_DECL;
    MPIDI_CH4_EXSCAN_PARAMS_DECL;
} MPIDI_CH4_coll_params_t;

typedef struct MPIDI_coll_algo_container {
    int id;
    MPIDI_CH4_coll_params_t params;
} MPIDI_coll_algo_container_t;

#endif /* CH4_COLL_PARAMS_H_INCLUDED */
