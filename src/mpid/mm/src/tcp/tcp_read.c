/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

#ifdef WITH_METHOD_TCP

#ifndef HAVE_WINSOCK2_H
#include <errno.h>
#endif

#ifdef WITH_METHOD_SHM
int tcp_read_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
#ifdef WITH_METHOD_VIA
int tcp_read_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
#ifdef WITH_METHOD_VIA_RDMA
int tcp_read_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
int tcp_read_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int tcp_read_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int tcp_read_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int tcp_read_connecting(MPIDI_VC *vc_ptr);
#ifdef WITH_METHOD_IB
int tcp_read_ib(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
#ifdef WITH_METHOD_NEW
int tcp_read_new(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif

int tcp_read_header(MPIDI_VC *vc_ptr)
{
    int num_read;
    MPIDI_STATE_DECL(MPID_STATE_TCP_READ_HEADER);

    MPIDI_FUNC_ENTER(MPID_STATE_TCP_READ_HEADER);
#ifdef MPICH_DEV_BUILD
    if (vc_ptr->data.tcp.bytes_of_header_read == sizeof(MPID_Packet))
    {
	err_printf("tcp_read_header called but the entire header has already been read.\n");
    }
    if (!vc_ptr->data.tcp.connected)
    {
	err_printf("tcp_read_header called on an unconnected vc\n");
    }
#endif
    num_read = bread(vc_ptr->data.tcp.bfd, 
	&((char*)(&vc_ptr->pkt_car.msg_header.pkt))[vc_ptr->data.tcp.bytes_of_header_read],
	sizeof(MPID_Packet) - vc_ptr->data.tcp.bytes_of_header_read);
    if (num_read == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
#ifdef HAVE_WINSOCK2_H
	if (TCP_Process.error == WSAEWOULDBLOCK)
#else
	if ((TCP_Process.error == EINTR) || (TCP_Process.error == EAGAIN))
#endif
	    num_read = 0;
	else
	{
	    beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	    err_printf("tcp_read_header: bread failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	    MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_HEADER);
	    return -1;
	}
    }

    vc_ptr->data.tcp.bytes_of_header_read += num_read;

    if (vc_ptr->data.tcp.bytes_of_header_read == sizeof(MPID_Packet))
    {
#ifdef MPICH_DEV_BUILD
     /* set the function pointer to INVALID_POINTER to catch the potential error
        of calling this function again before the pointer is reset. */
	vc_ptr->data.tcp.read = INVALID_POINTER;
#endif
	mm_cq_enqueue(&vc_ptr->pkt_car);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_HEADER);
    return MPI_SUCCESS;
}

int tcp_read_data(MPIDI_VC *vc_ptr)
{
    MM_Car *car_ptr;
    MM_Segment_buffer *buf_ptr;
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_TCP_READ_DATA);

    MPIDI_FUNC_ENTER(MPID_STATE_TCP_READ_DATA);

#ifdef MPICH_DEV_BUILD
    if (vc_ptr->data.tcp.connecting)
    {
	err_printf("Error: tcp_read_data called on connecting vc\n");
	ret_val = tcp_read_connecting(vc_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_DATA);
	return ret_val;
    }
#endif

    if (vc_ptr->readq_head == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_DATA);
	return MPI_SUCCESS;
    }

    car_ptr = vc_ptr->readq_head;
    buf_ptr = car_ptr->buf_ptr;

    switch (buf_ptr->type)
    {
    case MM_VEC_BUFFER:
	ret_val = tcp_read_vec(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_DATA);
	return ret_val;
	break;
    case MM_SIMPLE_BUFFER:
	ret_val = tcp_read_simple(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_DATA);
	return ret_val;
	break;
    case MM_TMP_BUFFER:
	ret_val = tcp_read_tmp(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_DATA);
	return ret_val;
	break;
#ifdef WITH_METHOD_SHM
    case MM_SHM_BUFFER:
	ret_val = tcp_read_shm(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA
    case MM_VIA_BUFFER:
	ret_val = tcp_read_via(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    case MM_VIA_RDMA_BUFFER:
	ret_val = tcp_read_via_rdma(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_IB
    case MM_IB_BUFFER:
	ret_val = tcp_read_ib(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD_BUFFER:
	ret_val = tcp_read_new(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_DATA);
	return ret_val;
	break;
#endif
    case MM_NULL_BUFFER:
	err_printf("Error: tcp_read_data called on a null buffer\n");
	break;
    default:
	err_printf("Error: tcp_read_data: unknown or unsupported buffer type: %d\n", buf_ptr->type);
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_DATA);
    return -1;
}

#ifdef WITH_METHOD_SHM
int tcp_read_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_TCP_READ_SHM);
    MPIDI_FUNC_ENTER(MPID_STATE_TCP_READ_SHM);
    MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_SHM);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA
int tcp_read_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_TCP_READ_VIA);
    MPIDI_FUNC_ENTER(MPID_STATE_TCP_READ_VIA);
    MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_VIA);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA_RDMA
int tcp_read_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_TCP_READ_VIA_RDMA);
    MPIDI_FUNC_ENTER(MPID_STATE_TCP_READ_VIA_RDMA);
    MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_VIA_RDMA);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_IB
