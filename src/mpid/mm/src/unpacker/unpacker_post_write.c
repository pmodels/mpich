/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidimpl.h"

int unpacker_post_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_UNPACKER_POST_WRITE);
    MPIDI_FUNC_ENTER(MPID_STATE_UNPACKER_POST_WRITE);

    unpacker_car_enqueue(vc_ptr, car_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_UNPACKER_POST_WRITE);
    return MPI_SUCCESS;
}

