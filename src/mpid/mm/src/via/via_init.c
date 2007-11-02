/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "viaimpl.h"

#ifdef WITH_METHOD_VIA

VIA_PerProcess VIA_Process;
MPIDI_VC_functions g_via_vc_functions = 
{
    NULL, /*via_post_read,*/
    NULL, /*via_car_head_enqueue_read,*/
    via_merge_with_unexpected,
    NULL, /*via_merge_with_posted,*/
    NULL, /*via_merge_unexpected_data,*/
    via_post_write,
    NULL, /*via_car_head_enqueue_write,*/
    NULL, /*via_reset_car,*/
    NULL, /*via_setup_packet_car,*/
    NULL /*via_post_read_pkt*/
};

/*@
   via_init - initialize via method

   Notes:
@*/
int via_init( void )
{
    MPIDI_STATE_DECL(MPID_STATE_VIA_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_VIA_INIT);
    MPIDI_FUNC_EXIT(MPID_STATE_VIA_INIT);
    return MPI_SUCCESS;
}

/*@
   via_finalize - finalize via method

   Notes:
@*/
int via_finalize( void )
{
    MPIDI_STATE_DECL(MPID_STATE_VIA_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_VIA_FINALIZE);
    MPIDI_FUNC_EXIT(MPID_STATE_VIA_FINALIZE);
    return MPI_SUCCESS;
}

#endif
