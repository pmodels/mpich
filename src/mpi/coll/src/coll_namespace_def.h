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

#ifndef MPIR_ALGO_CAT_GUARD
#define MPIR_ALGO_CAT_GUARD

#define MPIR_ALGO_NSCAT0(a,b) a##b
#define MPIR_ALGO_NSCAT1(a,b) MPIR_ALGO_NSCAT0(a,b)

#endif /* MPIR_ALGO_CAT_GUARD */

#ifndef MPIR_ALGO_NAMESPACE
#define MPIR_ALGO_NAMESPACE(fn)  MPIR_ALGO_NSCAT1(\
                                MPIR_ALGO_NSCAT1(\
                                    MPIR_ALGO_NSCAT1(\
                                        GLOBAL_NAME,\
                                        TRANSPORT_NAME),\
                                    MPIR_ALGO_NAME),\
                                fn)
#endif /* MPIR_ALGO_NAMESPACE */

/* Common Collective Types */
#define MPIR_ALGO_dt_t         MPIR_TSP_dt_t
#define MPIR_ALGO_op_t         MPIR_TSP_op_t

#define MPIR_ALGO_comm_t       MPIR_ALGO_NAMESPACE(comm_t)
#define MPIR_ALGO_req_t        MPIR_ALGO_NAMESPACE(req_t)
#define MPIR_ALGO_sched_t      MPIR_ALGO_NAMESPACE(sched_t)
#define MPIR_ALGO_aint_t       MPIR_ALGO_NAMESPACE(aint_t)
#define MPIR_ALGO_global_t     MPIR_ALGO_NAMESPACE(global_t)
#define MPIR_ALGO_args_t       MPIR_ALGO_NAMESPACE(args_t)

/* Tree collective types */
#define MPIR_ALGO_fence_t        MPIR_ALGO_NAMESPACE(fence_t)
#define MPIR_ALGO_tree_t         MPIR_ALGO_NAMESPACE(tree_t)
#define MPIR_ALGO_tree_comm_t    MPIR_ALGO_NAMESPACE(tree_comm_t)
#define MPIR_ALGO_child_range_t  MPIR_ALGO_NAMESPACE(child_range_t)
#define MPIR_ALGO_tree_add_child MPIR_ALGO_NAMESPACE(tree_add_child)

/* Tree schedule utility API */
#define MPIR_ALGO_tree_init         MPIR_ALGO_NAMESPACE(tree_init)
#define MPIR_ALGO_tree_free         MPIR_ALGO_NAMESPACE(tree_free)
#define MPIR_ALGO_tree_dump         MPIR_ALGO_NAMESPACE(tree_dump)
#define MPIR_ALGO_tree_kary         MPIR_ALGO_NAMESPACE(tree_kary)
#define MPIR_ALGO_tree_knomial      MPIR_ALGO_NAMESPACE(tree_knomial)
#define MPIR_ALGO_tree_kary_init    MPIR_ALGO_NAMESPACE(tree_kary_init)
#define MPIR_ALGO_tree_knomial_init MPIR_ALGO_NAMESPACE(tree_knomial_init)
#define MPIR_ALGO_ilog              MPIR_ALGO_NAMESPACE(ilog)
#define MPIR_ALGO_ipow              MPIR_ALGO_NAMESPACE(ipow)
#define MPIR_ALGO_getdigit          MPIR_ALGO_NAMESPACE(getdigit)
#define MPIR_ALGO_setdigit          MPIR_ALGO_NAMESPACE(setdigit)

/* Generic collective algorithms API */
#define MPIR_ALGO_init                  MPIR_ALGO_NAMESPACE(init)
#define MPIR_ALGO_comm_init             MPIR_ALGO_NAMESPACE(comm_init)
#define MPIR_ALGO_comm_init_null        MPIR_ALGO_NAMESPACE(comm_init_null)
#define MPIR_ALGO_comm_cleanup          MPIR_ALGO_NAMESPACE(comm_cleanup)

#define MPIR_ALGO_FIELD_NAME         MPIR_ALGO_NSCAT1(MPIR_ALGO_NSCAT1(TRANSPORT_NAME_LC,_),MPIR_ALGO_NAME_LC)
#define MPIR_ALGO_COMM_BASE(addr)    container_of(addr,MPIC_comm_t, MPIR_ALGO_FIELD_NAME)
#define MPIR_ALGO_global             MPIC_global_instance.MPIR_ALGO_FIELD_NAME

