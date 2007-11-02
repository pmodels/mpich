/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_post_rndv_clear_to_send(MM_Car *posted_car_ptr, MM_Car *rndv_rts_car_ptr)
{
    MM_Car *rndv_car_ptr;
    MPID_Rndv_clear_to_send_pkt *rndv_cts_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MM_POST_RNDV_CLEAR_TO_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_POST_RNDV_CLEAR_TO_SEND);

    rndv_car_ptr = mm_car_alloc();
    
    posted_car_ptr->vc_ptr->fn.setup_packet_car(
	posted_car_ptr->vc_ptr,
	MM_WRITE_CAR,
	posted_car_ptr->src, /* this could be an error because src could be MPI_ANY_SRC */
	rndv_car_ptr);
    rndv_car_ptr->request_ptr = posted_car_ptr->request_ptr;
    /* increment the completion counter once for the cts packet */
    /*printf("inc cc: cts\n");fflush(stdout);*/
    mm_inc_cc_atomic(posted_car_ptr->request_ptr);

    /* set up the cts header packet */
    rndv_cts_ptr = &rndv_car_ptr->msg_header.pkt.u.cts;
    rndv_cts_ptr->type = MPID_RNDV_CLEAR_TO_SEND_PKT;

#ifdef MPICH_DEV_BUILD
    if ((unsigned long)posted_car_ptr < 1000)
	err_printf("Error: mm_post_rndv_clear_to_send setting invalid receiver_car_ptr: %u\n", posted_car_ptr);
#endif
    rndv_cts_ptr->receiver_car_ptr = posted_car_ptr;

#ifdef MPICH_DEV_BUILD
    if ((unsigned long)rndv_rts_car_ptr->msg_header.pkt.u.hdr.sender_car_ptr < 1000)
	err_printf("Error: mm_post_rndv_clear_to_send setting invalid send_car_ptr: %u\n", rndv_rts_car_ptr->msg_header.pkt.u.hdr.sender_car_ptr);
#endif
    rndv_cts_ptr->sender_car_ptr = rndv_rts_car_ptr->msg_header.pkt.u.hdr.sender_car_ptr;

    posted_car_ptr->vc_ptr->fn.post_write(posted_car_ptr->vc_ptr, rndv_car_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_MM_POST_RNDV_CLEAR_TO_SEND);
    return MPI_SUCCESS;
}
