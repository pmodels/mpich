#include <mpidimpl.h>
#include "ch4_impl.h"

/* empty container */
const MPIDI_coll_algo_container_t MPIDI_empty_cnt = {
    .id = -1,
};

/* Barrier default composition containers initialization*/
const MPIDI_coll_algo_container_t MPIDI_Barrier_intra_composition_alpha_cnt = {
    .id = MPIDI_Barrier_intra_composition_alpha_id
};

const MPIDI_coll_algo_container_t MPIDI_Barrier_intra_composition_beta_cnt = {
    .id = MPIDI_Barrier_intra_composition_beta_id,
};

/* Bcast default composition containers initialization*/
const MPIDI_coll_algo_container_t MPIDI_Bcast_intra_composition_alpha_cnt = {
    .id = MPIDI_Bcast_intra_composition_alpha_id,
};

const MPIDI_coll_algo_container_t MPIDI_Bcast_intra_composition_beta_cnt = {
    .id = MPIDI_Bcast_intra_composition_beta_id,
};

const MPIDI_coll_algo_container_t MPIDI_Bcast_intra_composition_gamma_cnt = {
    .id = MPIDI_Bcast_intra_composition_gamma_id,
};

/* Reduce default composition containers initialization*/
const MPIDI_coll_algo_container_t MPIDI_Reduce_intra_composition_alpha_cnt = {
    .id = MPIDI_Reduce_intra_composition_alpha_id,
};

const MPIDI_coll_algo_container_t MPIDI_Reduce_intra_composition_beta_cnt = {
    .id = MPIDI_Reduce_intra_composition_beta_id,
};

const MPIDI_coll_algo_container_t MPIDI_Reduce_intra_composition_gamma_cnt = {
    .id = MPIDI_Reduce_intra_composition_gamma_id,
};

/* Allreduce default composition containers initialization*/
const MPIDI_coll_algo_container_t MPIDI_Allreduce_intra_composition_alpha_cnt = {
    .id = MPIDI_Allreduce_intra_composition_alpha_id,
};

const MPIDI_coll_algo_container_t MPIDI_Allreduce_intra_composition_beta_cnt = {
    .id = MPIDI_Allreduce_intra_composition_beta_id,
};

const MPIDI_coll_algo_container_t MPIDI_Allreduce_intra_composition_gamma_cnt = {
    .id = MPIDI_Allreduce_intra_composition_gamma_id,
};

/* Gather default composition containers initialization */
const MPIDI_coll_algo_container_t MPIDI_Gather_intra_composition_alpha_cnt = {
    .id = MPIDI_Gather_intra_composition_alpha_id,
};

/* Gatherv default composition containers initialization */
const MPIDI_coll_algo_container_t MPIDI_Gatherv_intra_composition_alpha_cnt = {
    .id = MPIDI_Gatherv_intra_composition_alpha_id,
};

/* Scatter default composition containers initialization */
const MPIDI_coll_algo_container_t MPIDI_Scatter_intra_composition_alpha_cnt = {
    .id = MPIDI_Scatter_intra_composition_alpha_id,
};

/* Scatterv default composition containers initialization */
const MPIDI_coll_algo_container_t MPIDI_Scatterv_intra_composition_alpha_cnt = {
    .id = MPIDI_Scatterv_intra_composition_alpha_id,
};

/* Alltoall default containers initialization */
const MPIDI_coll_algo_container_t MPIDI_Alltoall_intra_composition_alpha_cnt = {
    .id = MPIDI_Alltoall_intra_composition_alpha_id,
};

/* Alltoallv default containers initialization */
const MPIDI_coll_algo_container_t MPIDI_Alltoallv_intra_composition_alpha_cnt = {
    .id = MPIDI_Alltoallv_intra_composition_alpha_id,
};

/* Alltoallw default containers initialization */
const MPIDI_coll_algo_container_t MPIDI_Alltoallw_intra_composition_alpha_cnt = {
    .id = MPIDI_Alltoallw_intra_composition_alpha_id,
};

/* Allgather default containers initialization */
const MPIDI_coll_algo_container_t MPIDI_Allgather_intra_composition_alpha_cnt = {
    .id = MPIDI_Allgather_intra_composition_alpha_id,
};

/* Allgatherv default containers initialization */
const MPIDI_coll_algo_container_t MPIDI_Allgatherv_intra_composition_alpha_cnt = {
    .id = MPIDI_Allgatherv_intra_composition_alpha_id,
};

/* Reduce_scatter default composition containers initialization */
const MPIDI_coll_algo_container_t MPIDI_Reduce_scatter_intra_composition_alpha_cnt = {
    .id = MPIDI_Reduce_scatter_intra_composition_alpha_id,
};

/* Reduce_scatter_block default composition containers initialization */
const MPIDI_coll_algo_container_t MPIDI_Reduce_scatter_block_intra_composition_alpha_cnt = {
    .id = MPIDI_Reduce_scatter_block_intra_composition_alpha_id,
};

/* Scan default containers initialization */
const MPIDI_coll_algo_container_t MPIDI_Scan_intra_composition_alpha_cnt = {
    .id = MPIDI_Scan_intra_composition_alpha_id,
};

const MPIDI_coll_algo_container_t MPIDI_Scan_intra_composition_beta_cnt = {
    .id = MPIDI_Scan_intra_composition_beta_id,
};

/* Exscan default containers initialization */
const MPIDI_coll_algo_container_t MPIDI_Exscan_intra_composition_alpha_cnt = {
    .id = MPIDI_Exscan_intra_composition_alpha_id,
};
