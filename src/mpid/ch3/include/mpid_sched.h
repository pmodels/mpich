/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPID_SCHED_H_INCLUDED
#define MPID_SCHED_H_INCLUDED
#include "mpidu_sched.h"

#define MPIR_Sched_cb MPIDU_Sched_cb
#define MPIR_Sched_cb2 MPIDU_Sched_cb2
#define MPIR_Sched_next_tag  MPIDU_Sched_next_tag
#define MPIR_Sched_set_tag  MPIDU_Sched_set_tag
#define MPIR_Sched_create MPIDU_Sched_create
#define MPIR_Sched_clone MPIDU_Sched_clone
#define MPIR_Sched_free MPIDU_Sched_free
#define MPIR_Sched_reset MPIDU_Sched_reset
#define MPIR_Sched_alloc_state MPIDU_Sched_alloc_state
#define MPIR_Sched_start MPIDU_Sched_start
#define MPIR_Sched_send MPIDU_Sched_send
#define MPIR_Sched_pt2pt_send MPIDU_Sched_pt2pt_send
#define MPIR_Sched_send_defer MPIDU_Sched_send_defer
#define MPIR_Sched_recv MPIDU_Sched_recv
#define MPIR_Sched_pt2pt_recv MPIDU_Sched_pt2pt_recv
#define MPIR_Sched_recv_status MPIDU_Sched_recv_status
#define MPIR_Sched_ssend MPIDU_Sched_ssend
#define MPIR_Sched_reduce MPIDU_Sched_reduce
#define MPIR_Sched_copy MPIDU_Sched_copy
#define MPIR_Sched_barrier MPIDU_Sched_barrier

#endif /* MPID_SCHED_H_INCLUDED */
