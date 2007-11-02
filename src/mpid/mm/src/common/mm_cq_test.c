/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* local prototypes */
MM_Car *find_in_queue(MM_Car **, MM_Car **, MM_Car *);
int mm_cq_handle_read_head_car(MM_Car *);
int mm_cq_handle_read_data_car(MM_Car *);
int mm_cq_handle_read_car(MM_Car *);
#if 0
int mm_cq_handle_write_head_car(MM_Car *);
int mm_cq_handle_write_data_car(MM_Car *);
int mm_cq_handle_write_car(MM_Car *);
#else
int mm_cq_handle_write_car(MM_Car *);
#endif

MM_Car *find_in_queue(MM_Car **find_q_head_ptr, MM_Car **find_q_tail_ptr, MM_Car *car_ptr)
{
    MM_Car *iter_ptr, *trailer_ptr;
    MPIDI_STATE_DECL(MPID_STATE_FIND_IN_QUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_FIND_IN_QUEUE);

    trailer_ptr = iter_ptr = *find_q_head_ptr;
    while (iter_ptr)
    {
	if ((iter_ptr->msg_header.pkt.u.hdr.context == car_ptr->msg_header.pkt.u.hdr.context) &&
	    (iter_ptr->msg_header.pkt.u.hdr.tag == car_ptr->msg_header.pkt.u.hdr.tag) &&
	    (iter_ptr->msg_header.pkt.u.hdr.src == car_ptr->msg_header.pkt.u.hdr.src))
	{
	    if (iter_ptr->msg_header.pkt.u.hdr.size > car_ptr->msg_header.pkt.u.hdr.size)
	    {
		err_printf("Error: unex msg size %d > posted msg size %d\n", iter_ptr->msg_header.pkt.u.hdr.size, car_ptr->msg_header.pkt.u.hdr.size);
		return NULL;
	    }
	    /* dequeue the car from the posted_q */
	    if (trailer_ptr == iter_ptr)
	    {
		if ((*find_q_head_ptr = iter_ptr->qnext_ptr) == NULL)
		    *find_q_tail_ptr = NULL;
	    }
	    else
	    {
		trailer_ptr->qnext_ptr = iter_ptr->qnext_ptr;
		if (*find_q_tail_ptr == iter_ptr)
		    *find_q_tail_ptr = trailer_ptr;
	    }
	    MPIDI_FUNC_EXIT(MPID_STATE_FIND_IN_QUEUE);
	    return iter_ptr;
	}
	if (trailer_ptr != iter_ptr)
	    trailer_ptr = trailer_ptr->qnext_ptr;
	iter_ptr = iter_ptr->qnext_ptr;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_FIND_IN_QUEUE);
    return NULL;
}

