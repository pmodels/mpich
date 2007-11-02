/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_SOCKET_H
#define MM_SOCKET_H

#include "mpidimpl.h"

extern MPIDI_VC_functions g_socket_vc_functions;

int socket_init(void);
int socket_finalize(void);
int socket_get_business_card(char *value, int length);
int socket_can_connect(char *business_card);
int socket_post_connect(MPIDI_VC *vc_ptr, char *business_card);
int socket_handle_connect(MPIDI_VC *vc_ptr);
int socket_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int socket_merge_with_unexpected(MM_Car *car_ptr, MM_Car *unex_car_ptr);
int socket_merge_with_posted(MM_Car *posted_car_ptr, MM_Car *pkt_car_ptr);
int socket_merge_unexpected_data(MPIDI_VC *vc_ptr, MM_Car *car_ptr, char *buffer, int length);
int socket_post_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int socket_make_progress(void);
int socket_car_enqueue_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int socket_car_enqueue_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int socket_car_head_enqueue_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int socket_car_head_enqueue_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int socket_car_dequeue_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int socket_car_dequeue_write(MPIDI_VC *vc_ptr);
int socket_reset_car(MM_Car *car_ptr);
int socket_post_read_pkt(MPIDI_VC *vc_ptr);
int socket_setup_packet_car(MPIDI_VC *vc_ptr, MM_CAR_TYPE read_write, int src_dest, MM_Car *car_ptr);
void socket_format_sock_error(int error);
void socket_print_sock_error(int error, char *msg);

#endif
