/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_PACKER_H
#define MM_PACKER_H

#include "mpidimpl.h"

extern MPIDI_VC_functions g_packer_vc_functions;

int packer_init(void);
int packer_finalize(void);
int packer_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int packer_merge_with_unexpected(MM_Car *car_ptr, MM_Car *unex_car_ptr);
int packer_post_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int packer_make_progress(void);
int packer_car_enqueue(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int packer_car_dequeue(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int packer_reset_car(MM_Car *car_ptr);

#endif
