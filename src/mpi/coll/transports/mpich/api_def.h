/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* Name of the transport, '_' is suffixed for function name generation */
#define TRANSPORT_NAME MPICH_

/* Transport name in lower case (LC) */
#define TRANSPORT_NAME_LC mpich

/* Some flags */
#define MPIR_TSP_FLAG_REDUCE_L    MPIC_FLAG_REDUCE_L
#define MPIR_TSP_FLAG_REDUCE_R    MPIC_FLAG_REDUCE_R

/* Transport data structures */
#define MPIR_TSP_sched_t     MPIC_MPICH_sched_t
#define MPIR_TSP_aint_t      MPIC_MPICH_aint_t
#define MPIR_TSP_comm_t      MPIC_MPICH_comm_t
#define MPIR_TSP_global_t    MPIC_MPICH_global_t
#define MPIR_TSP_dt_t        MPIC_MPICH_dt_t
#define MPIR_TSP_op_t        MPIC_MPICH_op_t
#define MPIR_TSP_TASK_STATE  MPIC_MPICH_TASK_STATE
#define MPIR_TSP_TASK_KIND   MPIC_MPICH_TASK_KIND

/* Transport specific global data structure */
#define MPIR_TSP_global      ((MPIC_global_instance.tsp_mpich))

/* General transport API */
#define MPIR_TSP_init             MPIC_MPICH_init
#define MPIR_TSP_comm_cleanup     MPIC_MPICH_comm_cleanup
#define MPIR_TSP_comm_init        MPIC_MPICH_comm_init
#define MPIR_TSP_comm_init_null   MPIC_MPICH_comm_init_null
#define MPIR_TSP_init_control_dt  MPIC_MPICH_init_control_dt
#define MPIR_TSP_fence            MPIC_MPICH_fence
#define MPIR_TSP_wait             MPIC_MPICH_wait
#define MPIR_TSP_wait_for         MPIC_MPICH_wait_for
#define MPIR_TSP_opinfo           MPIC_MPICH_opinfo
#define MPIR_TSP_isinplace        MPIC_MPICH_isinplace
#define MPIR_TSP_dtinfo           MPIC_MPICH_dtinfo
#define MPIR_TSP_addref_dt        MPIC_MPICH_addref_dt
#define MPIR_TSP_addref_dt_nb     MPIC_MPICH_addref_dt_nb
#define MPIR_TSP_addref_op        MPIC_MPICH_addref_op
#define MPIR_TSP_addref_op_nb     MPIC_MPICH_addref_op_nb
#define MPIR_TSP_send             MPIC_MPICH_send
#define MPIR_TSP_recv             MPIC_MPICH_recv
#define MPIR_TSP_multicast        MPIC_MPICH_multicast
#define MPIR_TSP_recv_reduce      MPIC_MPICH_recv_reduce
#define MPIR_TSP_send_accumulate  MPIC_MPICH_send_accumulate
#define MPIR_TSP_test             MPIC_MPICH_test
#define MPIR_TSP_rank             MPIC_MPICH_rank
#define MPIR_TSP_size             MPIC_MPICH_size
#define MPIR_TSP_reduce_local     MPIC_MPICH_reduce_local
#define MPIR_TSP_dtcopy           MPIC_MPICH_dtcopy
#define MPIR_TSP_dtcopy_nb        MPIC_MPICH_dtcopy_nb
#define MPIR_TSP_allocate_mem     MPIC_MPICH_allocate_mem
#define MPIR_TSP_allocate_buffer  MPIC_MPICH_allocate_buffer
#define MPIR_TSP_free_mem         MPIC_MPICH_free_mem
#define MPIR_TSP_free_mem_nb      MPIC_MPICH_free_mem_nb
#define MPIR_TSP_free_buffers     MPIC_MPICH_free_buffers

/* Transport API for collective schedules */

/* Get schedule from database. Returns NULL if schedule not present
 * else reset schedule and return the schedule */
#define MPIR_TSP_get_schedule        MPIC_MPICH_get_schedule
/* Save schedule in the database */
#define MPIR_TSP_save_schedule       MPIC_MPICH_save_schedule
/* Release schedule after use (release all associated memory if
 * it is not being stored in database for future use */
#define MPIR_TSP_sched_finalize      MPIC_MPICH_sched_finalize
/* Indicate that schedule generation is now complete */
#define MPIR_TSP_sched_commit        MPIC_MPICH_sched_commit
