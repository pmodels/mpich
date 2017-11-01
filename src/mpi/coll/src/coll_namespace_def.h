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

#ifndef COLL_CAT_GUARD
#define COLL_CAT_GUARD

#define COLL_NSCAT0(a,b) a##b
#define COLL_NSCAT1(a,b) COLL_NSCAT0(a,b)

#endif /* COLL_CAT_GUARD */

#ifndef COLL_NAMESPACE
#define COLL_NAMESPACE(fn)  COLL_NSCAT1(\
                                COLL_NSCAT1(\
                                    COLL_NSCAT1(\
                                        GLOBAL_NAME,\
                                        TRANSPORT_NAME),\
                                    COLL_NAME),\
                                fn)
#endif /* COLL_NAMESPACE */

/* Common Collective Types */
#define COLL_dt_t         TSP_dt_t
#define COLL_op_t         TSP_op_t

#define COLL_comm_t       COLL_NAMESPACE(comm_t)
#define COLL_req_t        COLL_NAMESPACE(req_t)
#define COLL_sched_t      COLL_NAMESPACE(sched_t)
#define COLL_aint_t       COLL_NAMESPACE(aint_t)
#define COLL_global_t     COLL_NAMESPACE(global_t)
#define COLL_args_t       COLL_NAMESPACE(args_t)

/* Tree collective types */
#define COLL_fence_t        COLL_NAMESPACE(fence_t)
#define COLL_tree_t         COLL_NAMESPACE(tree_t)
#define COLL_tree_comm_t    COLL_NAMESPACE(tree_comm_t)
#define COLL_child_range_t  COLL_NAMESPACE(child_range_t)
#define COLL_tree_add_child COLL_NAMESPACE(tree_add_child)

/* Tree schedule utility API */
#define COLL_tree_init         COLL_NAMESPACE(tree_init)
#define COLL_tree_free         COLL_NAMESPACE(tree_free)
#define COLL_tree_dump         COLL_NAMESPACE(tree_dump)
#define COLL_tree_kary         COLL_NAMESPACE(tree_kary)
#define COLL_tree_knomial      COLL_NAMESPACE(tree_knomial)
#define COLL_tree_kary_init    COLL_NAMESPACE(tree_kary_init)
#define COLL_tree_knomial_1_init COLL_NAMESPACE(tree_knomial_1_init)
#define COLL_tree_knomial_2_init COLL_NAMESPACE(tree_knomial_2_init)
#define COLL_ilog              COLL_NAMESPACE(ilog)
#define COLL_ipow              COLL_NAMESPACE(ipow)
#define COLL_getdigit          COLL_NAMESPACE(getdigit)
#define COLL_setdigit          COLL_NAMESPACE(setdigit)

/* Generic collective algorithms API */
#define COLL_init                  COLL_NAMESPACE(init)
#define COLL_comm_init             COLL_NAMESPACE(comm_init)
#define COLL_comm_cleanup          COLL_NAMESPACE(comm_cleanup)

#define COLL_FIELD_NAME         COLL_NSCAT1(COLL_NSCAT1(TRANSPORT_NAME_LC,_),COLL_NAME_LC)
#define COLL_COMM_BASE(addr)    container_of(addr,MPIC_comm_t, COLL_FIELD_NAME)
#define COLL_global             MPIC_global_instance.COLL_FIELD_NAME

/* Scheduling API */
#define COLL_sched_init                 COLL_NAMESPACE(sched_init)
#define COLL_sched_reset                COLL_NAMESPACE(sched_reset)
#define COLL_sched_free                 COLL_NAMESPACE(sched_free)
#define COLL_sched_init_nb              COLL_NAMESPACE(sched_init_nb)
#define COLL_sched_wait                 COLL_NAMESPACE(sched_wait)
#define COLL_sched_test                 COLL_NAMESPACE(sched_test)
#define COLL_sched_barrier_dissem       COLL_NAMESPACE(sched_barrier_dissem)
#define COLL_sched_allreduce_dissem     COLL_NAMESPACE(sched_allreduce_dissem)
#define COLL_sched_allreduce            COLL_NAMESPACE(sched_allreduce)
#define COLL_sched_allgather            COLL_NAMESPACE(sched_allgather)
#define COLL_sched_allreduce_tree       COLL_NAMESPACE(sched_allreduce_tree)
#define COLL_sched_bcast                COLL_NAMESPACE(sched_bcast)
#define COLL_sched_bcast_tree           COLL_NAMESPACE(sched_bcast_tree)
#define COLL_sched_bcast_tree_pipelined COLL_NAMESPACE(sched_bcast_tree_pipelined)
#define COLL_bcast_get_tree_schedule    COLL_NAMESPACE(bcast_get_tree_schedule)
#define COLL_sched_reduce_full          COLL_NAMESPACE(sched_reduce_full)
#define COLL_sched_reduce_tree_full     COLL_NAMESPACE(sched_reduce_tree_full)
#define COLL_sched_reduce_tree_full_pipelined   COLL_NAMESPACE(sched_reduce_tree_full_pipelined)
#define COLL_sched_barrier              COLL_NAMESPACE(sched_barrier)
#define COLL_sched_barrier_tree         COLL_NAMESPACE(sched_barrier_tree)
#define COLL_sched_allreduce_recexch    COLL_NAMESPACE(sched_allreduce_recexch)
#define COLL_sched_allgather_recexch    COLL_NAMESPACE(sched_allgather_recexch)
#define COLL_sched_reduce_full          COLL_NAMESPACE(sched_reduce_full)
#define COLL_sched_reduce               COLL_NAMESPACE(sched_reduce)
#define COLL_sched_reduce_tree          COLL_NAMESPACE(sched_reduce_tree)
#define COLL_sched_barrier              COLL_NAMESPACE(sched_barrier)
#define COLL_get_neighbors_recexch      COLL_NAMESPACE(get_neighbors_recexch)
#define COLL_sched_allgather_recexch_data_exchange    COLL_NAMESPACE(sched_allgather_recexch_data_exchange)
#define COLL_sched_allgather_recexch_step1  COLL_NAMESPACE(sched_allgather_recexch_step1)
#define COLL_sched_allgather_recexch_step2  COLL_NAMESPACE(sched_allgather_recexch_step2)
#define COLL_sched_allgather_recexch_step3  COLL_NAMESPACE(sched_allgather_recexch_step3)
#define COLL_get_neighbors_recexch      COLL_NAMESPACE(get_neighbors_recexch)
#define COLL_sched_alltoall_scattered   COLL_NAMESPACE(sched_alltoall_scattered)
#define COLL_sched_alltoall_pairwise    COLL_NAMESPACE(sched_alltoall_pairwise)
#define COLL_sched_alltoall_ring        COLL_NAMESPACE(sched_alltoall_ring)
#define COLL_sched_alltoall_brucks      COLL_NAMESPACE(sched_alltoall_brucks)
#define COLL_sched_alltoallv_scattered  COLL_NAMESPACE(sched_alltoallv_scattered)
#define COLL_sched_allgather_ring       COLL_NAMESPACE(sched_allgather_ring)
#define COLL_sched_alltoall             COLL_NAMESPACE(sched_alltoall)
#define COLL_brucks_pup                 COLL_NAMESPACE(brucks_pup)
#define COLL_get_key_size               COLL_NAMESPACE(get_key_size)

