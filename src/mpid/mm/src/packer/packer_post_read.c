/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidimpl.h"

int packer_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_PACKER_POST_READ);
    MPIDI_FUNC_ENTER(MPID_STATE_PACKER_POST_READ);

    packer_car_enqueue(vc_ptr, car_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_PACKER_POST_READ);
    return MPI_SUCCESS;
}

int packer_merge_with_unexpected(MM_Car *car_ptr, MM_Car *unex_car_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_PACKER_MERGE_WITH_UNEXPECTED);
    MPIDI_FUNC_ENTER(MPID_STATE_PACKER_MERGE_WITH_UNEXPECTED);

    err_printf("packer_merge_with_unexpected: Congratulations,\nyou win the opportuninty to report a bug.  I thought this function would never be called.\n");

    MPIDI_FUNC_EXIT(MPID_STATE_PACKER_MERGE_WITH_UNEXPECTED);
    return MPI_SUCCESS;
}
