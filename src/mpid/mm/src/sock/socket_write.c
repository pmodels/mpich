/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "socketimpl.h"

#ifdef WITH_METHOD_SHM
int socket_write_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
#ifdef WITH_METHOD_VIA
int socket_write_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
#ifdef WITH_METHOD_VIA_RDMA
int socket_write_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
int socket_write_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int socket_write_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int socket_write_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);

int socket_write(MPIDI_VC *vc_ptr)
{
    MM_Car *car_ptr;
    MM_Segment_buffer *buf_ptr;
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_WRITE);

    if (!vc_ptr->data.socket.connected)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE);
	return MPI_SUCCESS;
    }

    if (vc_ptr->writeq_head == NULL)
    {
	msg_printf("socket_write: write called with no vc's in the write queue.\n");
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE);
	return MPI_SUCCESS;
    }

    car_ptr = vc_ptr->writeq_head;
    buf_ptr = car_ptr->buf_ptr;

    switch (buf_ptr->type)
    {
#ifdef WITH_METHOD_SHM
    case MM_SHM_BUFFER:
	ret_val = socket_write_shm(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA
    case MM_VIA_BUFFER:
	ret_val = socket_write_via(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    case MM_VIA_RDMA_BUFFER:
	ret_val = socket_write_via_rdma(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE);
	return ret_val;
	break;
#endif
    case MM_VEC_BUFFER:
	ret_val = (buf_ptr->vec.num_cars_outstanding > 0) ? socket_write_vec(vc_ptr, car_ptr, buf_ptr) : MPI_SUCCESS;
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE);
	return ret_val;
	break;
    case MM_SIMPLE_BUFFER:
	ret_val = socket_write_simple(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE);
	return ret_val;
	break;
    case MM_TMP_BUFFER:
	ret_val = socket_write_tmp(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE);
	return ret_val;
	break;
#ifdef WITH_METHOD_IB
    case MM_IB_BUFFER:
	ret_val = socket_write_ib(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD_BUFFER:
	ret_val = socket_write_new(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE);
	return ret_val;
	break;
#endif
    case MM_NULL_BUFFER:
	err_printf("Error: socket_write called on a null buffer\n");
	break;
    default:
	err_printf("Error: socket_write: unknown or unsupported buffer type: %d\n", buf_ptr->type);
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE);
    return -1;
}

#ifdef WITH_METHOD_SHM
int socket_write_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_WRITE_SHM);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_WRITE_SHM);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE_SHM);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA
int socket_write_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_WRITE_VIA);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_WRITE_VIA);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE_VIA);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA_RDMA
int socket_write_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_WRITE_VIA_RDMA);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_WRITE_VIA_RDMA);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE_VIA_RDMA);
    return MPI_SUCCESS;
}
#endif

int socket_write_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    int error;
    int num_written;
    int cur_index;
    MPID_IOV *car_vec, *buf_vec;
    int num_left, i;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_WRITE_VEC);
    
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_WRITE_VEC);

#ifdef MPICH_DEV_BUILD
    /* this function assumes that buf_ptr->vec.num_cars_outstanding > 0 */
    if (buf_ptr->vec.num_cars_outstanding == 0)
    {
	err_printf("Error: socket_write_vec called when num_cars_outstanding == 0.\n");
	return -1;
    }