/* Collective API */
#define COLL_kick_nb               COLL_NAMESPACE(kick_nb)
#define COLL_allgather             COLL_NAMESPACE(allgather)
#define COLL_allgatherv            COLL_NAMESPACE(allgatherv)
#define COLL_allreduce             COLL_NAMESPACE(allreduce)
#define COLL_alltoall              COLL_NAMESPACE(alltoall)
#define COLL_alltoallv             COLL_NAMESPACE(alltoallv)
#define COLL_alltoallw             COLL_NAMESPACE(alltoallw)
#define COLL_bcast                 COLL_NAMESPACE(bcast)
#define COLL_exscan                COLL_NAMESPACE(exscan)
#define COLL_gather                COLL_NAMESPACE(gather)
#define COLL_gatherv               COLL_NAMESPACE(gatherv)
#define COLL_reduce_scatter        COLL_NAMESPACE(reduce_scatter)
#define COLL_reduce_scatter_block  COLL_NAMESPACE(reduce_scatter_block)
#define COLL_reduce                COLL_NAMESPACE(reduce)
#define COLL_scan                  COLL_NAMESPACE(scan)
#define COLL_scatter               COLL_NAMESPACE(scatter)
#define COLL_scatterv              COLL_NAMESPACE(scatterv)
#define COLL_barrier               COLL_NAMESPACE(barrier)
#define COLL_iallgather            COLL_NAMESPACE(iallgather)
#define COLL_iallgatherv           COLL_NAMESPACE(iallgatherv)
#define COLL_iallreduce            COLL_NAMESPACE(iallreduce)
#define COLL_ialltoall             COLL_NAMESPACE(ialltoall)
#define COLL_ialltoallv            COLL_NAMESPACE(ialltoallv)
#define COLL_ialltoallw            COLL_NAMESPACE(ialltoallw)
#define COLL_ibcast                COLL_NAMESPACE(ibcast)
#define COLL_iexscan               COLL_NAMESPACE(iexscan)
#define COLL_igather               COLL_NAMESPACE(igather)
#define COLL_igatherv              COLL_NAMESPACE(igatherv)
#define COLL_ireduce_scatter       COLL_NAMESPACE(ireduce_scatter)
#define COLL_ireduce_scatter_block COLL_NAMESPACE(ireduce_scatter_block)
#define COLL_ireduce               COLL_NAMESPACE(ireduce)
#define COLL_iscan                 COLL_NAMESPACE(iscan)
#define COLL_iscatter              COLL_NAMESPACE(iscatter)
#define COLL_iscatterv             COLL_NAMESPACE(iscatterv)
#define COLL_ibarrier              COLL_NAMESPACE(ibarrier)
#define COLL_neighbor_allgather    COLL_NAMESPACE(neighbor_allgather)
#define COLL_neighbor_allgatherv   COLL_NAMESPACE(neighbor_allgatherv)
#define COLL_neighbor_alltoall     COLL_NAMESPACE(neighbor_alltoall)
#define COLL_neighbor_alltoallv    COLL_NAMESPACE(neighbor_alltoallv)
#define COLL_neighbor_alltoallw    COLL_NAMESPACE(neighbor_alltoallw)
#define COLL_ineighbor_allgather   COLL_NAMESPACE(ineighbor_allgather)
#define COLL_ineighbor_allgatherv  COLL_NAMESPACE(ineighbor_allgatherv)
#define COLL_ineighbor_alltoall    COLL_NAMESPACE(ineighbor_alltoall)
#define COLL_ineighbor_alltoallv   COLL_NAMESPACE(ineighbor_alltoallv)
#define COLL_ineighbor_alltoallw   COLL_NAMESPACE(ineighbor_alltoallw)