/* Scheduling API */
#define MPIR_ALGO_sched_init                 MPIR_ALGO_NAMESPACE(sched_init)
#define MPIR_ALGO_sched_reset                MPIR_ALGO_NAMESPACE(sched_reset)
#define MPIR_ALGO_sched_free                 MPIR_ALGO_NAMESPACE(sched_free)
#define MPIR_ALGO_sched_init_nb              MPIR_ALGO_NAMESPACE(sched_init_nb)
#define MPIR_ALGO_sched_wait                 MPIR_ALGO_NAMESPACE(sched_wait)
#define MPIR_ALGO_sched_test                 MPIR_ALGO_NAMESPACE(sched_test)
#define MPIR_ALGO_sched_barrier_dissem       MPIR_ALGO_NAMESPACE(sched_barrier_dissem)
#define MPIR_ALGO_sched_allreduce_dissem     MPIR_ALGO_NAMESPACE(sched_allreduce_dissem)
#define MPIR_ALGO_sched_allreduce            MPIR_ALGO_NAMESPACE(sched_allreduce)
#define MPIR_ALGO_sched_allgather            MPIR_ALGO_NAMESPACE(sched_allgather)
#define MPIR_ALGO_sched_allreduce_tree       MPIR_ALGO_NAMESPACE(sched_allreduce_tree)
#define MPIR_ALGO_sched_bcast                MPIR_ALGO_NAMESPACE(sched_bcast)
#define MPIR_ALGO_sched_bcast_tree           MPIR_ALGO_NAMESPACE(sched_bcast_tree)
#define MPIR_ALGO_sched_bcast_tree_pipelined MPIR_ALGO_NAMESPACE(sched_bcast_tree_pipelined)
#define MPIR_ALGO_bcast_get_tree_schedule    MPIR_ALGO_NAMESPACE(bcast_get_tree_schedule)
#define MPIR_ALGO_sched_reduce_full          MPIR_ALGO_NAMESPACE(sched_reduce_full)
#define MPIR_ALGO_sched_reduce_tree_full     MPIR_ALGO_NAMESPACE(sched_reduce_tree_full)
#define MPIR_ALGO_sched_reduce_tree_full_pipelined   MPIR_ALGO_NAMESPACE(sched_reduce_tree_full_pipelined)
#define MPIR_ALGO_sched_barrier              MPIR_ALGO_NAMESPACE(sched_barrier)
#define MPIR_ALGO_sched_barrier_tree         MPIR_ALGO_NAMESPACE(sched_barrier_tree)
#define MPIR_ALGO_sched_allreduce_recexch    MPIR_ALGO_NAMESPACE(sched_allreduce_recexch)
#define MPIR_ALGO_sched_allgather_recexch    MPIR_ALGO_NAMESPACE(sched_allgather_recexch)
#define MPIR_ALGO_sched_reduce_full          MPIR_ALGO_NAMESPACE(sched_reduce_full)
#define MPIR_ALGO_sched_reduce               MPIR_ALGO_NAMESPACE(sched_reduce)
#define MPIR_ALGO_sched_reduce_tree          MPIR_ALGO_NAMESPACE(sched_reduce_tree)
#define MPIR_ALGO_sched_barrier              MPIR_ALGO_NAMESPACE(sched_barrier)
#define MPIR_ALGO_get_neighbors_recexch      MPIR_ALGO_NAMESPACE(get_neighbors_recexch)
#define MPIR_ALGO_sched_allgather_recexch_data_exchange    MPIR_ALGO_NAMESPACE(sched_allgather_recexch_data_exchange)
#define MPIR_ALGO_sched_allgather_recexch_step1  MPIR_ALGO_NAMESPACE(sched_allgather_recexch_step1)
#define MPIR_ALGO_sched_allgather_recexch_step2  MPIR_ALGO_NAMESPACE(sched_allgather_recexch_step2)
#define MPIR_ALGO_sched_allgather_recexch_step3  MPIR_ALGO_NAMESPACE(sched_allgather_recexch_step3)
#define MPIR_ALGO_get_neighbors_recexch      MPIR_ALGO_NAMESPACE(get_neighbors_recexch)
#define MPIR_ALGO_sched_alltoall_scattered   MPIR_ALGO_NAMESPACE(sched_alltoall_scattered)
#define MPIR_ALGO_sched_alltoall_pairwise    MPIR_ALGO_NAMESPACE(sched_alltoall_pairwise)
#define MPIR_ALGO_sched_alltoall_ring        MPIR_ALGO_NAMESPACE(sched_alltoall_ring)
#define MPIR_ALGO_sched_alltoall_brucks      MPIR_ALGO_NAMESPACE(sched_alltoall_brucks)
#define MPIR_ALGO_sched_alltoallv_scattered  MPIR_ALGO_NAMESPACE(sched_alltoallv_scattered)
#define MPIR_ALGO_sched_allgather_ring       MPIR_ALGO_NAMESPACE(sched_allgather_ring)
#define MPIR_ALGO_sched_alltoall             MPIR_ALGO_NAMESPACE(sched_alltoall)
#define MPIR_ALGO_brucks_pup                 MPIR_ALGO_NAMESPACE(brucks_pup)
#define MPIR_ALGO_get_key_size               MPIR_ALGO_NAMESPACE(get_key_size)

