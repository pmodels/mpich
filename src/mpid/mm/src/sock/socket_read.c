/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "socketimpl.h"

#ifdef WITH_METHOD_SOCKET

#ifndef HAVE_WINSOCK2_H
#include <errno.h>
#endif

#ifdef WITH_METHOD_SHM
int socket_read_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int socket_handle_read_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, int num_read);
#endif
#ifdef WITH_METHOD_VIA
int socket_read_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int socket_handle_read_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, int num_read);
#endif
#ifdef WITH_METHOD_VIA_RDMA
int socket_read_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int socket_handle_read_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, int num_read);
#endif
int socket_read_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int socket_handle_read_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, int num_read);
int socket_read_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int socket_handle_read_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, int num_read);
int socket_read_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int socket_handle_read_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, int num_read);
#ifdef WITH_METHOD_IB
int socket_read_ib(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int socket_handle_read_ib(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, int num_read);
#endif
#ifdef WITH_METHOD_NEW
int socket_read_new(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int socket_handle_read_new(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, int num_read);
#endif

#if 0
int socket_read_header(MPIDI_VC *vc_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_READ_HEADER);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_READ_HEADER);
#ifdef MPICH_DEV_BUILD
    if (!(vc_ptr->data.socket.state & SOCKET_CONNECTED))
    {
	err_printf("socket_read_header called on an unconnected vc\n");
    }
#endif

#ifdef MPICH_DEV_BUILD
    /* set the function pointer to INVALID_POINTER to catch the potential error
       of calling this function again before the pointer is reset. */
    /*vc_ptr->data.socket.read = INVALID_POINTER;*/
#endif
    mm_cq_enqueue(&vc_ptr->pkt_car);
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_HEADER);
    return MPI_SUCCESS;
}
#endif

int socket_read_data(MPIDI_VC *vc_ptr)
{
    MM_Car *car_ptr;
    MM_Segment_buffer *buf_ptr;
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_READ_DATA);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_READ_DATA);

#ifdef MPICH_DEV_BUILD
    if (!(vc_ptr->data.socket.state & SOCKET_CONNECTED))
    {
	err_printf("Error: socket_read_data called on connecting vc\n");
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_DATA);
	return -1;
    }
#endif

    if (vc_ptr->readq_head == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_DATA);
	return MPI_SUCCESS;
    }

    SOCKET_CLR_BIT(vc_ptr->data.socket.state, SOCKET_READING_HEADER);
    SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_READING_DATA);

    car_ptr = vc_ptr->readq_head;
    buf_ptr = car_ptr->buf_ptr;

    switch (buf_ptr->type)
    {
    case MM_VEC_BUFFER:
	ret_val = socket_read_vec(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_DATA);
	return ret_val;
	break;
    case MM_SIMPLE_BUFFER:
	ret_val = socket_read_simple(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_DATA);
	return ret_val;
	break;
    case MM_TMP_BUFFER:
	ret_val = socket_read_tmp(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_DATA);
	return ret_val;
	break;
#ifdef WITH_METHOD_SHM
    case MM_SHM_BUFFER:
	ret_val = socket_read_shm(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA
    case MM_VIA_BUFFER:
	ret_val = socket_read_via(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    case MM_VIA_RDMA_BUFFER:
	ret_val = socket_read_via_rdma(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_IB
    case MM_IB_BUFFER:
	ret_val = socket_read_ib(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD_BUFFER:
	ret_val = socket_read_new(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_DATA);
	return ret_val;
	break;
#endif
    case MM_NULL_BUFFER:
	err_printf("Error: socket_read_data called on a null buffer\n");
	break;
    default:
	err_printf("Error: socket_read_data: unknown or unsupported buffer type: %d\n", buf_ptr->type);
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_DATA);
    return -1;
}

int socket_handle_read_data(MPIDI_VC *vc_ptr, int num_read)
{
    MM_Car *car_ptr;
    MM_Segment_buffer *buf_ptr;
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_READ_DATA);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_READ_DATA);

#ifdef MPICH_DEV_BUILD
    if (!(vc_ptr->data.socket.state & SOCKET_CONNECTED))
    {
	err_printf("Error: socket_handle_read_data called on connecting vc\n");
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_DATA);
	return -1;
    }
#endif

    if (vc_ptr->readq_head == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_DATA);
	return MPI_SUCCESS;
    }

    car_ptr = vc_ptr->readq_head;
    buf_ptr = car_ptr->buf_ptr;

    switch (buf_ptr->type)
    {
    case MM_VEC_BUFFER:
	ret_val = socket_handle_read_vec(vc_ptr, car_ptr, buf_ptr, num_read);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_DATA);
	return ret_val;
	break;
    case MM_SIMPLE_BUFFER:
	ret_val = socket_handle_read_simple(vc_ptr, car_ptr, buf_ptr, num_read);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_DATA);
	return ret_val;
	break;
    case MM_TMP_BUFFER:
	ret_val = socket_handle_read_tmp(vc_ptr, car_ptr, buf_ptr, num_read);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_DATA);
	return ret_val;
	break;
