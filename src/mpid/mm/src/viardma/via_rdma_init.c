/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "via_rdmaimpl.h"

#ifdef WITH_METHOD_VIA_RDMA

MPIDI_VC_functions g_via_rdma_vc_functions = 
{
    NULL,/*via_rdma_post_read,*/
    NULL, /*via_rdma_car_head_enqueue_read,*/
    via_rdma_merge_with_unexpected,
    NULL, /*via_rdma_merge_with_posted,*/
    NULL, /*via_rdma_merge_unexpected_data,*/
    via_rdma_post_write,
    NULL, /*via_rdma_car_head_enqueue_write,*/
    NULL, /*via_rdma_reset_car,*/
    NULL, /*via_rdma_setup_packet_car,*/
    NULL /*via_rdma_post_read_pkt*/
};

/*@
   via_rdma_init - initialize via_rdma method

   Notes:
@*/
int via_rdma_init( void )
{
    MPIDI_STATE_DECL(MPID_STATE_VIA_RDMA_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_VIA_RDMA_INIT);
    MPIDI_FUNC_EXIT(MPID_STATE_VIA_RDMA_INIT);
    return MPI_SUCCESS;
}

/*@
   via_rdma_finalize - finalize via_rdma method

   Notes:
@*/
int via_rdma_finalize( void )
{
    MPIDI_STATE_DECL(MPID_STATE_VIA_RDMA_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_VIA_RDMA_FINALIZE);
    MPIDI_FUNC_EXIT(MPID_STATE_VIA_RDMA_FINALIZE);
    return MPI_SUCCESS;
}

#endif
