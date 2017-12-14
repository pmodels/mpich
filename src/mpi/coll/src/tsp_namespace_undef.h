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
#undef MPIR_TSP_dt_t
#undef MPIR_TSP_op_t
#undef MPIR_TSP_comm_t
#undef MPIR_TSP_sched_t
#undef MPIR_TSP_aint_t
#undef MPIR_TSP_global_t
#undef MPIR_TSP_global
#undef MPIR_TSP_TASK_KIND
#undef MPIR_TSP_TASK_STATE

#undef MPIR_TSP_FLAG_REDUCE_L
#undef MPIR_TSP_FLAG_REDUCE_R

/* Transport APIS */
#undef MPIR_TSP_init
#undef MPIR_TSP_comm_init
#undef MPIR_TSP_comm_init_null
#undef MPIR_TSP_comm_cleanup
#undef MPIR_TSP_dt_init
#undef MPIR_TSP_op_init
#undef MPIR_TSP_sched_commit
#undef MPIR_TSP_sched_finalize
#undef MPIR_TSP_get_schedule
#undef MPIR_TSP_save_schedule
#undef MPIR_TSP_fence
#undef MPIR_TSP_wait
#undef MPIR_TSP_wait_for
#undef MPIR_TSP_opinfo
#undef MPIR_TSP_isinplace
#undef MPIR_TSP_dtinfo
#undef MPIR_TSP_addref_dt
#undef MPIR_TSP_addref_dt_nb
#undef MPIR_TSP_addref_op
#undef MPIR_TSP_addref_op_nb
#undef MPIR_TSP_send
#undef MPIR_TSP_recv
#undef MPIR_TSP_multicast
#undef MPIR_TSP_recv_reduce
#undef MPIR_TSP_send_accumulate
#undef MPIR_TSP_test
#undef MPIR_TSP_rank
#undef MPIR_TSP_size
#undef MPIR_TSP_reduce_local
#undef MPIR_TSP_dtcopy
#undef MPIR_TSP_dtcopy_nb
#undef MPIR_TSP_allocate_mem
#undef MPIR_TSP_free_mem
#undef MPIR_TSP_free_mem_nb
#undef MPIR_TSP_allocate_buffer
#undef MPIR_TSP_free_buffers
