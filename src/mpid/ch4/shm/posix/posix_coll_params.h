#ifndef POSIXCOLLPARAMS_H_INCLUDED
#define POSIXCOLLPARAMS_H_INCLUDED

typedef union {
    //reserved for parameters related to SHM specific collectives
    struct shm_bcast_knomial_parameters{
        int radix;
        int64_t block_size;
    } shm_bcast_knomial_parameters;
    struct shm_bcast_shumilin_parameters{
        int segment_size;
    } shm_bcast_shumilin_parameters;
    struct shm_bcast_empty_parameters{
        int empty;
    } shm_bcast_empty_parameters;
    struct shm_reduce_empty_parameters{
        int empty;
    } shm_reduce_empty_parameters;
    struct shm_allreduce_empty_parameters{
        int empty;
    } shm_allreduce_empty_parameters;
} MPIDI_POSIX_coll_params_t;

#define MPIDI_POSIX_COLL_PARAMS_DECL MPIDI_POSIX_coll_params_t shm_parameters;

static const MPIDI_POSIX_coll_params_t MPIDI_CH4_bcast_shm_param_defaults[] = {
    {
        .shm_bcast_knomial_parameters.radix = 2,
        .shm_bcast_knomial_parameters.block_size = 1024,
    },
    {
        .shm_bcast_shumilin_parameters.segment_size = 4096,
    },
    {
        .shm_bcast_knomial_parameters.radix = 2,
        .shm_bcast_knomial_parameters.block_size = 2048,
    },
    {
        .shm_bcast_knomial_parameters.radix = 2,
        .shm_bcast_knomial_parameters.block_size = 4096,
    },
};

static const MPIDI_POSIX_coll_params_t MPIDI_CH4_reduce_shm_param_defaults[];

static const MPIDI_POSIX_coll_params_t MPIDI_CH4_allreduce_shm_param_defaults[];

#endif /* POSIXCOLLPARAMS_H_INCLUDED */