/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "socketimpl.h"

#ifdef WITH_METHOD_SOCKET

int socket_post_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MM_Car *rndv_car_ptr;
    MPID_Header_pkt *rndv_rts_ptr;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_POST_WRITE);

    MPIU_DBG_PRINTF(("socket_post_write\n"));

#ifdef MPICH_DEV_BUILD
    if (!(car_ptr->type & MM_HEAD_CAR))
    {
	err_printf("socket_post_write: only head cars can be posted.\n");
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_WRITE);
	return -1;
    }
#endif

    if ((car_ptr->msg_header.pkt.u.type != MPID_EAGER_PKT) ||
	(car_ptr->msg_header.pkt.u.hdr.size < SOCKET_Process.nSOCKET_EAGER_LIMIT))
    {
	/* enqueue the head packet car */
	/*msg_printf("socket_post_write: enqueueing packet\n");*/
	socket_car_enqueue_write(vc_ptr, car_ptr);
    }
    else
    {
	/* create a request to send car */
	rndv_car_ptr = mm_car_alloc();

	/* set up the rts car to use its internal buf and pkt fields */
	socket_setup_packet_car(vc_ptr, MM_WRITE_CAR, car_ptr->dest, rndv_car_ptr);
	rndv_car_ptr->request_ptr = car_ptr->request_ptr;
	/* increment the completion counter for this rts packet */
	/*printf("inc cc: rts\n");fflush(stdout);*/
	mm_inc_cc_atomic(car_ptr->request_ptr);

	/* set up the rts header packet, pointing it to the original car */
	rndv_rts_ptr = &rndv_car_ptr->msg_header.pkt.u.hdr;
	rndv_rts_ptr->context = car_ptr->msg_header.pkt.u.hdr.context;
	rndv_rts_ptr->size = car_ptr->msg_header.pkt.u.hdr.size;
	rndv_rts_ptr->src = car_ptr->msg_header.pkt.u.hdr.src;
	rndv_rts_ptr->tag = car_ptr->msg_header.pkt.u.hdr.tag;
	rndv_rts_ptr->type = MPID_RNDV_REQUEST_TO_SEND_PKT;
	if ((unsigned long)car_ptr < 1000)
	    msg_printf("Error: socket_post_write setting invalid send_car_ptr: %u\n", car_ptr);
	rndv_rts_ptr->sender_car_ptr = car_ptr;

	/*printf("enqueueing rts packet.\n");fflush(stdout);*/
	/* enqueue the request to send car */
	socket_car_enqueue_write(vc_ptr, rndv_car_ptr);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_WRITE);
    return MPI_SUCCESS;
}

#endif