/* Collective API */
#define MPIR_ALGO_kick_nb               MPIR_ALGO_NAMESPACE(kick_nb)
#define MPIR_ALGO_allgather             MPIR_ALGO_NAMESPACE(allgather)
#define MPIR_ALGO_allgatherv            MPIR_ALGO_NAMESPACE(allgatherv)
#define MPIR_ALGO_allreduce             MPIR_ALGO_NAMESPACE(allreduce)
#define MPIR_ALGO_alltoall              MPIR_ALGO_NAMESPACE(alltoall)
#define MPIR_ALGO_alltoallv             MPIR_ALGO_NAMESPACE(alltoallv)
#define MPIR_ALGO_alltoallw             MPIR_ALGO_NAMESPACE(alltoallw)
#define MPIR_ALGO_bcast                 MPIR_ALGO_NAMESPACE(bcast)
#define MPIR_ALGO_exscan                MPIR_ALGO_NAMESPACE(exscan)
#define MPIR_ALGO_gather                MPIR_ALGO_NAMESPACE(gather)
#define MPIR_ALGO_gatherv               MPIR_ALGO_NAMESPACE(gatherv)
#define MPIR_ALGO_reduce_scatter        MPIR_ALGO_NAMESPACE(reduce_scatter)
#define MPIR_ALGO_reduce_scatter_block  MPIR_ALGO_NAMESPACE(reduce_scatter_block)
#define MPIR_ALGO_reduce                MPIR_ALGO_NAMESPACE(reduce)
#define MPIR_ALGO_scan                  MPIR_ALGO_NAMESPACE(scan)
#define MPIR_ALGO_scatter               MPIR_ALGO_NAMESPACE(scatter)
#define MPIR_ALGO_scatterv              MPIR_ALGO_NAMESPACE(scatterv)
#define MPIR_ALGO_barrier               MPIR_ALGO_NAMESPACE(barrier)
#define MPIR_ALGO_iallgather            MPIR_ALGO_NAMESPACE(iallgather)
#define MPIR_ALGO_iallgatherv           MPIR_ALGO_NAMESPACE(iallgatherv)
#define MPIR_ALGO_iallreduce            MPIR_ALGO_NAMESPACE(iallreduce)
#define MPIR_ALGO_ialltoall             MPIR_ALGO_NAMESPACE(ialltoall)
#define MPIR_ALGO_ialltoallv            MPIR_ALGO_NAMESPACE(ialltoallv)
#define MPIR_ALGO_ialltoallw            MPIR_ALGO_NAMESPACE(ialltoallw)
#define MPIR_ALGO_ibcast                MPIR_ALGO_NAMESPACE(ibcast)
#define MPIR_ALGO_iexscan               MPIR_ALGO_NAMESPACE(iexscan)
#define MPIR_ALGO_igather               MPIR_ALGO_NAMESPACE(igather)
#define MPIR_ALGO_igatherv              MPIR_ALGO_NAMESPACE(igatherv)
#define MPIR_ALGO_ireduce_scatter       MPIR_ALGO_NAMESPACE(ireduce_scatter)
#define MPIR_ALGO_ireduce_scatter_block MPIR_ALGO_NAMESPACE(ireduce_scatter_block)
#define MPIR_ALGO_ireduce               MPIR_ALGO_NAMESPACE(ireduce)
#define MPIR_ALGO_iscan                 MPIR_ALGO_NAMESPACE(iscan)
#define MPIR_ALGO_iscatter              MPIR_ALGO_NAMESPACE(iscatter)
#define MPIR_ALGO_iscatterv             MPIR_ALGO_NAMESPACE(iscatterv)
#define MPIR_ALGO_ibarrier              MPIR_ALGO_NAMESPACE(ibarrier)
#define MPIR_ALGO_neighbor_allgather    MPIR_ALGO_NAMESPACE(neighbor_allgather)
#define MPIR_ALGO_neighbor_allgatherv   MPIR_ALGO_NAMESPACE(neighbor_allgatherv)
#define MPIR_ALGO_neighbor_alltoall     MPIR_ALGO_NAMESPACE(neighbor_alltoall)
#define MPIR_ALGO_neighbor_alltoallv    MPIR_ALGO_NAMESPACE(neighbor_alltoallv)
#define MPIR_ALGO_neighbor_alltoallw    MPIR_ALGO_NAMESPACE(neighbor_alltoallw)
#define MPIR_ALGO_ineighbor_allgather   MPIR_ALGO_NAMESPACE(ineighbor_allgather)
#define MPIR_ALGO_ineighbor_allgatherv  MPIR_ALGO_NAMESPACE(ineighbor_allgatherv)
#define MPIR_ALGO_ineighbor_alltoall    MPIR_ALGO_NAMESPACE(ineighbor_alltoall)
#define MPIR_ALGO_ineighbor_alltoallv   MPIR_ALGO_NAMESPACE(ineighbor_alltoallv)
#define MPIR_ALGO_ineighbor_alltoallw   MPIR_ALGO_NAMESPACE(ineighbor_alltoallw)