#endif

    /* num_read_copy is the amount of data described by the writer's vector in car_ptr->data.socket.buf.vec_write */
    /* num_read is the amount of data described by the reader's vector in buf_ptr->vec */
    /* If they are not equal then the writer's car needs to be updated */
    if (car_ptr->data.socket.buf.vec_write.num_read_copy != buf_ptr->vec.num_read)
    {
	/* update vector */
	cur_index = car_ptr->data.socket.buf.vec_write.cur_index;
	car_vec = car_ptr->data.socket.buf.vec_write.vec;
	buf_vec = buf_ptr->vec.vec;
	
	/* update num_read_copy */
	car_ptr->data.socket.buf.vec_write.num_read_copy = buf_ptr->vec.num_read;
	
	/* copy the buf vector into the car vector from the current index to the end */
	memcpy(&car_vec[cur_index], &buf_vec[cur_index], 
	    (buf_ptr->vec.vec_size - cur_index) * sizeof(MPID_IOV));
	car_vec[cur_index].MPID_IOV_BUF = 
	    (char*)car_vec[cur_index].MPID_IOV_BUF + car_ptr->data.socket.buf.vec_write.num_written_at_cur_index;
	car_vec[cur_index].MPID_IOV_LEN = car_vec[cur_index].MPID_IOV_LEN - car_ptr->data.socket.buf.vec_write.num_written_at_cur_index;

	/* modify the vector copied from buf_ptr->vec to represent only the data that has been read 
	 * This is done by traversing the vector, subtracting the lengths of each buffer until all the read
	 * data is accounted for.
	 */

	/* set the size of the car vector to zero */
	car_ptr->data.socket.buf.vec_write.vec_size = 0;
	
	/* add vector elements to the size until all the read data is accounted for */
	num_left = car_ptr->data.socket.buf.vec_write.num_read_copy - car_ptr->data.socket.buf.vec_write.cur_num_written;
	i = cur_index;
	while (num_left > 0)
	{
	    car_ptr->data.socket.buf.vec_write.vec_size++;
	    num_left -= car_vec[i].MPID_IOV_LEN;
	    i++;
	}
	/* if the last vector buffer is larger than the amount of data read into that buffer,
	update the length field in the car's copy of the vector to represent only the read data */
	if (num_left < 0)
	{
	    car_vec[i].MPID_IOV_LEN += num_left;
	}
	
	/* at this point the vec in the writer's car describes all the currently read data */
    }

    /* num_read_copy is the amount of data described by the writer's vector in car_ptr->data.socket.buf.vec_write */
    /* cur_num_written is the amount of data in this car that has already been written */
    /* If they are not equal then there is data available to be written */
    if (car_ptr->data.socket.buf.vec_write.cur_num_written < car_ptr->data.socket.buf.vec_write.num_read_copy)
    {
	/* write */
	if (car_ptr->data.socket.buf.vec_write.vec_size == 1) /* optimization for single buffer writes */
	{
	    /*num_written = bwrite(vc_ptr->data.socket.bfd, 
		car_ptr->data.socket.buf.vec_write.vec[car_ptr->data.socket.buf.vec_write.cur_index].MPID_IOV_BUF,
		car_ptr->data.socket.buf.vec_write.vec[car_ptr->data.socket.buf.vec_write.cur_index].MPID_IOV_LEN);
	    if (num_written == SOCKET_ERROR)
		*/
	    if ((error = sock_write(vc_ptr->data.socket.sock,
		car_ptr->data.socket.buf.vec_write.vec[car_ptr->data.socket.buf.vec_write.cur_index].MPID_IOV_BUF,
		car_ptr->data.socket.buf.vec_write.vec[car_ptr->data.socket.buf.vec_write.cur_index].MPID_IOV_LEN,
		&num_written)) != SOCK_SUCCESS)
	    {
		socket_print_sock_error(error, "socket_write: sock_write failed");
		MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE_VEC);
		return -1;
	    }
	}
	else
	{
	    /*num_written = bwritev(
		vc_ptr->data.socket.bfd, 
		&car_ptr->data.socket.buf.vec_write.vec[car_ptr->data.socket.buf.vec_write.cur_index], 
		car_ptr->data.socket.buf.vec_write.vec_size);
	    if (num_written == SOCKET_ERROR)
		*/
	    if ((error = sock_writev(
		vc_ptr->data.socket.sock,
		&car_ptr->data.socket.buf.vec_write.vec[car_ptr->data.socket.buf.vec_write.cur_index], 
		car_ptr->data.socket.buf.vec_write.vec_size, &num_written)) != SOCK_SUCCESS)
	    {
		socket_print_sock_error(error, "socket_write: sock_writev failed");
		MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE_VEC);
		return -1;
	    }
	}

	/* update vector */
	car_ptr->data.socket.buf.vec_write.cur_num_written += num_written;
	car_ptr->data.socket.buf.vec_write.total_num_written += num_written;
	if (car_ptr->data.socket.buf.vec_write.cur_num_written == buf_ptr->vec.buf_size)
	{
	    /* reset this car */
	    car_ptr->data.socket.buf.vec_write.cur_index = 0;
	    car_ptr->data.socket.buf.vec_write.num_read_copy = 0;
	    car_ptr->data.socket.buf.vec_write.cur_num_written = 0;
	    car_ptr->data.socket.buf.vec_write.num_written_at_cur_index = 0;
	    car_ptr->data.socket.buf.vec_write.vec_size = 0;
	    /* signal that we have finished writing the current vector */
	    mm_dec_atomic(&(buf_ptr->vec.num_cars_outstanding));
	}
	else
	{
	    num_left = num_written;
	    i = car_ptr->data.socket.buf.vec_write.cur_index;
	    while (num_left > 0)
	    {
		num_left -= car_ptr->data.socket.buf.vec_write.vec[i].MPID_IOV_LEN;
		if (num_left > 0)
		{
		    i++;
		}
		else
		{
		    car_ptr->data.socket.buf.vec_write.vec[i].MPID_IOV_BUF = 
			car_ptr->data.socket.buf.vec_write.vec[i].MPID_IOV_BUF +
			car_ptr->data.socket.buf.vec_write.vec[i].MPID_IOV_LEN +
			num_left;
		    car_ptr->data.socket.buf.vec_write.num_written_at_cur_index = 
			car_ptr->data.socket.buf.vec_write.vec[i].MPID_IOV_LEN + num_left;
		    car_ptr->data.socket.buf.vec_write.vec[i].MPID_IOV_LEN = -num_left;
		}
	    }
	    car_ptr->data.socket.buf.vec_write.cur_index = i;
	}
    }
    
    /* if the entire mpi segment has been written, enqueue the car in the completion queue */
    if (car_ptr->data.socket.buf.vec_write.total_num_written == buf_ptr->vec.segment_last)
    {
#ifdef MPICH_DEV_BUILD
	if (car_ptr != car_ptr->vc_ptr->writeq_head)
	{
	    err_printf("Error: socket_write_vec not dequeueing the head write car.\n");
	}
#endif
	socket_car_dequeue_write(car_ptr->vc_ptr);
	car_ptr->next_ptr = NULL; /* prevent the next car from being enqueued by mm_cq_handle_write_car() */
	mm_cq_enqueue(car_ptr);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE_VEC);
    return MPI_SUCCESS;
}

