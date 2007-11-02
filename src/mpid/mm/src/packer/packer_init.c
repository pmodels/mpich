/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidimpl.h"

MPIDI_VC_functions g_packer_vc_functions = 
{
    packer_post_read,
    NULL, /*packer_car_head_enqueue_read,*/
    packer_merge_with_unexpected,
    NULL, /*packer_merge_with_posted,*/
    NULL, /*packer_merge_unexpected_data,*/
    packer_post_write,
    NULL, /*packer_car_head_enqueue_write,*/
    packer_reset_car,
    NULL, /*packer_setup_packet_car,*/
    NULL /*packer_post_read_pkt*/
};

/*@
   packer_init - initialize the packer method

   Notes:
@*/
int packer_init(void)
{
    MPIDI_STATE_DECL(MPID_STATE_PACKER_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_PACKER_INIT);

    MPID_Process.packer_vc_ptr = mm_vc_alloc(MM_PACKER_METHOD);

    MPIDI_FUNC_EXIT(MPID_STATE_PACKER_INIT);
    return MPI_SUCCESS;
}

/*@
   packer_finalize - finalize the packer method

   Notes:
@*/
int packer_finalize(void)
{
    MPIDI_STATE_DECL(MPID_STATE_PACKER_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_PACKER_FINALIZE);

    if (MPID_Process.packer_vc_ptr != NULL)
    {
	mm_vc_free(MPID_Process.packer_vc_ptr);
	MPID_Process.packer_vc_ptr = NULL;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_PACKER_FINALIZE);
    return MPI_SUCCESS;
}
