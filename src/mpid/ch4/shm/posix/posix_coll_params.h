#ifndef POSIXCOLLPARAMS_H_INCLUDED
#define POSIXCOLLPARAMS_H_INCLUDED

typedef union {
    //reserved for parameters related to SHM specific collectives
    struct posix_bcast_knomial_parameters{
        int radix;
        int64_t block_size;
    } posix_bcast_knomial_parameters;
    struct posix_bcast_shumilin_parameters{
        int segment_size;
    } posix_bcast_shumilin_parameters;
    struct posix_bcast_empty_parameters{
        int empty;
    } posix_bcast_empty_parameters;
    struct posix_reduce_empty_parameters{
        int empty;
    } posix_reduce_empty_parameters;
    struct posix_allreduce_empty_parameters{
        int empty;
    } posix_allreduce_empty_parameters;
} MPIDI_POSIX_coll_params_t;

#define MPIDI_POSIX_COLL_PARAMS_DECL MPIDI_POSIX_coll_params_t posix_parameters;

static const MPIDI_POSIX_coll_params_t MPIDI_CH4_bcast_posix_param_defaults[] = {
    {
        .posix_bcast_knomial_parameters.radix = 2,
        .posix_bcast_knomial_parameters.block_size = 1024,
    },
    {
        .posix_bcast_shumilin_parameters.segment_size = 4096,
    },
    {
        .posix_bcast_knomial_parameters.radix = 2,
        .posix_bcast_knomial_parameters.block_size = 2048,
    },
    {
        .posix_bcast_knomial_parameters.radix = 2,
        .posix_bcast_knomial_parameters.block_size = 4096,
    },
};

static const MPIDI_POSIX_coll_params_t MPIDI_CH4_reduce_posix_param_defaults[];

static const MPIDI_POSIX_coll_params_t MPIDI_CH4_allreduce_posix_param_defaults[];

#endif /* POSIXCOLLPARAMS_H_INCLUDED */