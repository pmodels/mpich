#ifndef OFICOLLPARAMS_H_INCLUDED
#define OFICOLLPARAMS_H_INCLUDED

typedef union {
    // reserved for parameters related to NETMOD specific collectives
    struct ofi_bcast_knomial_parameters{
        int radix;
        int64_t block_size;
    } ofi_bcast_knomial_parameters;
    struct ofi_bcast_shumilin_parameters{
        int segment_size;
    } ofi_bcast_shumilin_parameters;
    struct ofi_bcast_empty_parameters{
        int empty;
    } ofi_bcast_empty_parameters;
    struct ofi_reduce_empty_parameters{
        int empty;
    } ofi_reduce_empty_parameters;
    struct ofi_allreduce_empty_parameters{
        int empty;
    } ofi_allreduce_empty_parameters;
} MPIDI_OFI_coll_params_t;

#define MPIDI_OFI_COLL_PARAMS_DECL MPIDI_OFI_coll_params_t ofi_parameters;

static const MPIDI_OFI_coll_params_t MPIDI_CH4_bcast_ofi_param_defaults[] = {
    {
        .ofi_bcast_knomial_parameters.radix = 2,
        .ofi_bcast_knomial_parameters.block_size = 1024,
    },
    {
        .ofi_bcast_shumilin_parameters.segment_size = 4096,
    },
    {
        .ofi_bcast_knomial_parameters.radix = 2,
        .ofi_bcast_knomial_parameters.block_size = 2048,
    },
    {
        .ofi_bcast_knomial_parameters.radix = 2,
        .ofi_bcast_knomial_parameters.block_size = 4096,
    },
};

static const MPIDI_OFI_coll_params_t MPIDI_CH4_reduce_ofi_param_defaults[];

static const MPIDI_OFI_coll_params_t MPIDI_CH4_allreduce_ofi_param_defaults[];

#endif /* OFICOLLPARAMS_H_INCLUDED */