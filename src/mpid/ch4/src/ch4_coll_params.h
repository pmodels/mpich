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

#define MPIDI_CH4_BARRIER_PARAMS_DECL MPIDI_CH4_Barrier_params_t ch4_barrier_params
#define MPIDI_CH4_BCAST_PARAMS_DECL MPIDI_CH4_Bcast_params_t ch4_bcast_params
#define MPIDI_CH4_REDUCE_PARAMS_DECL MPIDI_CH4_Reduce_params_t ch4_reduce_params
#define MPIDI_CH4_ALLREDUCE_PARAMS_DECL MPIDI_CH4_Allreduce_params_t ch4_allreduce_params

typedef union {
    MPIDI_CH4_BARRIER_PARAMS_DECL;
    MPIDI_CH4_BCAST_PARAMS_DECL;
    MPIDI_CH4_REDUCE_PARAMS_DECL;
    MPIDI_CH4_ALLREDUCE_PARAMS_DECL;
} MPIDI_CH4_coll_params_t;

typedef struct MPIDI_coll_algo_container {
    int id;
    MPIDI_CH4_coll_params_t params;
} MPIDI_coll_algo_container_t;

#endif /* CH4_COLL_PARAMS_H_INCLUDED */