int tcp_read_ib(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_TCP_READ_IB);
    MPIDI_FUNC_ENTER(MPID_STATE_TCP_READ_IB);
    MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_IB);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_NEW
int tcp_read_new(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_TCP_READ_NEW);
    MPIDI_FUNC_ENTER(MPID_STATE_TCP_READ_NEW);
    MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_NEW);
    return MPI_SUCCESS;
}
#endif

int tcp_read_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    int num_read;
    int num_left, i;
    MPIDI_STATE_DECL(MPID_STATE_TCP_READ_VEC);

    MPIDI_FUNC_ENTER(MPID_STATE_TCP_READ_VEC);

    if (buf_ptr->vec.num_cars_outstanding == 0)
    {
	/* get more buffers */
	car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);
	/* reset the progress structures in the car */
	car_ptr->data.tcp.buf.vec_write.cur_index = 0;
	car_ptr->data.tcp.buf.vec_write.num_read_copy = 0;
	car_ptr->data.tcp.buf.vec_write.cur_num_written = 0;
	car_ptr->data.tcp.buf.vec_write.num_written_at_cur_index = 0;
	car_ptr->data.tcp.buf.vec_write.vec_size = 0;
	/* copy the vector from the buffer to the car */
	memcpy(car_ptr->data.tcp.buf.vec_read.vec,
	    buf_ptr->vec.vec,
	    buf_ptr->vec.vec_size * sizeof(MPID_IOV));
	car_ptr->data.tcp.buf.vec_read.vec_size = buf_ptr->vec.vec_size;
	buf_ptr->vec.num_read = 0;
	/* reset the number of outstanding write cars */
	buf_ptr->vec.num_cars_outstanding = buf_ptr->vec.num_cars;
    }
    
    if (car_ptr->data.tcp.buf.vec_read.cur_num_read < buf_ptr->vec.buf_size)
    {
	/* read */
	if (car_ptr->data.tcp.buf.vec_read.vec_size == 1) /* optimization for single buffer reads */
	{
	    num_read = bread(vc_ptr->data.tcp.bfd,
		car_ptr->data.tcp.buf.vec_read.vec[car_ptr->data.tcp.buf.vec_read.cur_index].MPID_IOV_BUF,
		car_ptr->data.tcp.buf.vec_read.vec[car_ptr->data.tcp.buf.vec_read.cur_index].MPID_IOV_LEN);
	    if (num_read == SOCKET_ERROR)
	    {
		TCP_Process.error = beasy_getlasterror();
#ifdef HAVE_WINSOCK2_H
		if (TCP_Process.error == WSAEWOULDBLOCK)
#else
		if ((TCP_Process.error == EINTR) || (TCP_Process.error == EAGAIN))
#endif
		    num_read = 0;
		else
		{
		    beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
		    err_printf("tcp_read_vec: bread failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
		    MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_VEC);
		    return -1;
		}
	    }
	}
	else
	{
	    num_read = breadv(vc_ptr->data.tcp.bfd,
		&car_ptr->data.tcp.buf.vec_read.vec[car_ptr->data.tcp.buf.vec_read.cur_index],
		car_ptr->data.tcp.buf.vec_read.vec_size);
	    if (num_read == SOCKET_ERROR)
	    {
		TCP_Process.error = beasy_getlasterror();
		beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
		err_printf("tcp_read_vec: breadv failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
		MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_VEC);
		return -1;
	    }
	}

	/*msg_printf("num_read vec: %d\n", num_read);*/
	
	/* update vectors */
	buf_ptr->vec.num_read += num_read;
	car_ptr->data.tcp.buf.vec_read.cur_num_read += num_read;
	car_ptr->data.tcp.buf.vec_read.total_num_read += num_read;
	if (car_ptr->data.tcp.buf.vec_read.cur_num_read == buf_ptr->vec.buf_size)
	{
	    /* reset the car */
	    car_ptr->data.tcp.buf.vec_read.cur_index = 0;
	    car_ptr->data.tcp.buf.vec_read.cur_num_read = 0;
	    car_ptr->data.tcp.buf.vec_read.vec_size = 0;
	}
	else
	{
	    num_left = num_read;
	    i = car_ptr->data.tcp.buf.vec_read.cur_index;
	    while (num_left > 0)
	    {
		num_left -= car_ptr->data.tcp.buf.vec_read.vec[i].MPID_IOV_LEN;
		if (num_left > 0)
		{
		    i++;
		}
		else
		{
		    car_ptr->data.tcp.buf.vec_read.vec[i].MPID_IOV_BUF = 
			(char*)(car_ptr->data.tcp.buf.vec_read.vec[i].MPID_IOV_BUF) +
			car_ptr->data.tcp.buf.vec_read.vec[i].MPID_IOV_LEN +
			num_left;
		    car_ptr->data.tcp.buf.vec_read.vec[i].MPID_IOV_LEN = -num_left;
		}
	    }
	    car_ptr->data.tcp.buf.vec_read.cur_index = i;
	}
    }
    
    if (car_ptr->data.tcp.buf.vec_read.total_num_read == buf_ptr->vec.segment_last)
    {
	tcp_car_dequeue(vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_VEC);
    return MPI_SUCCESS;
}

