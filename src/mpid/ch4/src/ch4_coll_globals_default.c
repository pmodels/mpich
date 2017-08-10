#include "coll_algo_params.h"
#include <mpidimpl.h>
#include "ch4_impl.h"
 
/* Barrier default containers initialization*/
const MPIDI_coll_algo_container_t CH4_barrier_composition_alpha_cnt =
    {
        .id = MPIDI_CH4_barrier_composition_alpha_id,
    };

const MPIDI_coll_algo_container_t CH4_barrier_composition_beta_cnt =
    {
        .id = MPIDI_CH4_barrier_composition_beta_id,
    };

const MPIDI_coll_algo_container_t CH4_barrier_intercomm_cnt =
    {
        .id = MPIDI_CH4_barrier_intercomm_id,
    };

  /* Bcast default containers initialization*/
const MPIDI_coll_algo_container_t CH4_bcast_composition_alpha_cnt =
    {
        .id = MPIDI_CH4_bcast_composition_alpha_id,
    };

const MPIDI_coll_algo_container_t CH4_bcast_composition_beta_cnt =
    {
        .id = MPIDI_CH4_bcast_composition_beta_id,
    };

const MPIDI_coll_algo_container_t CH4_bcast_composition_gamma_cnt =
    {
        .id = MPIDI_CH4_bcast_composition_gamma_id,
    };

const MPIDI_coll_algo_container_t CH4_bcast_intercomm_cnt =
    {
        .id = MPIDI_CH4_bcast_intercomm_id
    };

 /* Reduce default containers initialization*/
const MPIDI_coll_algo_container_t CH4_reduce_composition_alpha_cnt =
    {
        .id = MPIDI_CH4_reduce_composition_alpha_id,
    };

const MPIDI_coll_algo_container_t CH4_reduce_composition_beta_cnt =
    {
        .id = MPIDI_CH4_reduce_composition_beta_id,
    };

const MPIDI_coll_algo_container_t CH4_reduce_intercomm_cnt =
    {
        .id = MPIDI_CH4_reduce_intercomm_id
    };

 /* Allreduce default containers initialization*/
const MPIDI_coll_algo_container_t CH4_allreduce_composition_alpha_cnt =
    {
        .id = MPIDI_CH4_allreduce_composition_alpha_id,
    };

const MPIDI_coll_algo_container_t CH4_allreduce_composition_beta_cnt =
    {
        .id = MPIDI_CH4_allreduce_composition_beta_id,
    };

const MPIDI_coll_algo_container_t CH4_allreduce_intercomm_cnt =
    {
        .id = MPIDI_CH4_allreduce_intercomm_id
    };
