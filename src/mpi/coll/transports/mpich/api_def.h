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
#define TSP_FLAG_REDUCE_L    MPIC_FLAG_REDUCE_L
#define TSP_FLAG_REDUCE_R    MPIC_FLAG_REDUCE_R

/* Transport data structures */
#define TSP_sched_t     MPIC_MPICH_sched_t
#define TSP_aint_t      MPIC_MPICH_aint_t
#define TSP_comm_t      MPIC_MPICH_comm_t
#define TSP_global_t    MPIC_MPICH_global_t
#define TSP_dt_t        MPIC_MPICH_dt_t
#define TSP_op_t        MPIC_MPICH_op_t
#define TSP_TASK_STATE  MPIC_MPICH_TASK_STATE
#define TSP_TASK_KIND   MPIC_MPICH_TASK_KIND

/* Transport specific global data structure */
#define TSP_global      ((MPIC_global_instance.tsp_mpich))

/* General transport API */
#define TSP_init             MPIC_MPICH_init
#define TSP_comm_cleanup     MPIC_MPICH_comm_cleanup
#define TSP_comm_init        MPIC_MPICH_comm_init
#define TSP_init_control_dt  MPIC_MPICH_init_control_dt
#define TSP_fence            MPIC_MPICH_fence
#define TSP_wait             MPIC_MPICH_wait
#define TSP_wait_for         MPIC_MPICH_wait_for
#define TSP_opinfo           MPIC_MPICH_opinfo
#define TSP_isinplace        MPIC_MPICH_isinplace
#define TSP_dtinfo           MPIC_MPICH_dtinfo
#define TSP_addref_dt        MPIC_MPICH_addref_dt
#define TSP_addref_dt_nb     MPIC_MPICH_addref_dt_nb
#define TSP_addref_op        MPIC_MPICH_addref_op
#define TSP_addref_op_nb     MPIC_MPICH_addref_op_nb
#define TSP_send             MPIC_MPICH_send
#define TSP_recv             MPIC_MPICH_recv
#define TSP_recv_reduce      MPIC_MPICH_recv_reduce
#define TSP_send_accumulate  MPIC_MPICH_send_accumulate
#define TSP_test             MPIC_MPICH_test
#define TSP_rank             MPIC_MPICH_rank
#define TSP_size             MPIC_MPICH_size
#define TSP_reduce_local     MPIC_MPICH_reduce_local
#define TSP_dtcopy           MPIC_MPICH_dtcopy
#define TSP_dtcopy_nb        MPIC_MPICH_dtcopy_nb
#define TSP_allocate_mem     MPIC_MPICH_allocate_mem
#define TSP_allocate_buffer  MPIC_MPICH_allocate_buffer
#define TSP_free_mem         MPIC_MPICH_free_mem
#define TSP_free_mem_nb      MPIC_MPICH_free_mem_nb
#define TSP_free_buffers     MPIC_MPICH_free_buffers

/* Transport API for collective schedules */

/* Get schedule from database. Returns NULL if schedule not present
 * else reset schedule and return the schedule */
#define TSP_get_schedule        MPIC_MPICH_get_schedule
/* Save schedule in the database */
#define TSP_save_schedule       MPIC_MPICH_save_schedule
/* Release schedule after use (release all associated memory if
 * it is not being stored in database for future use */
#define TSP_sched_finalize      MPIC_MPICH_sched_finalize
/* Indicate that schedule generation is now complete */
#define TSP_sched_commit        MPIC_MPICH_sched_commit
