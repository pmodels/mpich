#ifndef CH4_COLL_CONTAINERS_H_INCLUDED
#define CH4_COLL_CONTAINERS_H_INCLUDED

/* Barrier CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Barrier_intra_local_then_nodes_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Barrier_intra_netmod_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Barrier_inter_fallback_cnt;

/* Bcast  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Bcast_intra_noderoots_local_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Bcast_intra_local_then_nodes_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Bcast_intra_netmod_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Bcast_inter_fallback_cnt;

/* Reduce  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Reduce_intra_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Reduce_intra_composition_beta_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Reduce_inter_composition_alpha_cnt;

/* Allreduce  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Allreduce_intra_local_node_bcast_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Allreduce_intra_netmod_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Allreduce_inter_fallback_cnt;

/* Alltoall  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Alltoall_intra_netmod_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Alltoall_inter_composition_alpha_cnt;

/* Alltoallv  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Alltoallv_intra_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Alltoallv_inter_composition_alpha_cnt;

/* Alltoallw  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Alltoallw_intra_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Alltoallw_inter_composition_alpha_cnt;

/* Allgather  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Allgather_intra_netmod_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Allgather_inter_fallback_cnt;

/* Allgatherv  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Allgatherv_intra_netmod_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Allgatherv_inter_fallback_cnt;

/* Gather  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Gather_intra_netmod_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Gather_inter_fallback_cnt;

/* Gatherv  CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Gatherv_intra_netmod_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Gatherv_inter_fallback_cnt;

/* Scatter CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Scatter_intra_netmod_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Scatter_inter_fallback_cnt;

/* Scatterv CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Scatterv_intra_netmod_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Scatterv_inter_fallback_cnt;

/* Reduce_scatter CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Reduce_scatter_intra_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Reduce_scatter_inter_composition_alpha_cnt;

/* Reduce_scatter_block CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Reduce_scatter_block_intra_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Reduce_scatter_block_inter_composition_alpha_cnt;

/* Scan CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Scan_intra_composition_alpha_cnt;
extern const MPIDI_coll_algo_container_t MPIDI_Scan_intra_composition_beta_cnt;

/* Exscan CH4 level containers declaration */
extern const MPIDI_coll_algo_container_t MPIDI_Exscan_intra_composition_alpha_cnt;

#endif /* CH4_COLL_CONTAINERS_H_INCLUDED */
