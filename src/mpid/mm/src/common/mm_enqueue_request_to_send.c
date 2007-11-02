/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_enqueue_request_to_send(MM_Car *unex_head_car_ptr)
{
    MM_Car *car_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MM_ENQUEUE_REQUEST_TO_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_ENQUEUE_REQUEST_TO_SEND);
    dbg_printf("mm_enqueue_request_to_send\n");

    /*printf("mm_enqueue_request_to_send\n");fflush(stdout);*/
    car_ptr = mm_car_alloc();

    car_ptr->msg_header = unex_head_car_ptr->msg_header;
    /*
    car_ptr->msg_header.pkt.u.hdr.context = unex_head_car_ptr->msg_header.pkt.u.hdr.context;
    car_ptr->msg_header.pkt.u.hdr.sender_car_ptr = unex_head_car_ptr->msg_header.pkt.u.hdr.sender_car_ptr;
    car_ptr->msg_header.pkt.u.hdr.size = unex_head_car_ptr->msg_header.pkt.u.hdr.size;
    car_ptr->msg_header.pkt.u.hdr.src = unex_head_car_ptr->msg_header.pkt.u.hdr.src;
    car_ptr->msg_header.pkt.u.hdr.tag = unex_head_car_ptr->msg_header.pkt.u.hdr.tag;
    car_ptr->msg_header.pkt.u.hdr.type = unex_head_car_ptr->msg_header.pkt.u.hdr.type;
    */
#ifdef MPICH_DEV_BUILD
    if ((unsigned long)unex_head_car_ptr->msg_header.pkt.u.hdr.sender_car_ptr < 1000)
	err_printf("Error: mm_enqueue_request_to_send setting invalid send_car_ptr: %u\n", unex_head_car_ptr->msg_header.pkt.u.hdr.sender_car_ptr);
#endif

    car_ptr->buf_ptr = &car_ptr->msg_header.buf;
    car_ptr->vc_ptr = unex_head_car_ptr->vc_ptr;
    car_ptr->qnext_ptr = NULL;
    car_ptr->next_ptr = NULL;
    car_ptr->opnext_ptr = NULL;
    car_ptr->vcqnext_ptr = NULL;
    car_ptr->type = MM_READ_CAR | MM_HEAD_CAR;

    /* enqueue the car in the unexpected queue */
    if (MPID_Process.unex_q_tail == NULL)
	MPID_Process.unex_q_head = car_ptr;
    else
	MPID_Process.unex_q_tail->qnext_ptr = car_ptr;
    MPID_Process.unex_q_tail = car_ptr;

    MPIDI_FUNC_EXIT(MPID_STATE_MM_ENQUEUE_REQUEST_TO_SEND);
    return MPI_SUCCESS;
}
