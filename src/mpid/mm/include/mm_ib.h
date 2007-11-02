/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_IB_H
#define MM_IB_H

#include "mpidimpl.h"

extern MPIDI_VC_functions g_ib_vc_functions;

int ib_init(void);
int ib_finalize(void);
int ib_get_business_card(char *value, int length);
int ib_can_connect(char *business_card);
int ib_post_connect(MPIDI_VC *vc_ptr, char *business_card);
int ib_handle_connect(MPIDI_VC *vc_ptr);
int ib_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int ib_merge_with_unexpected(MM_Car *car_ptr, MM_Car *unex_car_ptr);
int ib_merge_with_posted(MM_Car *posted_car_ptr, MM_Car *pkt_car_ptr);
int ib_merge_unexpected_data(MPIDI_VC *vc_ptr, MM_Car *car_ptr, char *buffer, int length);
int ib_post_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int ib_make_progress(void);
int ib_enqueue_read_at_head(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int ib_enqueue_write_at_head(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int ib_car_enqueue_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int ib_car_enqueue_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int ib_car_head_enqueue(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int ib_car_dequeue_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int ib_car_dequeue_write(MPIDI_VC *vc_ptr);
 int ib_reset_car(MM_Car *car_ptr);
 int ib_post_read_pkt(MPIDI_VC *vc_ptr);
 int ib_setup_packet_car(MPIDI_VC *vc_ptr, MM_CAR_TYPE read_write, int src_dest, MM_Car *car_ptr);
/*
void ib_format_error(int error);
void ib_print_error(int error, char *msg);
*/

#endif
