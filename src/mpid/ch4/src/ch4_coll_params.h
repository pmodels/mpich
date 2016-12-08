#ifndef CH4COLLPARAMS_H_INCLUDED
#define CH4COLLPARAMS_H_INCLUDED


typedef union algo_parameters {
    MPIDI_NM_COLL_PARAMS_DECL;
    MPIDI_SHM_COLL_PARAMS_DECL;

    struct generic_bcast_knomial_parameters{
        int radix;
        int64_t block_size;
    } generic_bcast_knomial_parameters;
    struct generic_bcast_shumilin_parameters{
        int segment_size;
    } generic_bcast_shumilin_parameters;
    struct generic_bcast_empty_parameters{
        int empty;
    } generic_bcast_empty_parameters;
    struct generic_reduce_empty_parameters{
        int empty;
    } generic_reduce_empty_parameters;
    struct generic_allreduce_empty_parameters{
        int empty;
    } generic_allreduce_empty_parameters;    

    struct ch4_bcast{
        int nm_bcast;
        int shm_bcast;
    } ch4_bcast; 
    struct ch4_allreduce_0{
        int shm_reduce;
        int nm_allreduce;
        int shm_bcast;
    }ch4_allreduce_0;
    struct ch4_allreduce_1{
        int shm_reduce;
        int nm_reduce;
        int nm_bcast;
        int shm_bcast;
    }ch4_allreduce_1;
    struct ch4_allreduce_2{
        int nm_reduce;
        int nm_bcast;
    }ch4_allreduce_2;
    struct ch4_allreduce{
        int nm_allreduce;
        int shm_allreduce;
    }ch4_allreduce;
    struct ch4_reduce{
        int nm_reduce;
        int shm_reduce;
    }ch4_reduce;
} MPIDI_algo_parameters_t;

static const MPIDI_algo_parameters_t MPIDI_CH4_bcast_param_defaults[] = {
    {
        .ch4_bcast.nm_bcast = -1,
        .ch4_bcast.shm_bcast = -1,
    },
};

static const MPIDI_algo_parameters_t MPIDI_CH4_bcast_generic_param_defaults[] = {
    {
        .generic_bcast_knomial_parameters.radix = 2,
        .generic_bcast_knomial_parameters.block_size = 1024,
    },
    {
        .generic_bcast_shumilin_parameters.segment_size = 4096,
    },
    {
        .generic_bcast_knomial_parameters.radix = 2,
        .generic_bcast_knomial_parameters.block_size = 2048,
    },
    {
        .generic_bcast_knomial_parameters.radix = 2,
        .generic_bcast_knomial_parameters.block_size = 4096,
    },
};

static const MPIDI_algo_parameters_t MPIDI_CH4_reduce_generic_param_defaults[];

static const MPIDI_algo_parameters_t MPIDI_CH4_allreduce_generic_param_defaults[];

#endif /* CH4COLLPARAMS_H_INCLUDED */