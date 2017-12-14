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
#define MPIR_TSP_FLAG_REDUCE_L    MPIR_COLL_FLAG_REDUCE_L
#define MPIR_TSP_FLAG_REDUCE_R    MPIR_COLL_FLAG_REDUCE_R

/* Transport data structures */
#define MPIR_TSP_sched_t     MPIR_COLL_MPICH_sched_t
#define MPIR_TSP_aint_t      MPIR_COLL_MPICH_aint_t
#define MPIR_TSP_comm_t      MPIR_COLL_MPICH_comm_t
#define MPIR_TSP_global_t    MPIR_COLL_MPICH_global_t
#define MPIR_TSP_dt_t        MPIR_COLL_MPICH_dt_t
#define MPIR_TSP_op_t        MPIR_COLL_MPICH_op_t
#define MPIR_TSP_TASK_STATE  MPIR_COLL_MPICH_TASK_STATE
#define MPIR_TSP_TASK_KIND   MPIR_COLL_MPICH_TASK_KIND

/* Transport specific global data structure */
#define MPIR_TSP_global      ((MPIR_COLL_global_instance.tsp_mpich))

/* General transport API */
#define MPIR_TSP_init             MPIR_COLL_MPICH_init
#define MPIR_TSP_comm_cleanup     MPIR_COLL_MPICH_comm_cleanup
#define MPIR_TSP_comm_init        MPIR_COLL_MPICH_comm_init
#define MPIR_TSP_comm_init_null   MPIR_COLL_MPICH_comm_init_null
#define MPIR_TSP_init_control_dt  MPIR_COLL_MPICH_init_control_dt
#define MPIR_TSP_fence            MPIR_COLL_MPICH_fence
#define MPIR_TSP_wait             MPIR_COLL_MPICH_wait
#define MPIR_TSP_wait_for         MPIR_COLL_MPICH_wait_for
#define MPIR_TSP_opinfo           MPIR_COLL_MPICH_opinfo
#define MPIR_TSP_isinplace        MPIR_COLL_MPICH_isinplace
#define MPIR_TSP_dtinfo           MPIR_COLL_MPICH_dtinfo
#define MPIR_TSP_addref_dt        MPIR_COLL_MPICH_addref_dt
#define MPIR_TSP_addref_dt_nb     MPIR_COLL_MPICH_addref_dt_nb
#define MPIR_TSP_addref_op        MPIR_COLL_MPICH_addref_op
#define MPIR_TSP_addref_op_nb     MPIR_COLL_MPICH_addref_op_nb
#define MPIR_TSP_send             MPIR_COLL_MPICH_send
#define MPIR_TSP_recv             MPIR_COLL_MPICH_recv
#define MPIR_TSP_multicast        MPIR_COLL_MPICH_multicast
#define MPIR_TSP_recv_reduce      MPIR_COLL_MPICH_recv_reduce
#define MPIR_TSP_send_accumulate  MPIR_COLL_MPICH_send_accumulate
#define MPIR_TSP_test             MPIR_COLL_MPICH_test
#define MPIR_TSP_rank             MPIR_COLL_MPICH_rank
#define MPIR_TSP_size             MPIR_COLL_MPICH_size
#define MPIR_TSP_reduce_local     MPIR_COLL_MPICH_reduce_local
#define MPIR_TSP_dtcopy           MPIR_COLL_MPICH_dtcopy
#define MPIR_TSP_dtcopy_nb        MPIR_COLL_MPICH_dtcopy_nb
#define MPIR_TSP_allocate_mem     MPIR_COLL_MPICH_allocate_mem
#define MPIR_TSP_allocate_buffer  MPIR_COLL_MPICH_allocate_buffer
#define MPIR_TSP_free_mem         MPIR_COLL_MPICH_free_mem
#define MPIR_TSP_free_mem_nb      MPIR_COLL_MPICH_free_mem_nb
#define MPIR_TSP_free_buffers     MPIR_COLL_MPICH_free_buffers

/* Transport API for collective schedules */

/* Get schedule from database. Returns NULL if schedule not present
 * else reset schedule and return the schedule */
#define MPIR_TSP_get_schedule        MPIR_COLL_MPICH_get_schedule
/* Save schedule in the database */
#define MPIR_TSP_save_schedule       MPIR_COLL_MPICH_save_schedule
/* Release schedule after use (release all associated memory if
 * it is not being stored in database for future use */
#define MPIR_TSP_sched_finalize      MPIR_COLL_MPICH_sched_finalize
/* Indicate that schedule generation is now complete */
#define MPIR_TSP_sched_commit        MPIR_COLL_MPICH_sched_commit