#ifdef WITH_METHOD_SHM
    case MM_SHM_BUFFER:
	ret_val = socket_handle_read_shm(vc_ptr, car_ptr, buf_ptr, num_read);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA
    case MM_VIA_BUFFER:
	ret_val = socket_handle_read_via(vc_ptr, car_ptr, buf_ptr, num_read);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    case MM_VIA_RDMA_BUFFER:
	ret_val = socket_handle_read_via_rdma(vc_ptr, car_ptr, buf_ptr, num_read);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_IB
    case MM_IB_BUFFER:
	ret_val =socket_handle_read_ib(vc_ptr, car_ptr, buf_ptr, num_read);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD_BUFFER:
	ret_val = socket_handle_read_new(vc_ptr, car_ptr, buf_ptr, num_read);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_DATA);
	return ret_val;
	break;
#endif
    case MM_NULL_BUFFER:
	err_printf("Error: socket_read_data called on a null buffer\n");
	break;
    default:
	err_printf("Error: socket_read_data: unknown or unsupported buffer type: %d\n", buf_ptr->type);
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_DATA);
    return -1;
}

#ifdef WITH_METHOD_SHM
int socket_read_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_READ_SHM);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_READ_SHM);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_SHM);
    return MPI_SUCCESS;
}

int socket_handle_read_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_READ_SHM);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_READ_SHM);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_SHM);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA
int socket_read_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_READ_VIA);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_READ_VIA);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_VIA);
    return MPI_SUCCESS;
}

int socket_handle_read_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_READ_VIA);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_READ_VIA);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_VIA);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA_RDMA
int socket_read_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_READ_VIA_RDMA);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_READ_VIA_RDMA);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_VIA_RDMA);
    return MPI_SUCCESS;
}

int socket_handle_read_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_READ_VIA_RDMA);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_READ_VIA_RDMA);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_VIA_RDMA);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_IB
int socket_read_ib(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_READ_IB);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_READ_IB);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_IB);
    return MPI_SUCCESS;
}
int socket_handle_read_ib(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_READ_IB);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_READ_IB);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_IB);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_NEW
int socket_read_new(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_READ_IB);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_READ_IB);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_IB);
    return MPI_SUCCESS;
}
int socket_handle_read_new(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_READ_IB);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_READ_IB);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_IB);
    return MPI_SUCCESS;
}
#endif

int socket_read_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    int error;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_READ_VEC);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_READ_VEC);

    if (buf_ptr->vec.num_cars_outstanding == 0)
    {
	/* get more buffers */
	car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);
	/* reset the progress structures in the car */
	car_ptr->data.socket.buf.vec_write.cur_index = 0;
	car_ptr->data.socket.buf.vec_write.num_read_copy = 0;
	car_ptr->data.socket.buf.vec_write.cur_num_written = 0;
	car_ptr->data.socket.buf.vec_write.num_written_at_cur_index = 0;
	car_ptr->data.socket.buf.vec_write.vec_size = 0;
	/* copy the vector from the buffer to the car */
	memcpy(car_ptr->data.socket.buf.vec_read.vec,
	    buf_ptr->vec.vec,
	    buf_ptr->vec.vec_size * sizeof(MPID_IOV));
	car_ptr->data.socket.buf.vec_read.vec_size = buf_ptr->vec.vec_size;
	buf_ptr->vec.num_read = 0;
	/* reset the number of outstanding write cars */
	buf_ptr->vec.num_cars_outstanding = buf_ptr->vec.num_cars;
    }
    
    if (car_ptr->data.socket.buf.vec_read.cur_num_read < buf_ptr->vec.buf_size)
    {
	/* read */
	if (car_ptr->data.socket.buf.vec_read.vec_size == 1) /* optimization for single buffer reads */
	{
	    if ((error = sock_post_read(vc_ptr->data.socket.sock,
		car_ptr->data.socket.buf.vec_read.vec[car_ptr->data.socket.buf.vec_read.cur_index].MPID_IOV_BUF,
		car_ptr->data.socket.buf.vec_read.vec[car_ptr->data.socket.buf.vec_read.cur_index].MPID_IOV_LEN, NULL)) != SOCK_SUCCESS)
	    {
		SOCKET_Process.error = error;
		socket_print_sock_error(error, "socket_read_vec: sock_read failed.");
		MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_VEC);
		return -1;
	    }
	}
	else
	{
	    if ((error = sock_post_readv(vc_ptr->data.socket.sock,
		&car_ptr->data.socket.buf.vec_read.vec[car_ptr->data.socket.buf.vec_read.cur_index],
		car_ptr->data.socket.buf.vec_read.vec_size, NULL)) != SOCK_SUCCESS)
	    {
		SOCKET_Process.error = error;
		socket_print_sock_error(error, "socket_read_vec: sock_readv failed.");
		MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_VEC);
		return -1;
	    }
	}

	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_VEC);
	return MPI_SUCCESS;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_VEC);
    return MPI_SUCCESS;
}

