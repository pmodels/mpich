#ifndef CH4_COLL_PARAMS_H_INCLUDED
#define CH4_COLL_PARAMS_H_INCLUDED

typedef enum {
    MPIDI_CH4_barrier_composition_alpha_id,
    MPIDI_CH4_barrier_composition_beta_id,
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
} MPIDI_CH4_barrier_params_t;

typedef enum {
    MPIDI_CH4_bcast_composition_alpha_id,
    MPIDI_CH4_bcast_composition_beta_id,
    MPIDI_CH4_bcast_composition_gamma_id,
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
} MPIDI_CH4_bcast_params_t;

typedef enum {
    MPIDI_CH4_reduce_composition_alpha_id,
    MPIDI_CH4_reduce_composition_beta_id,
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
} MPIDI_CH4_reduce_params_t;

typedef enum {
    MPIDI_CH4_allreduce_composition_alpha_id,
    MPIDI_CH4_allreduce_composition_beta_id,
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
} MPIDI_CH4_allreduce_params_t;

typedef enum {
    MPIDI_CH4_alltoall_composition_alpha_id,
    MPIDI_CH4_alltoall_intercomm_id
} MPIDI_CH4_alltoall_id_t;

typedef union {
    struct MPIDI_CH4_alltoall_alpha {
        int alltoall;
    } ch4_alltoall_alpha;
} MPIDI_CH4_alltoall_params_t;

typedef enum {
    MPIDI_CH4_alltoallv_composition_alpha_id,
    MPIDI_CH4_alltoallv_intercomm_id
} MPIDI_CH4_alltoallv_id_t;

typedef union {
    struct MPIDI_CH4_alltoallv_alpha {
        int alltoallv;
    } ch4_alltoallv_alpha;
} MPIDI_CH4_alltoallv_params_t;

typedef enum {
    MPIDI_CH4_alltoallw_composition_alpha_id,
    MPIDI_CH4_alltoallw_intercomm_id
} MPIDI_CH4_alltoallw_id_t;

typedef union {
    struct MPIDI_CH4_alltoallw_alpha {
        int alltoallw;
    } ch4_alltoallw_alpha;
} MPIDI_CH4_alltoallw_params_t;

typedef enum {
    MPIDI_CH4_allgather_composition_alpha_id,
    MPIDI_CH4_allgather_intercomm_id
} MPIDI_CH4_allgather_id_t;

typedef union {
    struct MPIDI_CH4_allgather_alpha {
        int allgather;
    } ch4_allgather_alpha;
} MPIDI_CH4_allgather_params_t;

typedef enum {
    MPIDI_CH4_allgatherv_composition_alpha_id,
    MPIDI_CH4_allgatherv_intercomm_id
} MPIDI_CH4_allgatherv_id_t;

typedef union {
    struct MPIDI_CH4_allgatherv_alpha {
        int allgatherv;
    } ch4_allgatherv_alpha;
} MPIDI_CH4_allgatherv_params_t;

#define MPIDI_CH4_BARRIER_PARAMS_DECL MPIDI_CH4_barrier_params_t ch4_barrier_params;
#define MPIDI_CH4_BCAST_PARAMS_DECL MPIDI_CH4_bcast_params_t ch4_bcast_params;
#define MPIDI_CH4_REDUCE_PARAMS_DECL MPIDI_CH4_reduce_params_t ch4_reduce_params;
#define MPIDI_CH4_ALLREDUCE_PARAMS_DECL MPIDI_CH4_allreduce_params_t ch4_allreduce_params;
#define MPIDI_CH4_ALLTOALL_PARAMS_DECL MPIDI_CH4_alltoall_params_t ch4_alltoall_params;
#define MPIDI_CH4_ALLTOALLV_PARAMS_DECL MPIDI_CH4_alltoallv_params_t ch4_alltoallv_params;
#define MPIDI_CH4_ALLTOALLW_PARAMS_DECL MPIDI_CH4_alltoallw_params_t ch4_alltoallw_params;
#define MPIDI_CH4_ALLGATHER_PARAMS_DECL MPIDI_CH4_allgather_params_t ch4_allgather_params;
#define MPIDI_CH4_ALLGATHERV_PARAMS_DECL MPIDI_CH4_allgatherv_params_t ch4_allgatherv_params;

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
} MPIDI_CH4_coll_params_t;

typedef struct MPIDI_coll_algo_container {
    int id;
    MPIDI_CH4_coll_params_t params;
} MPIDI_coll_algo_container_t;

#endif /* CH4_COLL_PARAMS_H_INCLUDED */
