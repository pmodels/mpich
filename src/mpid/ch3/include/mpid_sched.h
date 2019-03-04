/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_SCHED_H_INCLUDED
#define MPID_SCHED_H_INCLUDED
#include "mpidu_sched.h"

#define MPIR_Sched_element_cb MPIDU_Sched_element_cb
#define MPIR_Sched_element_cb2 MPIDU_Sched_element_cb2
#define MPIR_Sched_list_get_next_tag  MPIDU_Sched_list_get_next_tag
#define MPIR_Sched_element_create MPIDU_Sched_element_create
#define MPIR_Sched_element_clone MPIDU_Sched_element_clone
#define MPIR_Sched_list_enqueue_sched MPIDU_Sched_list_enqueue_sched
#define MPIR_Sched_element_send MPIDU_Sched_element_send
#define MPIR_Sched_element_send_defer MPIDU_Sched_element_send_defer
#define MPIR_Sched_element_recv MPIDU_Sched_element_recv
#define MPIR_Sched_element_recv_status MPIDU_Sched_element_recv_status
#define MPIR_Sched_element_ssend MPIDU_Sched_element_ssend
#define MPIR_Sched_element_reduce MPIDU_Sched_element_reduce
#define MPIR_Sched_element_copy MPIDU_Sched_element_copy
#define MPIR_Sched_element_barrier MPIDU_Sched_element_barrier

#endif /* MPID_SCHED_H_INCLUDED */
