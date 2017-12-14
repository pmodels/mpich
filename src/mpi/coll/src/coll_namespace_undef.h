/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* defines */
#undef MPIR_ALGO_USE_KNOMIAL
#undef MPIR_ALGO_TREE_RADIX_DEFAULT
#undef MPIR_ALGO_MAX_TREE_BREADTH

/* Common Collective Types */
#undef MPIR_ALGO_dt_t
#undef MPIR_ALGO_op_t
#undef MPIR_ALGO_comm_t
#undef MPIR_ALGO_req_t
#undef MPIR_ALGO_sched_t
#undef MPIR_ALGO_aint_t
#undef MPIR_ALGO_global_t
#undef MPIR_ALGO_args_t
#undef MPIR_ALGO_global

/* Tree collective types */
#undef MPIR_ALGO_command_t
#undef MPIR_ALGO_tree_t
#undef MPIR_ALGO_tree_comm_t
#undef MPIR_ALGO_child_range_t
#undef MPIR_ALGO_tree_add_child

/* Tree API */
#undef MPIR_ALGO_tree_init
#undef MPIR_ALGO_tree_free
#undef MPIR_ALGO_tree_dump
#undef MPIR_ALGO_tree_kary
#undef MPIR_ALGO_tree_knomial
#undef MPIR_ALGO_tree_kary_init
#undef MPIR_ALGO_tree_knomial_init
#undef MPIR_ALGO_ilog
#undef MPIR_ALGO_ipow
#undef MPIR_ALGO_getdigit
#undef MPIR_ALGO_setdigit

/* Collective API */
#undef MPIR_ALGO_init
#undef MPIR_ALGO_comm_init
#undef MPIR_ALGO_comm_init_null
#undef MPIR_ALGO_comm_cleanup
#undef MPIR_ALGO_op_init
#undef MPIR_ALGO_dt_init

/* Schedule API */
#undef MPIR_ALGO_sched_init
#undef MPIR_ALGO_sched_reset
#undef MPIR_ALGO_sched_free
#undef MPIR_ALGO_sched_init_nb
#undef MPIR_ALGO_sched_wait
#undef MPIR_ALGO_sched_test
#undef MPIR_ALGO_sched_barrier_dissem
#undef MPIR_ALGO_sched_allreduce_dissem
#undef MPIR_ALGO_sched_allreduce
#undef MPIR_ALGO_sched_allgather
#undef MPIR_ALGO_sched_bcast
#undef MPIR_ALGO_sched_bcast_tree
#undef MPIR_ALGO_sched_bcast_tree_pipelined
#undef MPIR_ALGO_bcast_get_tree_schedule
#undef MPIR_ALGO_sched_reduce_full
#undef MPIR_ALGO_sched_reduce_tree_full
#undef MPIR_ALGO_sched_reduce_tree_full_pipelined
#undef MPIR_ALGO_sched_barrier
#undef MPIR_ALGO_sched_barrier_tree
#undef MPIR_ALGO_sched_allreduce_recexch
#undef MPIR_ALGO_sched_allgather_recexch
#undef MPIR_ALGO_sched_reduce_full
#undef MPIR_ALGO_sched_reduce
#undef MPIR_ALGO_sched_reduce_tree
#undef MPIR_ALGO_sched_barrier
#undef MPIR_ALGO_get_neighbors_recexch
#undef MPIR_ALGO_sched_allgather_recexch_data_exchange
#undef MPIR_ALGO_sched_allgather_recexch_step1
#undef MPIR_ALGO_sched_allgather_recexch_step2
#undef MPIR_ALGO_sched_allgather_recexch_step3
#undef MPIR_ALGO_sched_alltoall_scattered
#undef MPIR_ALGO_sched_alltoall_pairwise
#undef MPIR_ALGO_sched_alltoall_ring
#undef MPIR_ALGO_sched_alltoall_undef
#undef MPIR_ALGO_sched_alltoallv_scattered
#undef MPIR_ALGO_sched_allgather_ring
#undef MPIR_ALGO_sched_alltoall
#undef MPIR_ALGO_brucks_pup
#undef MPIR_ALGO_get_key_size

/* Collective API */
#undef MPIR_ALGO_kick_nb
#undef MPIR_ALGO_allgather
#undef MPIR_ALGO_allgatherv
#undef MPIR_ALGO_allreduce
#undef MPIR_ALGO_alltoall
#undef MPIR_ALGO_alltoallv
#undef MPIR_ALGO_alltoallw
#undef MPIR_ALGO_bcast
#undef MPIR_ALGO_exscan
#undef MPIR_ALGO_gather
#undef MPIR_ALGO_gatherv
#undef MPIR_ALGO_reduce_scatter
#undef MPIR_ALGO_reduce_scatter_block
#undef MPIR_ALGO_reduce
#undef MPIR_ALGO_scan
#undef MPIR_ALGO_scatter
#undef MPIR_ALGO_scatterv
#undef MPIR_ALGO_barrier
#undef MPIR_ALGO_iallgather
#undef MPIR_ALGO_iallgatherv
#undef MPIR_ALGO_iallreduce
#undef MPIR_ALGO_ialltoall
#undef MPIR_ALGO_ialltoallv
#undef MPIR_ALGO_ialltoallw
#undef MPIR_ALGO_ibcast
#undef MPIR_ALGO_iexscan
#undef MPIR_ALGO_igather
#undef MPIR_ALGO_igatherv
#undef MPIR_ALGO_ireduce_scatter
#undef MPIR_ALGO_ireduce_scatter_block
#undef MPIR_ALGO_ireduce
#undef MPIR_ALGO_iscan
#undef MPIR_ALGO_iscatter
#undef MPIR_ALGO_iscatterv
#undef MPIR_ALGO_ibarrier
#undef MPIR_ALGO_neighbor_allgather
#undef MPIR_ALGO_neighbor_allgatherv
#undef MPIR_ALGO_neighbor_alltoall
#undef MPIR_ALGO_neighbor_alltoallv
#undef MPIR_ALGO_neighbor_alltoallw
#undef MPIR_ALGO_ineighbor_allgather
#undef MPIR_ALGO_ineighbor_allgatherv
#undef MPIR_ALGO_ineighbor_alltoall
#undef MPIR_ALGO_ineighbor_alltoallv
#undef MPIR_ALGO_ineighbor_alltoallw

#undef MPIR_ALGO_COMM_BASE
#undef MPIR_ALGO_DT_BASE
#undef MPIR_ALGO_OP_BASE
#undef MPIR_ALGO_FIELD_NAME

#undef MPIR_ALGO_NAME
#undef MPIR_ALGO_NAME_LC
#undef MPIR_ALGO_NAMESPACE