int socket_write_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    int error;
    int num_written;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_WRITE_TMP);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_WRITE_TMP);

    if ((car_ptr->data.socket.buf.tmp.num_written == buf_ptr->tmp.num_read) || (buf_ptr->tmp.num_read == 0))
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE_TMP);
	return MPI_SUCCESS;
    }

    /* write as much as possible */
    /*num_written = bwrite(vc_ptr->data.socket.bfd, 
	(char*)(buf_ptr->tmp.buf) + car_ptr->data.socket.buf.tmp.num_written,
	buf_ptr->tmp.num_read - car_ptr->data.socket.buf.tmp.num_written);
    if (num_written == SOCKET_ERROR)
	*/
    if ((error = sock_write(vc_ptr->data.socket.sock, 
	(char*)(buf_ptr->tmp.buf) + car_ptr->data.socket.buf.tmp.num_written,
	buf_ptr->tmp.num_read - car_ptr->data.socket.buf.tmp.num_written, &num_written)) != SOCK_SUCCESS)
    {
	socket_print_sock_error(error, "socket_write_tmp: sock_write failed");
    }
    /* update the amount written */
    car_ptr->data.socket.buf.tmp.num_written += num_written;

    /* check to see if finished */
    if (car_ptr->data.socket.buf.tmp.num_written == buf_ptr->tmp.len)
    {
	dbg_printf("num_written: %d\n", car_ptr->data.socket.buf.tmp.num_written);
	/* remove from write queue and insert in completion queue */
#ifdef MPICH_DEV_BUILD
	if (car_ptr != vc_ptr->writeq_head)
	{
	    err_printf("Error: socket_write_tmp not dequeueing the head write car.\n");
	}
#endif
	socket_car_dequeue_write(vc_ptr);
	car_ptr->next_ptr = NULL; /* prevent the next car from being enqueued by mm_cq_handle_write_car() */
	mm_cq_enqueue(car_ptr);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE_TMP);
    return MPI_SUCCESS;
}

int socket_write_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    int error;
    int num_written;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_WRITE_SIMPLE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_WRITE_SIMPLE);

    if ((car_ptr->data.socket.buf.simple.num_written == buf_ptr->simple.num_read) || (buf_ptr->simple.num_read == 0))
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE_SIMPLE);
	return MPI_SUCCESS;
    }

    /* write as much as possible */
    /*
    num_written = bwrite(vc_ptr->data.socket.bfd, 
	(char*)(buf_ptr->simple.buf) + car_ptr->data.socket.buf.simple.num_written,
	buf_ptr->simple.num_read - car_ptr->data.socket.buf.simple.num_written);
    if (num_written == SOCKET_ERROR)
	*/
    if ((error = sock_write(vc_ptr->data.socket.sock, 
	(char*)(buf_ptr->simple.buf) + car_ptr->data.socket.buf.simple.num_written,
	buf_ptr->simple.num_read - car_ptr->data.socket.buf.simple.num_written, &num_written)) != SOCK_SUCCESS)
    {
	socket_print_sock_error(error, "socket_write_tmp: sock_write failed.");
    }
    /* update the amount written */
    car_ptr->data.socket.buf.simple.num_written += num_written;

    /* check to see if finished */
    if (car_ptr->data.socket.buf.simple.num_written == buf_ptr->simple.len)
    {
	dbg_printf("num_written: %d\n", car_ptr->data.socket.buf.simple.num_written);
	/* remove from write queue and insert in completion queue */
#ifdef MPICH_DEV_BUILD
	if (car_ptr != vc_ptr->writeq_head)
	{
	    err_printf("Error: socket_write_tmp not dequeueing the head write car.\n");
	}
#endif
	socket_car_dequeue_write(vc_ptr);
	car_ptr->next_ptr = NULL; /* prevent the next car from being enqueued by mm_cq_handle_write_car() */
	mm_cq_enqueue(car_ptr);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_WRITE_SIMPLE);
    return MPI_SUCCESS;
}
