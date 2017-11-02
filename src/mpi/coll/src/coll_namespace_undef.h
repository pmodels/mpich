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
#undef COLL_USE_KNOMIAL
#undef COLL_TREE_RADIX_DEFAULT
#undef COLL_MAX_TREE_BREADTH

/* Common Collective Types */
#undef COLL_dt_t
#undef COLL_op_t
#undef COLL_comm_t
#undef COLL_req_t
#undef COLL_sched_t
#undef COLL_aint_t
#undef COLL_global_t
#undef COLL_args_t
#undef COLL_global

/* Tree collective types */
#undef COLL_command_t
#undef COLL_tree_t
#undef COLL_tree_comm_t
#undef COLL_child_range_t
#undef COLL_tree_add_child

/* Tree API */
#undef COLL_tree_init
#undef COLL_tree_dump
#undef COLL_tree_kary
#undef COLL_tree_knomial
#undef COLL_tree_kary_init
#undef COLL_tree_knomial_init
#undef COLL_ilog
#undef COLL_ipow
#undef COLL_getdigit
#undef COLL_setdigit

/* Collective API */
#undef COLL_init
#undef COLL_comm_init
#undef COLL_comm_cleanup
#undef COLL_op_init
#undef COLL_dt_init

/* Schedule API */
#undef COLL_sched_init
#undef COLL_sched_reset
#undef COLL_sched_free
#undef COLL_sched_init_nb
#undef COLL_sched_wait
#undef COLL_sched_test
#undef COLL_sched_barrier_dissem
#undef COLL_sched_allreduce_dissem
#undef COLL_sched_allreduce
#undef COLL_sched_allgather
#undef COLL_sched_bcast
#undef COLL_sched_bcast_tree
#undef COLL_sched_bcast_tree_pipelined
#undef COLL_bcast_get_tree_schedule
#undef COLL_sched_reduce_full
#undef COLL_sched_reduce_tree_full
#undef COLL_sched_reduce_tree_full_pipelined
#undef COLL_sched_barrier
#undef COLL_sched_barrier_tree
#undef COLL_sched_allreduce_recexch
#undef COLL_sched_allgather_recexch
#undef COLL_sched_reduce_full
#undef COLL_sched_reduce
#undef COLL_sched_reduce_tree
#undef COLL_sched_barrier
#undef COLL_get_neighbors_recexch
#undef COLL_sched_allgather_recexch_data_exchange
#undef COLL_sched_allgather_recexch_step1
#undef COLL_sched_allgather_recexch_step2
#undef COLL_sched_allgather_recexch_step3
#undef COLL_sched_alltoall_scattered
#undef COLL_sched_alltoall_pairwise
#undef COLL_sched_alltoall_ring
#undef COLL_sched_alltoall_undef
#undef COLL_sched_alltoallv_scattered
#undef COLL_sched_allgather_ring
#undef COLL_sched_alltoall
#undef COLL_brucks_pup

/* Collective API */
#undef COLL_kick_nb
#undef COLL_allgather
#undef COLL_allgatherv
#undef COLL_allreduce
#undef COLL_alltoall
#undef COLL_alltoallv
#undef COLL_alltoallw
#undef COLL_bcast
#undef COLL_exscan
#undef COLL_gather
#undef COLL_gatherv
#undef COLL_reduce_scatter
#undef COLL_reduce_scatter_block
#undef COLL_reduce
#undef COLL_scan
#undef COLL_scatter
#undef COLL_scatterv
#undef COLL_barrier
#undef COLL_iallgather
#undef COLL_iallgatherv
#undef COLL_iallreduce
#undef COLL_ialltoall
#undef COLL_ialltoallv
#undef COLL_ialltoallw
#undef COLL_ibcast
#undef COLL_iexscan
#undef COLL_igather
#undef COLL_igatherv
#undef COLL_ireduce_scatter
#undef COLL_ireduce_scatter_block
#undef COLL_ireduce
#undef COLL_iscan
#undef COLL_iscatter
#undef COLL_iscatterv
#undef COLL_ibarrier
#undef COLL_neighbor_allgather
#undef COLL_neighbor_allgatherv
#undef COLL_neighbor_alltoall
#undef COLL_neighbor_alltoallv
#undef COLL_neighbor_alltoallw
#undef COLL_ineighbor_allgather
#undef COLL_ineighbor_allgatherv
#undef COLL_ineighbor_alltoall
#undef COLL_ineighbor_alltoallv
#undef COLL_ineighbor_alltoallw

#undef COLL_COMM_BASE
#undef COLL_DT_BASE
#undef COLL_OP_BASE
#undef COLL_FIELD_NAME

#undef COLL_NAME
#undef COLL_NAME_LC
#undef COLL_NAMESPACE