int tcp_read_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    int num_read;
    MPIDI_STATE_DECL(MPID_STATE_TCP_READ_TMP);

    MPIDI_FUNC_ENTER(MPID_STATE_TCP_READ_TMP);

    if (buf_ptr->tmp.buf == NULL)
    {
	/* get the tmp buffer */
	car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);
    }

    /* read as much as possible */
    num_read = bread(vc_ptr->data.tcp.bfd, 
	(char*)(buf_ptr->tmp.buf) + buf_ptr->tmp.num_read, 
	buf_ptr->tmp.len - buf_ptr->tmp.num_read);
    if (num_read == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
#ifdef HAVE_WINSOCK2_H
	if (TCP_Process.error == WSAEWOULDBLOCK)
#else
	if ((TCP_Process.error == EINTR) || (TCP_Process.error == EAGAIN))
#endif
	    num_read = 0;
	else
	{
	    err_printf("tcp_read_tmp:bread failed, error %d\n", beasy_getlasterror());
	}
    }

    /*msg_printf("num_read tmp: %d\n", num_read);*/

    /* update the amount read */
    buf_ptr->tmp.num_read += num_read;

    /* check to see if finished */
    if (buf_ptr->tmp.num_read == buf_ptr->tmp.len)
    {
	dbg_printf("num_read: %d\n", buf_ptr->tmp.num_read);
	/* remove from read queue and insert in completion queue */
	tcp_car_dequeue(vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_TMP);
    return MPI_SUCCESS;
}

int tcp_read_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    int num_read;
    MPIDI_STATE_DECL(MPID_STATE_TCP_READ_SIMPLE);

    MPIDI_FUNC_ENTER(MPID_STATE_TCP_READ_SIMPLE);

    if (buf_ptr->simple.buf == NULL)
    {
	/* get the simple buffer */
	/*car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);*/
	err_printf("Error: tcp_read_simple called with NULL simple buffer.\n");
	return -1;
    }

    /* read as much as possible */
    num_read = bread(vc_ptr->data.tcp.bfd, 
	(char*)(buf_ptr->simple.buf) + buf_ptr->simple.num_read, 
	buf_ptr->simple.len - buf_ptr->simple.num_read);
    if (num_read == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
#ifdef HAVE_WINSOCK2_H
	if (TCP_Process.error == WSAEWOULDBLOCK)
#else
	if ((TCP_Process.error == EINTR) || (TCP_Process.error == EAGAIN))
#endif
	    num_read = 0;
	else
	{
	    err_printf("tcp_read_tmp:bread failed, error %d\n", beasy_getlasterror());
	}
    }

    /*msg_printf("num_read simple: %d\n", num_read);*/

    /* update the amount read */
    buf_ptr->simple.num_read += num_read;

    /* check to see if finished */
    if (buf_ptr->simple.num_read == buf_ptr->simple.len)
    {
	dbg_printf("num_read: %d\n", buf_ptr->simple.num_read);
	/* remove from read queue and insert in completion queue */
	tcp_car_dequeue(vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_SIMPLE);
    return MPI_SUCCESS;
}

int tcp_read_connecting(MPIDI_VC *vc_ptr)
{
    char ack;
    MPIDI_STATE_DECL(MPID_STATE_TCP_READ_CONNECTING);
    
    MPIDI_FUNC_ENTER(MPID_STATE_TCP_READ_CONNECTING);

    if (beasy_receive(vc_ptr->data.tcp.bfd, &ack, 1) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_read_connecting: beasy_receive(ack) failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_CONNECTING);
	return -1;
    }
    if (ack == TCP_ACCEPT_CONNECTION)
    {
	vc_ptr->data.tcp.connected = TRUE;
	vc_ptr->data.tcp.connecting = FALSE;

	bmake_nonblocking(vc_ptr->data.tcp.bfd);
    }
    else if (ack == TCP_REJECT_CONNECTION)
    {
	vc_ptr->data.tcp.reject_received = TRUE;
    }
    else
    {
	err_printf("tcp_read_connecting: unknown ack char #%d received in read function.\n", (int)ack);
    }

    vc_ptr->data.tcp.read = tcp_read_header;
    vc_ptr->data.tcp.bytes_of_header_read = 0;

    MPIDI_FUNC_EXIT(MPID_STATE_TCP_READ_CONNECTING);
    return MPI_SUCCESS;
}

#endif
