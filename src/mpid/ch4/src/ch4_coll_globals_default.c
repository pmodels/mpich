#include "coll_algo_params.h"
#include <mpidimpl.h>
#include "ch4_impl.h"

/* container traverse function to be implemented  */
/* e.g. return ((void *) ((char *)container) + sizeof(MPIDI_COLL_ALGO_params_t)); */
void * MPIDI_coll_get_next_container(void * container){
    return NULL;
}

const MPIDI_coll_algo_container_t CH4_bcast_composition_alpha_cnt =
    {
        .id = MPIDI_CH4_bcast_composition_alpha_id
    };

const MPIDI_coll_algo_container_t CH4_bcast_composition_beta_cnt =
    {
        .id = MPIDI_CH4_bcast_composition_beta_id
    };

const MPIDI_coll_algo_container_t CH4_bcast_composition_gamma_cnt =
    {
        .id = MPIDI_CH4_bcast_composition_gamma_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_bcast_binomial_cnt =
    {
        .id = MPIDI_OFI_bcast_binomial_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_bcast_scatter_doubling_allgather_cnt =
    {
        .id = MPIDI_OFI_bcast_scatter_doubling_allgather_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_bcast_scatter_ring_allgather_cnt =
    {
        .id = MPIDI_OFI_bcast_scatter_ring_allgather_id
    };

#ifdef MPIDI_BUILD_CH4_SHM
const MPIDI_POSIX_coll_algo_container_t POSIX_bcast_binomial_cnt =
    {
        .id = MPIDI_POSIX_bcast_binomial_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_bcast_scatter_doubling_allgather_cnt =
    {
        .id = MPIDI_POSIX_bcast_scatter_doubling_allgather_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_bcast_scatter_ring_allgather_cnt =
    {
        .id = MPIDI_POSIX_bcast_scatter_ring_allgather_id
    };
#endif /* MPIDI_BUILD_CH4_SHM */


const MPIDI_coll_algo_container_t CH4_reduce_composition_alpha_cnt =
    {
        .id = MPIDI_CH4_reduce_composition_alpha_id
    };

const MPIDI_coll_algo_container_t CH4_reduce_composition_beta_cnt =
    {
        .id = MPIDI_CH4_reduce_composition_beta_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_reduce_redscat_gather_cnt =
    {
        .id = MPIDI_OFI_reduce_redscat_gather_id
    };

const MPIDI_OFI_coll_algo_container_t OFI_reduce_binomial_cnt =
    {
        .id = MPIDI_OFI_reduce_binomial_id
    };

#ifdef MPIDI_BUILD_CH4_SHM
const MPIDI_POSIX_coll_algo_container_t POSIX_reduce_redscat_gather_cnt =
    {
        .id = MPIDI_POSIX_reduce_redscat_gather_id
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_reduce_binomial_cnt =
    {
        .id = MPIDI_POSIX_reduce_binomial_id
    };
#endif /* MPIDI_BUILD_CH4_SHM */


const MPIDI_coll_algo_container_t CH4_allreduce_composition_alpha_cnt =
    {
        .id = MPIDI_CH4_reduce_composition_alpha_id
    };


const MPIDI_coll_algo_container_t CH4_allreduce_composition_beta_cnt =
    {
        .id = MPIDI_CH4_reduce_composition_beta_id
    };

const MPIDI_coll_algo_container_t CH4_allreduce_composition_gamma_cnt =
    {
        .id = MPIDI_CH4_allreduce_composition_gamma_id,
    };
const MPIDI_OFI_coll_algo_container_t OFI_allreduce_recursive_doubling_cnt =
    {
        .id = MPIDI_OFI_allreduce_recursive_doubling_id,
    };

const MPIDI_OFI_coll_algo_container_t OFI_allreduce_reduce_scatter_allgather_cnt =
    {
        .id = MPIDI_OFI_allreduce_reduce_scatter_allgather_id,
    };
#ifdef MPIDI_BUILD_CH4_SHM
const MPIDI_POSIX_coll_algo_container_t POSIX_allreduce_recursive_doubling_cnt =
    {
        .id = MPIDI_POSIX_allreduce_recursive_doubling_id,
    };

const MPIDI_POSIX_coll_algo_container_t POSIX_allreduce_reduce_scatter_allgather_cnt =
    {
        .id = MPIDI_POSIX_allreduce_reduce_scatter_allgather_id,
    };
#endif/* MPIDI_BUILD_CH4_SHM */