int mm_cq_handle_read_head_car(MM_Car *car_ptr)
{
    MM_Car *qcar_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MM_CQ_HANDLE_READ_HEAD_CAR);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_CQ_HANDLE_READ_HEAD_CAR);

    switch (car_ptr->msg_header.pkt.u.type)
    {
    case MPID_EAGER_PKT:
	MPID_Thread_lock(MPID_Process.qlock);
	qcar_ptr = find_in_queue(&MPID_Process.posted_q_head, &MPID_Process.posted_q_tail, car_ptr);
	if (qcar_ptr)
	{
	    /* merge the received header car with the posted car */
	    MPIU_DBG_PRINTF(("mm_cq_handle_read_head_car: receive found, merging with posted\n"));
	    car_ptr->vc_ptr->fn.merge_with_posted(car_ptr, qcar_ptr);
	}
	else
	{
	    /* else allocate a temp buffer, place in the unex_q, and post a read */
	    /*MPIU_dbg_*/printf("mm_cq_handle_read_head_car: receive not found, creating unexpected\n");fflush(stdout);
	    mm_create_post_unex(car_ptr);
	}
	MPID_Thread_unlock(MPID_Process.qlock);
	break;
    case MPID_RNDV_REQUEST_TO_SEND_PKT:
	MPID_Thread_lock(MPID_Process.qlock);
	qcar_ptr = find_in_queue(&MPID_Process.posted_q_head, &MPID_Process.posted_q_tail, car_ptr);
	if (qcar_ptr)
	{
	    /* send cts header packet */
	    mm_post_rndv_clear_to_send(qcar_ptr, car_ptr);
	}
	else
	{
	    /* post car in unex queue */
	    mm_enqueue_request_to_send(car_ptr);
	}
	MPID_Thread_unlock(MPID_Process.qlock);
	/* no data to follow this packet so post a receive for another packet */
	if (car_ptr->vc_ptr->fn.post_read_pkt)
	    car_ptr->vc_ptr->fn.post_read_pkt(car_ptr->vc_ptr);
	break;
    case MPID_RNDV_CLEAR_TO_SEND_PKT:
	/* post the rndv_data head packet for writing */
	mm_post_rndv_data_send(car_ptr);
	/*mm_dec_cc_atomic(car_ptr->request_ptr);*/ /* decrement once for the header packet? */
	/*mm_car_free(car_ptr);*/ /* This car doesn't need to be freed because it is static in the vc */
	/* no data to follow this packet so post a receive for another packet */
	if (car_ptr->vc_ptr->fn.post_read_pkt)
	    car_ptr->vc_ptr->fn.post_read_pkt(car_ptr->vc_ptr);
	break;
    case MPID_RNDV_DATA_PKT:
	/* decrement the completion counter for the head receive car */
	mm_dec_cc_atomic(car_ptr->msg_header.pkt.u.rdata.receiver_car_ptr->request_ptr);
	/* enqueue a read for the rndv data */
	car_ptr->vc_ptr->fn.enqueue_read_at_head(car_ptr->vc_ptr, car_ptr->msg_header.pkt.u.rdata.receiver_car_ptr->next_ptr);
	break;
    case MPID_RDMA_ACK_PKT:
	err_printf("Error: RDMA_ACK_PKT not handled yet.\n");
	break;
    case MPID_RDMA_DATA_ACK_PKT:
	err_printf("Error: RDMA_DATA_ACK_PKT not handled yet.\n");
	break;
    case MPID_RDMA_REQUEST_DATA_ACK_PKT:
	err_printf("Error: RDMA_REQUEST_DATA_ACK_PKT not handled yet.\n");
	break;
    default:
	err_printf("Error: mm_cq_handle_read_head_car: unknown car type %d\n", car_ptr->msg_header.pkt.u.type);
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MM_CQ_HANDLE_READ_HEAD_CAR);
    return MPI_SUCCESS;
}

int mm_cq_handle_read_data_car(MM_Car *car_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MM_CQ_HANDLE_READ_DATA_CAR);
    MPIDI_FUNC_ENTER(MPID_STATE_MM_CQ_HANDLE_READ_DATA_CAR);

    if (car_ptr->next_ptr)
    {
	/* enqueue next car to be read before any other pending cars */
	car_ptr->vc_ptr->fn.enqueue_read_at_head(car_ptr->vc_ptr, car_ptr->next_ptr);
    }
    else
    {
	if (car_ptr->vc_ptr->fn.post_read_pkt)
	    car_ptr->vc_ptr->fn.post_read_pkt(car_ptr->vc_ptr);
    }
    /*printf("dec cc: read data car\n");fflush(stdout);*/
    mm_dec_cc_atomic(car_ptr->request_ptr);
    mm_car_free(car_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_MM_CQ_HANDLE_READ_DATA_CAR);
    return MPI_SUCCESS;
}

int mm_cq_handle_read_car(MM_Car *car_ptr)
{
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_MM_CQ_HANDLE_READ_CAR);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_CQ_HANDLE_READ_CAR);

    if (car_ptr->type & MM_HEAD_CAR)
    {
	ret_val = mm_cq_handle_read_head_car(car_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_MM_CQ_HANDLE_READ_CAR);
	return ret_val;
    }

    ret_val = mm_cq_handle_read_data_car(car_ptr);
    MPIDI_FUNC_EXIT(MPID_STATE_MM_CQ_HANDLE_READ_CAR);
    return ret_val;
}

