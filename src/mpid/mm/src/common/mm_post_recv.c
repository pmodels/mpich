/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_post_recv(MM_Car *car_ptr)
{
    MM_Car *iter_ptr, *trailer_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MM_POST_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_POST_RECV);
    dbg_printf("mm_post_recv\n");
    
    /* find in unex_q or enqueue into the posted_q */
    MPID_Thread_lock(MPID_Process.qlock);
    trailer_ptr = iter_ptr = MPID_Process.unex_q_head;
    while (iter_ptr)
    {
	if ((iter_ptr->msg_header.pkt.u.hdr.context == car_ptr->msg_header.pkt.u.hdr.context) &&
	    (iter_ptr->msg_header.pkt.u.hdr.tag == car_ptr->msg_header.pkt.u.hdr.tag) &&
	    (iter_ptr->msg_header.pkt.u.hdr.src == car_ptr->msg_header.pkt.u.hdr.src))
	{
	    if (iter_ptr->msg_header.pkt.u.hdr.size > car_ptr->msg_header.pkt.u.hdr.size)
	    {
		err_printf("Error: unex msg size %d > posted msg size %d\n", 
		    iter_ptr->msg_header.pkt.u.hdr.size, 
		    car_ptr->msg_header.pkt.u.hdr.size);
		MPIDI_FUNC_EXIT(MPID_STATE_MM_POST_RECV);
		return -1;
	    }
	    /* dequeue the car from the unex_q */
	    if (trailer_ptr == iter_ptr)
	    {
		MPID_Process.unex_q_head = iter_ptr->qnext_ptr;
		if (MPID_Process.unex_q_head == NULL)
		    MPID_Process.unex_q_tail = NULL;
	    }
	    else
	    {
		trailer_ptr->qnext_ptr = iter_ptr->qnext_ptr;
		if (MPID_Process.unex_q_tail == iter_ptr)
		    MPID_Process.unex_q_tail = trailer_ptr;
	    }
	    MPID_Thread_unlock(MPID_Process.qlock);
	    /* merge the unex car with the posted car using the method in the vc */
	    iter_ptr->vc_ptr->fn.merge_with_unexpected(car_ptr, iter_ptr);
	    MPIDI_FUNC_EXIT(MPID_STATE_MM_POST_RECV);
	    return MPI_SUCCESS;
	}
	if (trailer_ptr != iter_ptr)
	    trailer_ptr = trailer_ptr->qnext_ptr;
	iter_ptr = iter_ptr->qnext_ptr;
    }

    /* the car was not found in the unexpected queue so put it in the posted queueu */
    if (MPID_Process.posted_q_tail == NULL)
    {
	MPID_Process.posted_q_head = car_ptr;
    }
    else
    {
	MPID_Process.posted_q_tail->qnext_ptr = car_ptr;
    }
    car_ptr->qnext_ptr = NULL;
    MPID_Process.posted_q_tail = car_ptr;

    MPID_Thread_unlock(MPID_Process.qlock);

    MPIDI_FUNC_EXIT(MPID_STATE_MM_POST_RECV);
    return MPI_SUCCESS;
}
