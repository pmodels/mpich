/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ibimpl.h"

#ifdef WITH_METHOD_IB

int ib_merge_with_posted(MM_Car *pkt_car_ptr, MM_Car *posted_car_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_MERGE_WITH_POSTED);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_MERGE_WITH_POSTED);
    
    /* copy the received packet into the posted packet */
    /* This will update all the variables from the requested values to
       the actual values.  For example, the posted length will be set 
       to the actual length. 
       */
    posted_car_ptr->msg_header.pkt = pkt_car_ptr->msg_header.pkt;
    
    if (pkt_car_ptr->msg_header.pkt.u.hdr.type == MPID_EAGER_PKT)
    {
	/* mark the head car as completed */
	/*printf("dec cc: read eager head car\n");fflush(stdout);*/
	mm_dec_cc_atomic(posted_car_ptr->request_ptr);
	posted_car_ptr = posted_car_ptr->next_ptr;
	
	if (posted_car_ptr)
	{
	    /* start reading the eager data */
	    MPIU_DBG_PRINTF(("ib_merge_with_posted: reading eager data\n"));
	    posted_car_ptr->vc_ptr->data.ib.reading_header = FALSE;
	    ib_enqueue_read_at_head(posted_car_ptr->vc_ptr, posted_car_ptr);
	}
	else
	{
	    /* post a read for the next header packet */
	    MPIU_DBG_PRINTF(("ib_merge_with_posted: message complete, posting next packet read\n"));
	    ib_post_read_pkt(posted_car_ptr->vc_ptr);
	}
    } 
    else if (pkt_car_ptr->msg_header.pkt.u.hdr.type == MPID_RNDV_REQUEST_TO_SEND_PKT)
    {
	err_printf("ib_merge_with_unexpected doesn't handle unexpected rndv yet.\n");
	mm_post_rndv_clear_to_send(posted_car_ptr, pkt_car_ptr);
    }
    else
    {
	err_printf("ib_merge_with_posted cannot process packet of type: %d\n", 
	    pkt_car_ptr->msg_header.pkt.u.hdr.type);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_WITH_POSTED);
    return MPI_SUCCESS;
}

#endif
