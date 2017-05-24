#ifndef CH4_COLL_PARAMS_H_INCLUDED
#define CH4_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_CH4_barrier_composition_alpha_id,
    MPIDI_CH4_barrier_composition_beta_id
} MPIDI_CH4_barrier_id_t;

typedef union {
    struct MPIDI_CH4_barrier_alpha {
        int intra_barrier;
        int inter_barreir;
    } ch4_barrier_alpha;
    struct MPIDI_CH4_barrier_beta {
        int barrier;
    } ch4_barrier_beta;
} MPIDI_CH4_barrier_params_t;

typedef enum {
    MPIDI_CH4_bcast_composition_alpha_id,
    MPIDI_CH4_bcast_composition_beta_id,
    MPIDI_CH4_bcast_composition_gamma_id
} MPIDI_CH4_bcast_id_t;

typedef union {
    struct MPIDI_CH4_bcast_alpha {
        int inter_bcast;
        int intra_bcast;
    } ch4_bcast_alpha;
    struct MPIDI_CH4_bcast_beta {
        int intra_bcast_alpha;
        int inter_bcast;
        int intra_bcast_beta;
    } ch4_bcast_beta;
    struct MPIDI_CH4_bcast_gamma {
        int bcast;
    } ch4_bcast_gamma;
} MPIDI_CH4_bcast_params_t;

typedef enum {
    MPIDI_CH4_reduce_composition_alpha_id,
    MPIDI_CH4_reduce_composition_beta_id
} MPIDI_CH4_reduce_id_t;

typedef union {
    struct MPIDI_CH4_reduce_alpha {
        int intra_reduce;
        int inter_reduce;
    } ch4_reduce_alpha;
    struct MPIDI_CH4_reduce_beta {
        int reduce;
    } ch4_reduce_beta;
} MPIDI_CH4_reduce_params_t;

typedef enum {
    MPIDI_CH4_allreduce_composition_alpha_id,
    MPIDI_CH4_allreduce_composition_beta_id,
    MPIDI_CH4_allreduce_composition_gamma_id
} MPIDI_CH4_allreduce_id_t;

typedef union {
    struct MPIDI_CH4_allreduce_alpha {
        int intra_reduce;
        int inter_allreduce;
        int intra_bcast;
    } ch4_allreduce_alpha;
    struct MPIDI_CH4_allreduce_beta {
        int reduce;
        int bcast;
    } ch4_allreduce_beta;
    struct MPIDI_CH4_allreduce_gamma {
        int allreduce;
    } ch4_allreduce_gamma;
} MPIDI_CH4_allreduce_params_t;

#define MPIDI_CH4_BARRIER_PARAMS_DECL MPIDI_CH4_barrier_params_t ch4_barrier_params;
#define MPIDI_CH4_BCAST_PARAMS_DECL MPIDI_CH4_bcast_params_t ch4_bcast_params;
#define MPIDI_CH4_REDUCE_PARAMS_DECL MPIDI_CH4_reduce_params_t ch4_reduce_params;
#define MPIDI_CH4_ALLREDUCE_PARAMS_DECL MPIDI_CH4_allreduce_params_t ch4_allreduce_params;

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
