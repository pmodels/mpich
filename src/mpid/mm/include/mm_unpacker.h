/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_UNPACKER_H
#define MM_UNPACKER_H

#include "mpidimpl.h"

extern MPIDI_VC_functions g_unpacker_vc_functions;

int unpacker_init(void);
int unpacker_finalize(void);
int unpacker_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int unpacker_merge_with_unexpected(MM_Car *car_ptr, MM_Car *unex_car_ptr);
int unpacker_post_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int unpacker_make_progress(void);
int unpacker_car_enqueue(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int unpacker_car_dequeue(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int unpacker_reset_car(MM_Car *car_ptr);

#endif
