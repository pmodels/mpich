/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_TCP_H
#define MM_TCP_H

#include "mpidimpl.h"
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

extern MPIDI_VC_functions g_tcp_vc_functions;

int tcp_init(void);
int tcp_finalize(void);
int tcp_get_business_card(char *value, int length);
int tcp_can_connect(char *business_card);
int tcp_post_connect(MPIDI_VC *vc_ptr, char *business_card);
int tcp_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int tcp_merge_with_unexpected(MM_Car *car_ptr, MM_Car *unex_car_ptr);
int tcp_merge_with_posted(MM_Car *posted_car_ptr, MM_Car *pkt_car_ptr);
int tcp_merge_unexpected_data(MPIDI_VC *vc_ptr, MM_Car *car_ptr, char *buffer, int length);
int tcp_post_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int tcp_make_progress(void);
int tcp_car_enqueue(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int tcp_car_head_enqueue(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int tcp_car_dequeue(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int tcp_car_dequeue_write(MPIDI_VC *vc_ptr);
int tcp_reset_car(MM_Car *car_ptr);
int tcp_post_read_pkt(MPIDI_VC *vc_ptr);
int tcp_setup_packet_car(MPIDI_VC *vc_ptr, MM_CAR_TYPE read_write, int src_dest, MM_Car *car_ptr);
int tcp_read_connecting(MPIDI_VC *vc_ptr);

#endif
