#ifndef OFICOLLPARAMS_H_INCLUDED
#define OFICOLLPARAMS_H_INCLUDED

typedef union {
    // reserved for parameters related to NETMOD specific collectives
    struct nm_bcast_knomial_parameters{
        int radix;
        int64_t block_size;
    } nm_bcast_knomial_parameters;
    struct nm_bcast_shumilin_parameters{
        int segment_size;
    } nm_bcast_shumilin_parameters;
    struct nm_bcast_empty_parameters{
        int empty;
    } nm_bcast_empty_parameters;
    struct nm_reduce_empty_parameters{
        int empty;
    } nm_reduce_empty_parameters;
    struct nm_allreduce_empty_parameters{
        int empty;
    } nm_allreduce_empty_parameters;
} MPIDI_OFI_coll_params_t;

#define MPIDI_OFI_COLL_PARAMS_DECL MPIDI_OFI_coll_params_t nm_parameters;

static const MPIDI_OFI_coll_params_t MPIDI_CH4_bcast_nm_param_defaults[] = {
    {
        .nm_bcast_knomial_parameters.radix = 2,
        .nm_bcast_knomial_parameters.block_size = 1024,
    },
    {
        .nm_bcast_shumilin_parameters.segment_size = 4096,
    },
    {
        .nm_bcast_knomial_parameters.radix = 2,
        .nm_bcast_knomial_parameters.block_size = 2048,
    },
    {
        .nm_bcast_knomial_parameters.radix = 2,
        .nm_bcast_knomial_parameters.block_size = 4096,
    },
};

static const MPIDI_OFI_coll_params_t MPIDI_CH4_reduce_nm_param_defaults[];

static const MPIDI_OFI_coll_params_t MPIDI_CH4_allreduce_nm_param_defaults[];

#endif /* OFICOLLPARAMS_H_INCLUDED */