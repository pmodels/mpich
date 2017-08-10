#ifndef COLL_ALGO_PARAMS_H_INCLUDED
#define COLL_ALGO_PARAMS_H_INCLUDED

#include "../src/ch4_coll_params.h"
#include "coll_tuning_types.h"

#define MPIDI_CH4_COLL_AUTO_SELECT (-1)

typedef union MPIDI_barrier_algo_params {
    /* *INDENT-OFF* */
    MPIDI_CH4_BARRIER_PARAMS_DECL;
    /* *INDENT-ON* */
} MPIDI_BARRIER_ALGO_params_t;

typedef union MPIDI_bcast_algo_params {
    /* *INDENT-OFF* */
    MPIDI_CH4_BCAST_PARAMS_DECL;
    /* *INDENT-ON* */
} MPIDI_BCAST_ALGO_params_t;

typedef union MPIDI_reduce_algo_params {
    /* *INDENT-OFF* */
    MPIDI_CH4_REDUCE_PARAMS_DECL;
    /* *INDENT-ON* */
} MPIDI_REDUCE_ALGO_params_t;

typedef union MPIDI_allreduce_algo_params {
    /* *INDENT-OFF* */
    MPIDI_CH4_ALLREDUCE_PARAMS_DECL;
    /* *INDENT-ON* */
} MPIDI_ALLREDUCE_ALGO_params_t;

#define MPIDI_BARRIER_PARAMS_DECL MPIDI_BARRIER_ALGO_params_t barrier_params;
#define MPIDI_BCAST_PARAMS_DECL MPIDI_BCAST_ALGO_params_t bcast_params;
#define MPIDI_REDUCE_PARAMS_DECL MPIDI_REDUCE_ALGO_params_t reduce_params;
#define MPIDI_ALLREDUCE_PARAMS_DECL MPIDI_ALLREDUCE_ALGO_params_t allreduce_params;

typedef union MPIDI_coll_algo_params {
    MPIDI_BARRIER_PARAMS_DECL;
    MPIDI_BCAST_PARAMS_DECL;
    MPIDI_REDUCE_PARAMS_DECL;
    MPIDI_ALLREDUCE_PARAMS_DECL;
} MPIDI_COLL_ALGO_params_t;

#endif /* COLL_ALGO_PARAMS_H_INCLUDED */