#if 0
int mm_cq_handle_write_head_car(MM_Car *car_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MM_CQ_HANDLE_WRITE_HEAD_CAR);
    MPIDI_FUNC_ENTER(MPID_STATE_MM_CQ_HANDLE_WRITE_HEAD_CAR);

    /* rndv */
    if (car_ptr->msg_header.pkt.u.hdr.type == MPID_RNDV_REQUEST_TO_SEND_PKT)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_MM_CQ_HANDLE_WRITE_HEAD_CAR);
	return MPI_SUCCESS;
    }

    /* eager */
    if (car_ptr->next_ptr)
    {
	car_ptr->vc_ptr->fn.enqueue_write_at_head(car_ptr->vc_ptr, car_ptr->next_ptr);
    }
    /*printf("dec cc: written head car\n");fflush(stdout);*/
    mm_dec_cc_atomic(car_ptr->request_ptr);
    mm_car_free(car_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_MM_CQ_HANDLE_WRITE_HEAD_CAR);
    return MPI_SUCCESS;
}

int mm_cq_handle_write_data_car(MM_Car *car_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MM_CQ_HANDLE_WRITE_DATA_CAR);
    MPIDI_FUNC_ENTER(MPID_STATE_MM_CQ_HANDLE_WRITE_DATA_CAR);

    if (car_ptr->next_ptr)
    {
	/* enqueue next car to be written before any other pending cars */
	car_ptr->vc_ptr->fn.enqueue_write_at_head(car_ptr->vc_ptr, car_ptr->next_ptr);
    }
    /*printf("dec cc: written data car\n");fflush(stdout);*/
    mm_dec_cc_atomic(car_ptr->request_ptr);
    mm_car_free(car_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_MM_CQ_HANDLE_WRITE_DATA_CAR);
    return MPI_SUCCESS;
}

int mm_cq_handle_write_car(MM_Car *car_ptr)
{
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_MM_CQ_HANDLE_WRITE_CAR);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_CQ_HANDLE_WRITE_CAR);

    if (car_ptr->type & MM_HEAD_CAR)
    {
	ret_val = mm_cq_handle_write_head_car(car_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_MM_CQ_HANDLE_WRITE_CAR);
	return ret_val;
    }

    ret_val = mm_cq_handle_write_data_car(car_ptr);
    MPIDI_FUNC_EXIT(MPID_STATE_MM_CQ_HANDLE_WRITE_CAR);
    return ret_val;
}

#else

int mm_cq_handle_write_car(MM_Car *car_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MM_CQ_HANDLE_WRITE_CAR);
    MPIDI_FUNC_ENTER(MPID_STATE_MM_CQ_HANDLE_WRITE_CAR);

    mm_dec_cc_atomic(car_ptr->request_ptr);
    mm_car_free(car_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_MM_CQ_HANDLE_WRITE_CAR);
    return MPI_SUCCESS;
}
#endif

int mm_cq_test()
{
    MM_Car *car_ptr, *next_car_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MM_CQ_TEST);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_CQ_TEST);

    /*dbg_printf(".");*/

    /* Should we call make_progress on all the methods?
     * before checking the cq?
     * after checking the cq?
     * only if the cq is empty?
     * Should this be a function or inline?
     */
    if (MPID_Process.cq_head == NULL)
    {
	mm_make_progress();
    }

    if (MPID_Process.cq_head == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_MM_CQ_TEST);
	return MPI_SUCCESS;
    }

    /* remove all the cars from the completion queue */
    MPID_Thread_lock(MPID_Process.cqlock);
    car_ptr = MPID_Process.cq_head;
    MPID_Process.cq_head = NULL;
    MPID_Process.cq_tail = NULL;
    MPID_Thread_unlock(MPID_Process.cqlock);

    /* handle all the complete cars */
    do
    {
	/* save the next field because handling the car can destroy it */
	next_car_ptr = car_ptr->qnext_ptr;

	if (car_ptr->type & MM_READ_CAR)
	{
	    mm_cq_handle_read_car(car_ptr);
	}

	if (car_ptr->type & MM_WRITE_CAR)
	{
	    mm_cq_handle_write_car(car_ptr);
	}

	car_ptr = next_car_ptr;
    }while (car_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_MM_CQ_TEST);
    return MPI_SUCCESS;
}
