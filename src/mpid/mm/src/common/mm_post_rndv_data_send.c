/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_post_rndv_data_send(MM_Car *rndv_cts_car_ptr)
{
    MPID_Rndv_data_pkt *rndv_data_ptr;
    MM_Car *sender_car_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MM_POST_RNDV_DATA_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_POST_RNDV_DATA_SEND);

    /* convert the sender car into a rndv data car */
#ifdef MPICH_DEV_BUILD
    if ((unsigned long)rndv_cts_car_ptr->msg_header.pkt.u.cts.sender_car_ptr < 1000)
	err_printf("Error: mm_post_rndv_data_send setting invalid send_car_ptr: %u\n", rndv_cts_car_ptr->msg_header.pkt.u.cts.sender_car_ptr);
#endif
    sender_car_ptr = rndv_cts_car_ptr->msg_header.pkt.u.cts.sender_car_ptr;

    /* set up the rndv data header packet */
    rndv_data_ptr = &sender_car_ptr->msg_header.pkt.u.rdata;
#ifdef MPICH_DEV_BUILD
    if ((unsigned long)rndv_cts_car_ptr->msg_header.pkt.u.cts.receiver_car_ptr < 1000)
	err_printf("Error: mm_post_rndv_data_send setting invalid receiver_car_ptr: %u\n", rndv_cts_car_ptr->msg_header.pkt.u.cts.receiver_car_ptr);
#endif
    rndv_data_ptr->receiver_car_ptr = rndv_cts_car_ptr->msg_header.pkt.u.cts.receiver_car_ptr;
    rndv_data_ptr->size = 0; /* How do I figure out this value? */
    rndv_data_ptr->type = MPID_RNDV_DATA_PKT;

    sender_car_ptr->vc_ptr->fn.post_write(sender_car_ptr->vc_ptr, sender_car_ptr);
    /*mm_dec_cc_atomic(sender_car_ptr->request_ptr);*/ /* decrement once for the unsent msg header car */

    MPIDI_FUNC_EXIT(MPID_STATE_MM_POST_RNDV_DATA_SEND);
    return MPI_SUCCESS;
}
