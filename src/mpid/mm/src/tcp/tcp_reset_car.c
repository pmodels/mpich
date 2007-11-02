/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "tcpimpl.h"

#ifdef WITH_METHOD_TCP

/*@
   tcp_reset_car - reset car

   Parameters:
+  MM_Car *car_ptr - car

   Notes:
@*/
int tcp_reset_car(MM_Car *car_ptr)
{
    MM_Segment_buffer *buf_ptr;
    MPIDI_STATE_DECL(MPID_STATE_TCP_RESET_CAR);

    MPIDI_FUNC_ENTER(MPID_STATE_TCP_RESET_CAR);

    buf_ptr = car_ptr->buf_ptr;
    if (buf_ptr == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_RESET_CAR);
	return -1;
    }

    switch (buf_ptr->type)
    {
    case MM_NULL_BUFFER:
	break;
    case MM_SIMPLE_BUFFER:
	car_ptr->data.tcp.buf.simple.num_written = 0;
	break;
    case MM_TMP_BUFFER:
	car_ptr->data.tcp.buf.tmp.num_written = 0;
	break;
    case MM_VEC_BUFFER:
	if (car_ptr->type & MM_WRITE_CAR)
	{
	    car_ptr->data.tcp.buf.vec_write.cur_index = 0;
	    car_ptr->data.tcp.buf.vec_write.num_read_copy = 0;
	    car_ptr->data.tcp.buf.vec_write.cur_num_written = 0;
	    car_ptr->data.tcp.buf.vec_write.total_num_written = 0;
	    car_ptr->data.tcp.buf.vec_write.num_written_at_cur_index = 0;
	    car_ptr->data.tcp.buf.vec_write.vec_size = 0;
	}
	else
	{
	    car_ptr->data.tcp.buf.vec_read.cur_index = 0;
	    car_ptr->data.tcp.buf.vec_read.cur_num_read = 0;
	    car_ptr->data.tcp.buf.vec_read.total_num_read = 0;
	    car_ptr->data.tcp.buf.vec_read.vec_size = 0;
	}
	break;
#ifdef WITH_METHOD_SHM
    case MM_SHM_BUFFER:
	break;
#endif
#ifdef WITH_METHOD_VIA
    case MM_VIA_BUFFER:
	break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    case MM_VIA_RDMA_BUFFER:
	break;
#endif
#ifdef WITH_METHOD_IB
    case MM_IB_BUFFER:
	break;
#endif
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD_BUFFER:
	break;
#endif
    default:
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_TCP_RESET_CAR);
    return MPI_SUCCESS;
}

/*@
   tcp_setup_packet_car - set up a car to read or write an mpid packet

   Parameters:
+  MPIDI_VC *vc_ptr - vc
.  MM_CAR_TYPE read_write - read or write car type
.  int src_dest - source or destination
-  MM_Car *car_ptr - car

   Notes:
@*/
int tcp_setup_packet_car(MPIDI_VC *vc_ptr, MM_CAR_TYPE read_write, int src_dest, MM_Car *car_ptr)
{
    MM_Segment_buffer *buf_ptr;
    MPIDI_STATE_DECL(MPID_STATE_TCP_SETUP_PACKET_CAR);

    MPIDI_FUNC_ENTER(MPID_STATE_TCP_SETUP_PACKET_CAR);

    buf_ptr = &car_ptr->msg_header.buf;
    
    /* set up the car */    
    car_ptr->vc_ptr = vc_ptr;
    car_ptr->next_ptr = NULL;
    car_ptr->opnext_ptr = NULL;
    car_ptr->qnext_ptr = NULL;
    car_ptr->request_ptr = NULL;
    car_ptr->buf_ptr = buf_ptr;

    /* set up the vector for reading or writing */
    switch (read_write)
    {
    case MM_READ_CAR:
	car_ptr->type = MM_HEAD_CAR | MM_READ_CAR;
	car_ptr->src = src_dest;
#ifdef MPICH_DEV_BUILD
	car_ptr->data.tcp.buf.simple.num_written = -1;
#endif
	/* readers have no data yet */
	buf_ptr->simple.num_read = 0;
	break;
    case MM_WRITE_CAR:
	car_ptr->type = MM_HEAD_CAR | MM_WRITE_CAR;
	car_ptr->dest = src_dest;
	car_ptr->data.tcp.buf.simple.num_written = 0;
	
	/* writers have the data ready */
	buf_ptr->simple.num_read = sizeof(MPID_Packet);
	break;
    default:
	err_printf("Error: tcp_setup_packet_car: invalid car type, %d\n", read_write);
	break;
    }
    
    /* set up the buffer */
    buf_ptr->type = MM_SIMPLE_BUFFER;
    buf_ptr->simple.buf = (void*)&car_ptr->msg_header.pkt;
    buf_ptr->simple.len = sizeof(MPID_Packet);

    MPIDI_FUNC_EXIT(MPID_STATE_TCP_SETUP_PACKET_CAR);
    return MPI_SUCCESS;
}

