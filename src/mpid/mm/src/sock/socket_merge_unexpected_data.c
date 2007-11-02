/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "socketimpl.h"

#ifdef WITH_METHOD_SOCKET

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifdef WITH_METHOD_SHM
int socket_merge_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
#endif
#ifdef WITH_METHOD_VIA
int socket_merge_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
#endif
#ifdef WITH_METHOD_VIA_RDMA
int socket_merge_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
#endif
int socket_merge_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
int socket_merge_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
int socket_merge_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
#ifdef WITH_METHOD_IB
int socket_merge_ib(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
#endif
#ifdef WITH_METHOD_NEW
int socket_merge_new(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
#endif

int socket_merge_unexpected_data(MPIDI_VC *vc_ptr, MM_Car *car_ptr, char *buffer, int length)
{
    int ret_val;
    MM_Segment_buffer *buf_ptr;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_MERGE_UNEXPECTED_DATA);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_MERGE_UNEXPECTED_DATA);

    if (!(vc_ptr->data.socket.state & SOCKET_CONNECTED))
    {
	/*
	if (socket_read_connecting(vc_ptr) != MPI_SUCCESS)
	{
	    err_printf("Error:socket_merge_unexpected_data: socket_read_connecting failed.\n");
	}
	*/
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_UNEXPECTED_DATA);
	return 0;
    }

    assert(car_ptr->type & MM_READ_CAR);

    buf_ptr = car_ptr->buf_ptr;
    switch (buf_ptr->type)
    {
    case MM_VEC_BUFFER:
	ret_val = socket_merge_vec(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
    case MM_SIMPLE_BUFFER:
	ret_val = socket_merge_simple(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
    case MM_TMP_BUFFER:
	ret_val = socket_merge_tmp(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#ifdef WITH_METHOD_SHM
    case MM_SHM_BUFFER:
	ret_val = socket_merge_shm(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA
    case MM_VIA_BUFFER:
	ret_val = socket_merge_via(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    case MM_VIA_RDMA_BUFFER:
	ret_val = socket_merge_via_rdma(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_IB
    case MM_IB_BUFFER:
	ret_val = socket_merge_ib(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD_BUFFER:
	ret_val = socket_merge_new(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#endif
    case MM_NULL_BUFFER:
	err_printf("Error: socket_merge_unexpected_data called on a null MM_Segment_buffer\n");
	break;
    default:
	err_printf("Error: socket_merge_unexpected_data: unknown or unsupported buffer type: %d\n", car_ptr->buf_ptr->type);
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_UNEXPECTED_DATA);
    return length;
}

#ifdef WITH_METHOD_SHM
int socket_merge_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_MERGE_SHM);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_MERGE_SHM);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_SHM);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA
int socket_merge_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_MERGE_VIA);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_MERGE_VIA);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_VIA);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA_RDMA
int socket_merge_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_MERGE_VIA_RDMA);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_MERGE_VIA_RDMA);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_VIA_RDMA);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_IB
int socket_merge_ib(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_MERGE_IB);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_MERGE_IB);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_IB);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_NEW
int socket_merge_new(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_MERGE_NEW);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_MERGE_NEW);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_NEW);
    return MPI_SUCCESS;
}
#endif

int socket_merge_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    int num_read = 0;
    int num_left, i;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_MERGE_VEC);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_MERGE_VEC);

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
	num_read = min(length, buf_ptr->vec.segment_last); /* This is incorrect because the segment may not fit in the current vector */
	
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
		    car_ptr->data.socket.buf.vec_read.vec[i].MPID_IOV_LEN -= num_left;
		}
	    }
	    car_ptr->data.socket.buf.vec_read.cur_index = i;
	}
    }
    
    if (car_ptr->data.socket.buf.vec_read.total_num_read == buf_ptr->vec.segment_last)
    {
	socket_car_dequeue_read(vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
    }
    else
    {
	msg_printf("total_num_read %d, segment_last %d\n", car_ptr->data.socket.buf.vec_read.total_num_read, buf_ptr->vec.segment_last);
	/* somehow save the extra data because it must be completely read or it will be lost */
	err_printf("Error: socket_merge_vec: data lost.\n");
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_VEC);
    return num_read;
}

int socket_merge_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    int num_read;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_MERGE_TMP);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_MERGE_TMP);

    if (buf_ptr->tmp.buf == NULL)
    {
	/* get the tmp buffer */
	car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);
    }

    /* read as much as possible */
    num_read = min(length, buf_ptr->tmp.len);

    /* Aren't we supposed to do a memcpy here? */

    /* update the amount read */
    buf_ptr->tmp.num_read += num_read;

    /* check to see if finished */
    if (buf_ptr->tmp.num_read == buf_ptr->tmp.len)
    {
	dbg_printf("num_read: %d\n", buf_ptr->tmp.num_read);
	/* remove from read queue and insert in completion queue */
	socket_car_dequeue_read(vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_TMP);
    return MPI_SUCCESS;
}

int socket_merge_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    int num_read;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_MERGE_SIMPLE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_MERGE_SIMPLE);

    if (buf_ptr->simple.buf == NULL)
    {
	/* get the simple buffer */
	/*car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);*/
	err_printf("Error: socket_merge_simple called with NULL simple pointer\n");
	return -1;
    }

    /* read as much as possible */
    num_read = min(length, buf_ptr->simple.len);

    /* Aren't we supposed to do a memcpy here? */

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

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MERGE_SIMPLE);
    return MPI_SUCCESS;
}

#endif
