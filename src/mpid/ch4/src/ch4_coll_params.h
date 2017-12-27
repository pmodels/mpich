#ifndef CH4_COLL_PARAMS_H_INCLUDED
#define CH4_COLL_PARAMS_H_INCLUDED

#include "mpichconf.h"

typedef enum {
    MPIDI_CH4_barrier_composition_alpha_id,
    MPIDI_CH4_barrier_composition_beta_id,
#ifdef MPIDI_BUILD_CH4_SHM
    MPIDI_CH4_barrier_composition_gamma_id,
#endif /* MPIDI_BUILD_CH4_SHM */
    MPIDI_CH4_barrier_intercomm_id,
} MPIDI_CH4_barrier_id_t;

typedef union {
    struct MPIDI_CH4_barrier_alpha {
        int node_barrier;
        int roots_barrier;
        int node_bcast;
    } ch4_barrier_alpha;
    struct MPIDI_CH4_barrier_beta {
        int barrier;
    } ch4_barrier_beta;
#ifdef MPIDI_BUILD_CH4_SHM
    struct MPIDI_CH4_barrier_gamma {
        int barrier;
    } ch4_barrier_gamma;
#endif /* MPIDI_BUILD_CH4_SHM */
} MPIDI_CH4_barrier_params_t;

typedef enum {
    MPIDI_CH4_bcast_composition_alpha_id,
    MPIDI_CH4_bcast_composition_beta_id,
    MPIDI_CH4_bcast_composition_gamma_id,
#ifdef MPIDI_BUILD_CH4_SHM
    MPIDI_CH4_bcast_composition_delta_id,
#endif /* MPIDI_BUILD_CH4_SHM */
    MPIDI_CH4_bcast_intercomm_id,
} MPIDI_CH4_bcast_id_t;

typedef union {
    struct MPIDI_CH4_bcast_alpha {
        int roots_bcast;
        int node_bcast;
    } ch4_bcast_alpha;
    struct MPIDI_CH4_bcast_beta {
        int node_bcast_first;
        int roots_bcast;
        int node_bcast_second;
    } ch4_bcast_beta;
    struct MPIDI_CH4_bcast_gamma {
        int bcast;
    } ch4_bcast_gamma;
#ifdef MPIDI_BUILD_CH4_SHM
    struct MPIDI_CH4_bcast_delta {
        int bcast;
    } ch4_bcast_delta;
#endif /* MPIDI_BUILD_CH4_SHM */
} MPIDI_CH4_bcast_params_t;

typedef enum {
    MPIDI_CH4_reduce_composition_alpha_id,
    MPIDI_CH4_reduce_composition_beta_id,
#ifdef MPIDI_BUILD_CH4_SHM
    MPIDI_CH4_reduce_composition_gamma_id,
#endif /* MPIDI_BUILD_CH4_SHM */
    MPIDI_CH4_reduce_intercomm_id,
} MPIDI_CH4_reduce_id_t;

typedef union {
    struct MPIDI_CH4_reduce_alpha {
        int node_reduce;
        int roots_reduce;
    } ch4_reduce_alpha;
    struct MPIDI_CH4_reduce_beta {
        int reduce;
    } ch4_reduce_beta;
#ifdef MPIDI_BUILD_CH4_SHM
    struct MPIDI_CH4_reduce_gamma {
        int reduce;
    } ch4_reduce_gamma;
#endif /* MPIDI_BUILD_CH4_SHM */
} MPIDI_CH4_reduce_params_t;

typedef enum {
    MPIDI_CH4_allreduce_composition_alpha_id,
    MPIDI_CH4_allreduce_composition_beta_id,
#ifdef MPIDI_BUILD_CH4_SHM
    MPIDI_CH4_allreduce_composition_gamma_id,
#endif /* MPIDI_BUILD_CH4_SHM */
    MPIDI_CH4_allreduce_intercomm_id,
} MPIDI_CH4_allreduce_id_t;

typedef union {
    struct MPIDI_CH4_allreduce_alpha {
        int node_reduce;
        int roots_allreduce;
        int node_bcast;
    } ch4_allreduce_alpha;
    struct MPIDI_CH4_allreduce_beta {
        int allreduce;
    } ch4_allreduce_beta;
#ifdef MPIDI_BUILD_CH4_SHM
    struct MPIDI_CH4_allreduce_gamma {
        int allreduce;
    } ch4_allreduce_gamma;
#endif /* MPIDI_BUILD_CH4_SHM */
} MPIDI_CH4_allreduce_params_t;

typedef enum {
    MPIDI_CH4_gather_intercomm_id,
    MPIDI_CH4_gather_composition_alpha_id,
#ifdef MPIDI_BUILD_CH4_SHM
    MPIDI_CH4_gather_composition_beta_id,
#endif /* MPIDI_BUILD_CH4_SHM */
} MPIDI_CH4_gather_id_t;

typedef enum {
    MPIDI_CH4_gatherv_intercomm_id,
    MPIDI_CH4_gatherv_composition_alpha_id,
#ifdef MPIDI_BUILD_CH4_SHM
    MPIDI_CH4_gatherv_composition_beta_id,
#endif /* MPIDI_BUILD_CH4_SHM */
} MPIDI_CH4_gatherv_id_t;

typedef enum {
    MPIDI_CH4_scatter_intercomm_id,
    MPIDI_CH4_scatter_composition_alpha_id,
#ifdef MPIDI_BUILD_CH4_SHM
    MPIDI_CH4_scatter_composition_beta_id,
#endif /* MPIDI_BUILD_CH4_SHM */
} MPIDI_CH4_scatter_id_t;

typedef enum {
    MPIDI_CH4_scatterv_intercomm_id,
    MPIDI_CH4_scatterv_composition_alpha_id,
#ifdef MPIDI_BUILD_CH4_SHM
    MPIDI_CH4_scatterv_composition_beta_id,
#endif /* MPIDI_BUILD_CH4_SHM */
} MPIDI_CH4_scatterv_id_t;

#define MPIDI_CH4_BARRIER_PARAMS_DECL MPIDI_CH4_barrier_params_t ch4_barrier_params
#define MPIDI_CH4_BCAST_PARAMS_DECL MPIDI_CH4_bcast_params_t ch4_bcast_params
#define MPIDI_CH4_REDUCE_PARAMS_DECL MPIDI_CH4_reduce_params_t ch4_reduce_params
#define MPIDI_CH4_ALLREDUCE_PARAMS_DECL MPIDI_CH4_allreduce_params_t ch4_allreduce_params

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
