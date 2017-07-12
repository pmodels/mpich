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

#undef TRANSPORT_NAME
#undef TRANSPORT_NAME_LC

/* Transport Types */
#undef TSP_dt_t
#undef TSP_op_t
#undef TSP_comm_t
#undef TSP_sched_t
#undef TSP_aint_t
#undef TSP_global_t
#undef TSP_global
#undef TSP_TASK_KIND
#undef TSP_TASK_STATE

#undef TSP_FLAG_REDUCE_L
#undef TSP_FLAG_REDUCE_R

/* Transport APIS */
#undef TSP_init
#undef TSP_comm_init
#undef TSP_comm_cleanup
#undef TSP_dt_init
#undef TSP_op_init
#undef TSP_sched_commit
#undef TSP_sched_finalize
#undef TSP_get_schedule
#undef TSP_save_schedule
#undef TSP_fence
#undef TSP_wait
#undef TSP_wait_for
#undef TSP_opinfo
#undef TSP_isinplace
#undef TSP_dtinfo
#undef TSP_addref_dt
#undef TSP_addref_dt_nb
#undef TSP_addref_op
#undef TSP_addref_op_nb
#undef TSP_send
#undef TSP_recv
#undef TSP_multicast
#undef TSP_recv_reduce
#undef TSP_send_accumulate
#undef TSP_test
#undef TSP_rank
#undef TSP_size
#undef TSP_reduce_local
#undef TSP_dtcopy
#undef TSP_dtcopy_nb
#undef TSP_allocate_mem
#undef TSP_free_mem
#undef TSP_free_mem_nb
#undef TSP_allocate_buffer
#undef TSP_free_buffers
