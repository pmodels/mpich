/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* Refer to ../mpich/ap_def.h for documentation */
#define TRANSPORT_NAME STUB_
#define TRANSPORT_NAME_LC stub

/* Interface types declaration: */
#define MPIR_TSP_sched_t             MPIR_COLL_STUB_sched_t
#define MPIR_TSP_aint_t              MPIR_COLL_STUB_aint_t
#define MPIR_TSP_comm_t              MPIR_COLL_STUB_comm_t
#define MPIR_TSP_global_t            MPIR_COLL_STUB_global_t
#define MPIR_TSP_dt_t                MPIR_COLL_STUB_dt_t
#define MPIR_TSP_op_t                MPIR_COLL_STUB_op_t

#define MPIR_TSP_global              ((MPIR_COLL_global_instance.tsp_stub))

#define MPIR_TSP_init                MPIR_COLL_STUB_init
#define MPIR_TSP_comm_cleanup        MPIR_COLL_STUB_comm_cleanup
#define MPIR_TSP_comm_init           MPIR_COLL_STUB_comm_init
#define MPIR_TSP_comm_init_null      MPIR_COLL_STUB_comm_init_null
#define MPIR_TSP_fence               MPIR_COLL_STUB_fence
#define MPIR_TSP_wait                MPIR_COLL_STUB_wait
#define MPIR_TSP_wait_for            MPIR_COLL_STUB_wait_for
#define MPIR_TSP_opinfo              MPIR_COLL_STUB_opinfo
#define MPIR_TSP_isinplace           MPIR_COLL_STUB_isinplace
#define MPIR_TSP_dtinfo              MPIR_COLL_STUB_dtinfo
#define MPIR_TSP_addref_dt           MPIR_COLL_STUB_addref_dt
#define MPIR_TSP_addref_dt_nb        MPIR_COLL_STUB_addref_dt_nb
#define MPIR_TSP_addref_op           MPIR_COLL_STUB_addref_op
#define MPIR_TSP_addref_op_nb        MPIR_COLL_STUB_addref_op_nb
#define MPIR_TSP_send                MPIR_COLL_STUB_send
#define MPIR_TSP_recv                MPIR_COLL_STUB_recv
#define MPIR_TSP_multicast           MPIR_COLL_STUB_multicast
#define MPIR_TSP_recv_reduce         MPIR_COLL_STUB_recv_reduce
#define MPIR_TSP_send_accumulate     MPIR_COLL_STUB_send_accumulate
#define MPIR_TSP_test                MPIR_COLL_STUB_test
#define MPIR_TSP_rank                MPIR_COLL_STUB_rank
#define MPIR_TSP_size                MPIR_COLL_STUB_size
#define MPIR_TSP_reduce_local        MPIR_COLL_STUB_reduce_local
#define MPIR_TSP_dtcopy              MPIR_COLL_STUB_dtcopy
#define MPIR_TSP_dtcopy_nb           MPIR_COLL_STUB_dtcopy_nb

#define MPIR_TSP_allocate_mem        MPIR_COLL_STUB_allocate_mem
#define MPIR_TSP_allocate_buffer     MPIR_COLL_STUB_allocate_buffer
#define MPIR_TSP_free_mem            MPIR_COLL_STUB_free_mem
#define MPIR_TSP_free_buffers        MPIR_COLL_STUB_free_buffers
#define MPIR_TSP_free_mem_nb         MPIR_COLL_STUB_free_mem_nb

#define MPIR_TSP_FLAG_REDUCE_L       MPIR_COLL_FLAG_REDUCE_L
#define MPIR_TSP_FLAG_REDUCE_R       MPIR_COLL_FLAG_REDUCE_R

/* Cache API */
#define MPIR_TSP_get_schedule        MPIR_COLL_STUB_get_schedule
#define MPIR_TSP_save_schedule       MPIR_COLL_STUB_save_schedule
#define MPIR_TSP_sched_commit        MPIR_COLL_STUB_sched_commit
#define MPIR_TSP_sched_finalize      MPIR_COLL_STUB_sched_finalize