int socket_handle_read_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, int num_read)
{
    int num_left;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_READ_VEC);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_READ_VEC);

    /* update vectors */
    buf_ptr->vec.num_read += num_read;
    car_ptr->data.socket.buf.vec_read.cur_num_read += num_read;
    car_ptr->data.socket.buf.vec_read.total_num_read += num_read;
    if (car_ptr->data.socket.buf.vec_read.cur_num_read == buf_ptr->vec.buf_size)
    {
	/* reset the car */
	car_ptr->data.socket.buf.vec_read.cur_index = 0;
	car_ptr->data.socket.buf.vec_read.cur_num_read = 0;
	car_ptr->data.socket.buf.vec_read.vec_size = 0;
    }
    else
    {
	num_left = num_read;
	i = car_ptr->data.socket.buf.vec_read.cur_index;
	while (num_left > 0)
	{
	    num_left -= car_ptr->data.socket.buf.vec_read.vec[i].MPID_IOV_LEN;
	    if (num_left > 0)
	    {
		i++;
	    }
	    else
	    {
		car_ptr->data.socket.buf.vec_read.vec[i].MPID_IOV_BUF = 
		    (char*)(car_ptr->data.socket.buf.vec_read.vec[i].MPID_IOV_BUF) +
		    car_ptr->data.socket.buf.vec_read.vec[i].MPID_IOV_LEN +
		    num_left;
		car_ptr->data.socket.buf.vec_read.vec[i].MPID_IOV_LEN = -num_left;
	    }
	}
	car_ptr->data.socket.buf.vec_read.cur_index = i;
    }
    
    if (car_ptr->data.socket.buf.vec_read.total_num_read == buf_ptr->vec.segment_last)
    {
	socket_car_dequeue_read(vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
    }
    else
    {
	/* post read of the rest of the data*/
	socket_read_data(vc_ptr);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_VEC);
    return MPI_SUCCESS;
}

int socket_read_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    int error;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_READ_TMP);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_READ_TMP);

    if (buf_ptr->tmp.buf == NULL)
    {
	/* get the tmp buffer */
	car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);
    }

    if ((error = sock_post_read(vc_ptr->data.socket.sock, 
	(char*)(buf_ptr->tmp.buf) + buf_ptr->tmp.num_read, 
	buf_ptr->tmp.len - buf_ptr->tmp.num_read, NULL)) != SOCK_SUCCESS)
    {
	SOCKET_Process.error = error;
	socket_print_sock_error(error, "socket_read_tmp: sock_read failed.");
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_TMP);
	return -1;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_TMP);
    return MPI_SUCCESS;
}

int socket_handle_read_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_READ_TMP);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_READ_TMP);
    /*msg_printf("num_read tmp: %d\n", num_read);*/

    /* update the amount read */
    buf_ptr->tmp.num_read += num_read;

    /* check to see if finished */
    if (buf_ptr->tmp.num_read == buf_ptr->tmp.len)
    {
	dbg_printf("num_read: %d\n", buf_ptr->tmp.num_read);
	/* remove from read queue and insert in completion queue */
	socket_car_dequeue_read(vc_ptr, car_ptr);

	/* If I put it in the completion queue, the car will be freed.  This is bad because
	   it needs to stay in the unexpected queue */
	/*mm_cq_enqueue(car_ptr);*/

	/* So I put the completion queue logic here, minus the freeing of the car */
	socket_post_read_pkt(vc_ptr);
	mm_dec_cc_atomic(car_ptr->request_ptr);
    }
    else
    {
	/* post more read*/
	socket_read_data(vc_ptr);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_TMP);
    return MPI_SUCCESS;
}

int socket_read_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    int error;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_READ_SIMPLE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_READ_SIMPLE);

    if (buf_ptr->simple.buf == NULL)
    {
	/* get the simple buffer */
	/*car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);*/
	err_printf("Error: socket_read_simple called with NULL simple buffer.\n");
	return -1;
    }

    if ((error = sock_post_read(vc_ptr->data.socket.sock, 
	(char*)(buf_ptr->simple.buf) + buf_ptr->simple.num_read, 
	buf_ptr->simple.len - buf_ptr->simple.num_read, NULL)) != SOCK_SUCCESS)
    {
	SOCKET_Process.error = error;
	socket_print_sock_error(error, "socket_read_tmp: sock_read failed.");
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_SIMPLE);
	return -1;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_READ_SIMPLE);
    return MPI_SUCCESS;
}

int socket_handle_read_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_READ_SIMPLE);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_READ_SIMPLE);
    /*msg_printf("num_read simple: %d\n", num_read);*/

    /* update the amount read */
    buf_ptr->simple.num_read += num_read;

    /* check to see if finished */
    if (buf_ptr->simple.num_read == buf_ptr->simple.len)
    {
	dbg_printf("num_read: %d\n", buf_ptr->simple.num_read);
	/* remove from read queue and insert in completion queue */
	socket_car_dequeue_read(vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
    }
    else
    {
	/* post a read of the rest of the data*/
	socket_read_data(vc_ptr);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_SIMPLE);
    return MPI_SUCCESS;
}

#endif
