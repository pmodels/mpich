#ifndef CH4COLLPARAMS_H_INCLUDED
#define CH4COLLPARAMS_H_INCLUDED


typedef union algo_parameters {
    struct bcast_knomial_parameters{
        int radix;
        int64_t block_size;
    } bcast_knomial_parameters;
    struct bcast_shumilin_parameters{
        int segment_size;
    } bcast_shumilin_parameters;
    struct bcast_empty_parameters{
        int empty;
    } bcast_empty_parameters;
    struct reduce_empty_parameters{
        int empty;
    } reduce_empty_parameters;
    struct allreduce_empty_parameters{
        int empty;
    } allreduce_empty_parameters;
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
    }
};

static const MPIDI_algo_parameters_t MPIDI_NM_bcast_param_defaults[] = {
    {
        .bcast_knomial_parameters.radix = 2,
        .bcast_knomial_parameters.block_size = 1024,
    },
    {
        .bcast_shumilin_parameters.segment_size = 4096,
    },
    {
        .bcast_knomial_parameters.radix = 2,
        .bcast_knomial_parameters.block_size = 2048,
    },
    {
        .bcast_knomial_parameters.radix = 2,
        .bcast_knomial_parameters.block_size = 4096,
    },
};

static const MPIDI_algo_parameters_t MPIDI_SHM_bcast_param_defaults[] = {
    {
        .bcast_knomial_parameters.radix = 2,
        .bcast_knomial_parameters.block_size = 2048,
    },
    {
        .bcast_shumilin_parameters.segment_size = 4096,
    },
    {
        .bcast_knomial_parameters.radix = 2,
        .bcast_knomial_parameters.block_size = 2048,
    },
    {
        .bcast_knomial_parameters.radix = 2,
        .bcast_knomial_parameters.block_size = 4096,
    },
};

static const MPIDI_algo_parameters_t MPIDI_NM_reduce_param_defaults[]; 

static const MPIDI_algo_parameters_t MPIDI_SHM_reduce_param_defaults[];

static const MPIDI_algo_parameters_t MPIDI_NM_allreduce_param_defaults[];

static const MPIDI_algo_parameters_t MPIDI_SHM_allreduce_param_defaults[];

#endif /* CH4COLLPARAMS_H_INCLUDED */