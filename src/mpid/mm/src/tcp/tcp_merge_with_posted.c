/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

#ifdef WITH_METHOD_TCP

int tcp_merge_with_posted(MM_Car *pkt_car_ptr, MM_Car *posted_car_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_TCP_MERGE_WITH_POSTED);

    MPIDI_FUNC_ENTER(MPID_STATE_TCP_MERGE_WITH_POSTED);
    
    /* copy the packet into the posted packet */
    posted_car_ptr->msg_header.pkt = pkt_car_ptr->msg_header.pkt;
    
    if (pkt_car_ptr->msg_header.pkt.u.hdr.type == MPID_EAGER_PKT)
    {
	/* mark the head car as completed */
	/*printf("dec cc: read eager head car\n");fflush(stdout);*/
	mm_dec_cc_atomic(posted_car_ptr->request_ptr);
	posted_car_ptr = posted_car_ptr->next_ptr;
	
	/* start reading the eager data */
	if (posted_car_ptr)
	    tcp_car_head_enqueue(posted_car_ptr->vc_ptr, posted_car_ptr);
    } 
    else if (pkt_car_ptr->msg_header.pkt.u.hdr.type == MPID_RNDV_REQUEST_TO_SEND_PKT)
    {
	/*err_printf("tcp_merge_with_unexpected doesn't handle unexpected rndv yet.\n");*/
	mm_post_rndv_clear_to_send(posted_car_ptr, pkt_car_ptr);
    }
    else
    {
	err_printf("tcp_merge_with_unexpected cannot process packet of type: %d\n", 
	    pkt_car_ptr->msg_header.pkt.u.hdr.type);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_TCP_MERGE_WITH_POSTED);
    return MPI_SUCCESS;
}

#endif
