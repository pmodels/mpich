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
} algo_parameters_t;

#endif /* CH4COLLPARAMS_H_INCLUDED */