#ifdef USE_VECTOR_BUFFER_FOR_PACKETS
int tcp_setup_packet_car(MPIDI_VC *vc_ptr, MM_CAR_TYPE read_write, int src_dest, MM_Car *car_ptr)
{
    MM_Segment_buffer *buf_ptr;
    MPIDI_STATE_DECL(MPID_STATE_TCP_SETUP_PACKET_CAR);

    MPIDI_FUNC_ENTER(MPID_STATE_TCP_SETUP_PACKET_CAR);

    buf_ptr = &car_ptr->msg_header.buf;
    
    /* set up the car */    
    car_ptr->vc_ptr = vc_ptr;
    car_ptr->next_ptr = NULL;
    car_ptr->opnext_ptr = NULL;
    car_ptr->qnext_ptr = NULL;
    car_ptr->request_ptr = NULL;
    car_ptr->buf_ptr = buf_ptr;

    /* set up the vector for reading or writing */
    switch (read_write)
    {
    case MM_READ_CAR:
	car_ptr->type = MM_HEAD_CAR | MM_READ_CAR;
	car_ptr->src = src_dest;
	car_ptr->data.tcp.buf.vec_read.cur_index = 0;
	car_ptr->data.tcp.buf.vec_read.cur_num_read = 0;
	car_ptr->data.tcp.buf.vec_read.total_num_read = 0;
	car_ptr->data.tcp.buf.vec_read.vec[0].MPID_IOV_BUF = (void*)&car_ptr->msg_header.pkt;
	car_ptr->data.tcp.buf.vec_read.vec[0].MPID_IOV_LEN = sizeof(MPID_Packet);
	car_ptr->data.tcp.buf.vec_read.vec_size = 1;

	/* readers have no data yet */
	buf_ptr->vec.num_read = 0;
	break;
    case MM_WRITE_CAR:
	car_ptr->type = MM_HEAD_CAR | MM_WRITE_CAR;
	car_ptr->dest = src_dest;
	car_ptr->data.tcp.buf.vec_write.cur_index = 0;
	car_ptr->data.tcp.buf.vec_write.num_written_at_cur_index = 0;
	car_ptr->data.tcp.buf.vec_write.cur_num_written = 0;
	car_ptr->data.tcp.buf.vec_write.total_num_written = 0;
	car_ptr->data.tcp.buf.vec_write.num_read_copy = 0;
	car_ptr->data.tcp.buf.vec_write.vec[0].MPID_IOV_BUF = (void*)&car_ptr->msg_header.pkt;
	car_ptr->data.tcp.buf.vec_write.vec[0].MPID_IOV_LEN = sizeof(MPID_Packet);
	car_ptr->data.tcp.buf.vec_write.vec_size = 1;
	
	/* writers have the data ready */
	buf_ptr->vec.num_read = sizeof(MPID_Packet);
	break;
    default:
	break;
    }
    
    /* set up the buffer */
    buf_ptr->type = MM_VEC_BUFFER;
    buf_ptr->vec.vec[0].MPID_IOV_BUF = (void*)&car_ptr->msg_header.pkt;
    buf_ptr->vec.vec[0].MPID_IOV_LEN = sizeof(MPID_Packet);
    buf_ptr->vec.vec_size = 1;
    buf_ptr->vec.first = 0;
    buf_ptr->vec.last = sizeof(MPID_Packet);
    buf_ptr->vec.segment_last = sizeof(MPID_Packet);
    buf_ptr->vec.buf_size = sizeof(MPID_Packet);
    buf_ptr->vec.num_cars = 1;
    buf_ptr->vec.num_cars_outstanding = 1;

    MPIDI_FUNC_EXIT(MPID_STATE_TCP_SETUP_PACKET_CAR);
    return MPI_SUCCESS;
}
#endif

#endif
