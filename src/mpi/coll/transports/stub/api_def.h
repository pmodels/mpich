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
#define MPIR_TSP_sched_t             MPIC_STUB_sched_t
#define MPIR_TSP_aint_t              MPIC_STUB_aint_t
#define MPIR_TSP_comm_t              MPIC_STUB_comm_t
#define MPIR_TSP_global_t            MPIC_STUB_global_t
#define MPIR_TSP_dt_t                MPIC_STUB_dt_t
#define MPIR_TSP_op_t                MPIC_STUB_op_t

#define MPIR_TSP_global              ((MPIC_global_instance.tsp_stub))

#define MPIR_TSP_init                MPIC_STUB_init
#define MPIR_TSP_comm_cleanup        MPIC_STUB_comm_cleanup
#define MPIR_TSP_comm_init           MPIC_STUB_comm_init
#define MPIR_TSP_comm_init_null      MPIC_STUB_comm_init_null
#define MPIR_TSP_fence               MPIC_STUB_fence
#define MPIR_TSP_wait                MPIC_STUB_wait
#define MPIR_TSP_wait_for            MPIC_STUB_wait_for
#define MPIR_TSP_opinfo              MPIC_STUB_opinfo
#define MPIR_TSP_isinplace           MPIC_STUB_isinplace
#define MPIR_TSP_dtinfo              MPIC_STUB_dtinfo
#define MPIR_TSP_addref_dt           MPIC_STUB_addref_dt
#define MPIR_TSP_addref_dt_nb        MPIC_STUB_addref_dt_nb
#define MPIR_TSP_addref_op           MPIC_STUB_addref_op
#define MPIR_TSP_addref_op_nb        MPIC_STUB_addref_op_nb
#define MPIR_TSP_send                MPIC_STUB_send
#define MPIR_TSP_recv                MPIC_STUB_recv
#define MPIR_TSP_multicast           MPIC_STUB_multicast
#define MPIR_TSP_recv_reduce         MPIC_STUB_recv_reduce
#define MPIR_TSP_send_accumulate     MPIC_STUB_send_accumulate
#define MPIR_TSP_test                MPIC_STUB_test
#define MPIR_TSP_rank                MPIC_STUB_rank
#define MPIR_TSP_size                MPIC_STUB_size
#define MPIR_TSP_reduce_local        MPIC_STUB_reduce_local
#define MPIR_TSP_dtcopy              MPIC_STUB_dtcopy
#define MPIR_TSP_dtcopy_nb           MPIC_STUB_dtcopy_nb

#define MPIR_TSP_allocate_mem        MPIC_STUB_allocate_mem
#define MPIR_TSP_allocate_buffer     MPIC_STUB_allocate_buffer
#define MPIR_TSP_free_mem            MPIC_STUB_free_mem
#define MPIR_TSP_free_buffers        MPIC_STUB_free_buffers
#define MPIR_TSP_free_mem_nb         MPIC_STUB_free_mem_nb

#define MPIR_TSP_FLAG_REDUCE_L       MPIC_FLAG_REDUCE_L
#define MPIR_TSP_FLAG_REDUCE_R       MPIC_FLAG_REDUCE_R

/* Cache API */
#define MPIR_TSP_get_schedule        MPIC_STUB_get_schedule
#define MPIR_TSP_save_schedule       MPIC_STUB_save_schedule
#define MPIR_TSP_sched_commit        MPIC_STUB_sched_commit
#define MPIR_TSP_sched_finalize      MPIC_STUB_sched_finalize
