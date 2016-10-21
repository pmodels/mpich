#ifndef CH4_COLL_PARAMS_H_INCLUDED
#define CH4_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_CH4_bcast_composition_nm_id,
    MPIDI_CH4_bcast_composition_shm_id,
    MPIDI_CH4_bcast_composition_alpha_id
} MPIDI_CH4_bcast_id_t;

typedef union {
    struct MPIDI_generic_bcast_knomial_parameters {
        int radix;
        int block_size;
    } generic_bcast_knomial_parameters;
    struct MPIDI_generic_bcast_parameters {
        int segment_size;
    } generic_bcast_parameters;
    struct MPIDI_generic_bcast_empty_parameters {
        int empty;
    } generic_bcast_empty_parameters;

    struct MPIDI_CH4_bcast {
        int nm_bcast;
        int shm_bcast;
    } ch4_bcast;
} MPIDI_CH4_bcast_params_t;

typedef enum {
    MPIDI_CH4_reduce_composition_alpha_id
} MPIDI_CH4_reduce_id_t;

typedef union {
    struct MPIDI_generic_reduce_empty_parameters {
        int empty;
    } generic_reduce_empty_parameters;

    struct MPIDI_CH4_reduce {
        int nm_reduce;
        int shm_reduce;
    } ch4_reduce;
} MPIDI_CH4_reduce_params_t;

typedef enum {
    MPIDI_CH4_allreduce_composition_alpha_id,
    MPIDI_CH4_allreduce_composition_beta_id,
    MPIDI_CH4_allreduce_composition_gamma_id,
    MPIDI_CH4_allreduce_composition_nm_id
} MPIDI_CH4_allreduce_id_t;

typedef union {
    struct MPIDI_generic_allreduce_empty_parameters {
        int empty;
    } generic_allreduce_empty_parameters;

    struct MPIDI_CH4_allreduce_alpha {
        int shm_reduce;
        int nm_allreduce;
        int shm_bcast;
    } ch4_allreduce_alpha;
    struct MPIDI_CH4_allreduce_beta {
        int shm_reduce;
        int nm_reduce;
        int nm_bcast;
        int shm_bcast;
    } ch4_allreduce_beta;
    struct MPIDI_CH4_allreduce_gamma {
        int nm_reduce;
        int nm_bcast;
    } ch4_allreduce_gamma;
} MPIDI_CH4_allreduce_params_t;

#define MPIDI_CH4_BCAST_PARAMS_DECL MPIDI_CH4_bcast_params_t ch4_bcast_params;
#define MPIDI_CH4_REDUCE_PARAMS_DECL MPIDI_CH4_reduce_params_t ch4_reduce_params;
#define MPIDI_CH4_ALLREDUCE_PARAMS_DECL MPIDI_CH4_allreduce_params_t ch4_allreduce_params;

typedef union {
    MPIDI_CH4_BCAST_PARAMS_DECL;
    MPIDI_CH4_REDUCE_PARAMS_DECL;
    MPIDI_CH4_ALLREDUCE_PARAMS_DECL;
} MPIDI_CH4_coll_params_t;

typedef struct MPIDI_coll_algo_container {
    int id;
    MPIDI_CH4_coll_params_t params;
} MPIDI_coll_algo_container_t;

#endif /* CH4_COLL_PARAMS_H_INCLUDED */
