/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ibimpl.h"

#ifdef WITH_METHOD_IB

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

/* Handling read data */
#ifdef WITH_METHOD_SHM
int ib_handle_read_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, void *mem_ptr, int num_read);
#endif
#ifdef WITH_METHOD_VIA
int ib_handle_read_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, void *mem_ptr, int num_read);
#endif
#ifdef WITH_METHOD_VIA_RDMA
int ib_handle_read_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, void *mem_ptr, int num_read);
#endif
int ib_handle_read_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, void *mem_ptr, int num_read);
int ib_handle_read_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, void *mem_ptr, int num_read);
int ib_handle_read_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, void *mem_ptr, int num_read);
#ifdef WITH_METHOD_IB
int ib_handle_read_ib(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, void *mem_ptr, int num_read);
#endif
#ifdef WITH_METHOD_NEW
int ib_handle_read_new(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, void *mem_ptr, int num_read);
#endif

/* Actively reading data */
#ifdef WITH_METHOD_SHM
int ib_read_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
#ifdef WITH_METHOD_VIA
int ib_read_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
#ifdef WITH_METHOD_VIA_RDMA
int ib_read_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
int ib_read_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int ib_read_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int ib_read_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#ifdef WITH_METHOD_IB
int ib_read_ib(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
#ifdef WITH_METHOD_NEW
int ib_read_new(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif

int ib_read_data(MPIDI_VC *vc_ptr)
{
    MM_Car *car_ptr;
    MM_Segment_buffer *buf_ptr;
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_IB_READ_DATA);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_READ_DATA);

    if (vc_ptr->readq_head == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_DATA);
	return MPI_SUCCESS;
    }

    vc_ptr->data.ib.reading_header = FALSE;

    car_ptr = vc_ptr->readq_head;
    buf_ptr = car_ptr->buf_ptr;

    switch (buf_ptr->type)
    {
    case MM_VEC_BUFFER:
	ret_val = ib_read_vec(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_DATA);
	return ret_val;
	break;
    case MM_SIMPLE_BUFFER:
	ret_val = ib_read_simple(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_DATA);
	return ret_val;
	break;
    case MM_TMP_BUFFER:
	ret_val = ib_read_tmp(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_DATA);
	return ret_val;
	break;
#ifdef WITH_METHOD_SHM
    case MM_SHM_BUFFER:
	ret_val = ib_read_shm(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA
    case MM_VIA_BUFFER:
	ret_val = ib_read_via(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    case MM_VIA_RDMA_BUFFER:
	ret_val = ib_read_via_rdma(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_IB
    case MM_IB_BUFFER:
	ret_val = ib_read_ib(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD_BUFFER:
	ret_val = ib_read_new(vc_ptr, car_ptr, buf_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_DATA);
	return ret_val;
	break;
#endif
    case MM_NULL_BUFFER:
	err_printf("Error: ib_read_data called on a null buffer\n");
	break;
    default:
	err_printf("Error: ib_read_data: unknown or unsupported buffer type: %d\n", buf_ptr->type);
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_DATA);
    return -1;
}

int ib_handle_read_data(MPIDI_VC *vc_ptr, void * mem_ptr, int num_read)
{
    MM_Car *car_ptr;
    MM_Segment_buffer *buf_ptr;
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_READ_DATA);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_READ_DATA);

    if (vc_ptr->readq_head == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_DATA);
	return MPI_SUCCESS;
    }

    car_ptr = vc_ptr->readq_head;
    buf_ptr = car_ptr->buf_ptr;

    switch (buf_ptr->type)
    {
    case MM_VEC_BUFFER:
	ret_val = ib_handle_read_vec(vc_ptr, car_ptr, buf_ptr, mem_ptr, num_read);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_DATA);
	return ret_val;
	break;
    case MM_SIMPLE_BUFFER:
	ret_val = ib_handle_read_simple(vc_ptr, car_ptr, buf_ptr, mem_ptr, num_read);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_DATA);
	return ret_val;
	break;
    case MM_TMP_BUFFER:
	ret_val = ib_handle_read_tmp(vc_ptr, car_ptr, buf_ptr, mem_ptr, num_read);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_DATA);
	return ret_val;
	break;
#ifdef WITH_METHOD_SHM
    case MM_SHM_BUFFER:
	ret_val = ib_handle_read_shm(vc_ptr, car_ptr, buf_ptr, mem_ptr, num_read);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA
    case MM_VIA_BUFFER:
	ret_val = ib_handle_read_via(vc_ptr, car_ptr, buf_ptr, mem_ptr, num_read);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    case MM_VIA_RDMA_BUFFER:
	ret_val = ib_handle_read_via_rdma(vc_ptr, car_ptr, buf_ptr, mem_ptr, num_read);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_IB
    case MM_IB_BUFFER:
	ret_val =ib_handle_read_ib(vc_ptr, car_ptr, buf_ptr, mem_ptr, num_read);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD_BUFFER:
	ret_val = ib_handle_read_new(vc_ptr, car_ptr, buf_ptr, mem_ptr, num_read);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_DATA);
	return ret_val;
	break;
#endif
    case MM_NULL_BUFFER:
	err_printf("Error: ib_read_data called on a null buffer\n");
	break;
    default:
	err_printf("Error: ib_read_data: unknown or unsupported buffer type: %d\n", buf_ptr->type);
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_DATA);
    return -1;
}

#ifdef WITH_METHOD_SHM
int ib_read_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_READ_SHM);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_READ_SHM);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_SHM);
    return MPI_SUCCESS;
}

int ib_handle_read_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, void *mem_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_READ_SHM);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_READ_SHM);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_SHM);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA
int ib_read_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_READ_VIA);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_READ_VIA);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_VIA);
    return MPI_SUCCESS;
}

int ib_handle_read_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, void *mem_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_READ_VIA);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_READ_VIA);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_VIA);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA_RDMA
int ib_read_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_READ_VIA_RDMA);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_READ_VIA_RDMA);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_VIA_RDMA);
    return MPI_SUCCESS;
}

int ib_handle_read_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, void *mem_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_READ_VIA_RDMA);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_READ_VIA_RDMA);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_VIA_RDMA);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_IB
int ib_read_ib(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_READ_IB);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_READ_IB);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_IB);
    return MPI_SUCCESS;
}

int ib_handle_read_ib(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, void *mem_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_READ_IB);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_READ_IB);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_IB);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_NEW
int ib_read_new(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_READ_IB);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_READ_IB);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_IB);
    return MPI_SUCCESS;
}

int ib_handle_read_new(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, void *mem_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_READ_IB);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_READ_IB);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_IB);
    return MPI_SUCCESS;
}
#endif

int ib_read_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
#if 0
    int error;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IB_READ_VEC);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_READ_VEC);

    if (buf_ptr->vec.num_cars_outstanding == 0)
    {
	/* get more buffers */
	car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);
	/* reset the progress structures in the car */
	car_ptr->data.ib.buf.vec_write.cur_index = 0;
	car_ptr->data.ib.buf.vec_write.num_read_copy = 0;
	car_ptr->data.ib.buf.vec_write.cur_num_written = 0;
	car_ptr->data.ib.buf.vec_write.num_written_at_cur_index = 0;
	car_ptr->data.ib.buf.vec_write.vec_size = 0;
	/* copy the vector from the buffer to the car */
	memcpy(car_ptr->data.ib.buf.vec_read.vec,
	    buf_ptr->vec.vec,
	    buf_ptr->vec.vec_size * sizeof(MPID_IOV));
	car_ptr->data.ib.buf.vec_read.vec_size = buf_ptr->vec.vec_size;
	buf_ptr->vec.num_read = 0;
	/* reset the number of outstanding write cars */
	buf_ptr->vec.num_cars_outstanding = buf_ptr->vec.num_cars;
    }

#if 0
    if (car_ptr->data.ib.buf.vec_read.cur_num_read < buf_ptr->vec.buf_size)
    {
	/* read */
	if (car_ptr->data.ib.buf.vec_read.vec_size == 1) /* optimization for single buffer reads */
	{
	    if ((error = sock_post_read(vc_ptr->data.ib.sock,
		car_ptr->data.ib.buf.vec_read.vec[car_ptr->data.ib.buf.vec_read.cur_index].MPID_IOV_BUF,
		car_ptr->data.ib.buf.vec_read.vec[car_ptr->data.ib.buf.vec_read.cur_index].MPID_IOV_LEN, NULL)) != SOCK_SUCCESS)
	    {
		IB_Process.error = error;
		ib_print_sock_error(error, "ib_read_vec: sock_read failed.");
		MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_VEC);
		return -1;
	    }
	}
	else
	{
	    if ((error = sock_post_readv(vc_ptr->data.ib.sock,
		&car_ptr->data.ib.buf.vec_read.vec[car_ptr->data.ib.buf.vec_read.cur_index],
		car_ptr->data.ib.buf.vec_read.vec_size, NULL)) != SOCK_SUCCESS)
	    {
		IB_Process.error = error;
		ib_print_sock_error(error, "ib_read_vec: sock_readv failed.");
		MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_VEC);
		return -1;
	    }
	}

	MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_VEC);
	return MPI_SUCCESS;
    }
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_VEC);
    return MPI_SUCCESS;
}

int ib_handle_read_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, void *mem_ptr, int num_read)
{
    int num_left;
    int i, count;
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_READ_VEC);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_READ_VEC);

    MPIU_DBG_PRINTF(("ib_handle_read_vec: received %d bytes total\n", num_read));

    num_left = num_read;
    i = car_ptr->data.ib.buf.vec_read.cur_index;
    while (num_left > 0)
    {
	count = min(car_ptr->data.ib.buf.vec_read.vec[i].MPID_IOV_LEN, (unsigned int)num_left);
	MPIU_DBG_PRINTF(("ib_handle_read_vec: copying %d bytes from index %d\n", count, i));
	memcpy(car_ptr->data.ib.buf.vec_read.vec[i].MPID_IOV_BUF, mem_ptr, count);
	mem_ptr = (char*)mem_ptr + count;

	num_left -= car_ptr->data.ib.buf.vec_read.vec[i].MPID_IOV_LEN;
	if (num_left > 0)
	{
	    i++;
	}
	else
	{
	    car_ptr->data.ib.buf.vec_read.vec[i].MPID_IOV_BUF = 
		(char*)(car_ptr->data.ib.buf.vec_read.vec[i].MPID_IOV_BUF) +
		car_ptr->data.ib.buf.vec_read.vec[i].MPID_IOV_LEN +
		num_left;
	    car_ptr->data.ib.buf.vec_read.vec[i].MPID_IOV_LEN = -num_left;
	}
    }
    car_ptr->data.ib.buf.vec_read.cur_index = i;
    
    buf_ptr->vec.num_read += num_read;
    car_ptr->data.ib.buf.vec_read.cur_num_read += num_read;
    car_ptr->data.ib.buf.vec_read.total_num_read += num_read;
    if (car_ptr->data.ib.buf.vec_read.cur_num_read == buf_ptr->vec.buf_size)
    {
	/* reset the car */
	car_ptr->data.ib.buf.vec_read.cur_index = 0;
	car_ptr->data.ib.buf.vec_read.cur_num_read = 0;
	car_ptr->data.ib.buf.vec_read.vec_size = 0;
    }

    if (car_ptr->data.ib.buf.vec_read.total_num_read == buf_ptr->vec.segment_last)
    {
	ib_car_dequeue_read(vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
    }
    else
    {
	/* post read of the rest of the data*/
	/*ib_read_data(vc_ptr);*/
	/* How do I know if there are more reads posted or if I need to post them here? */
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_VEC);
    return MPI_SUCCESS;
}

int ib_read_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
#if 0
    int error;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IB_READ_TMP);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_READ_TMP);

    if (buf_ptr->tmp.buf == NULL)
    {
	/* get the tmp buffer */
	car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);
    }

#if 0
    if ((error = sock_post_read(vc_ptr->data.ib.sock, 
	(char*)(buf_ptr->tmp.buf) + buf_ptr->tmp.num_read, 
	buf_ptr->tmp.len - buf_ptr->tmp.num_read, NULL)) != SOCK_SUCCESS)
    {
	IB_Process.error = error;
	ib_print_sock_error(error, "ib_read_tmp: sock_read failed.");
	MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_TMP);
	return -1;
    }
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_TMP);
    return MPI_SUCCESS;
}

int ib_handle_read_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, void *mem_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_READ_TMP);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_READ_TMP);
    /*msg_printf("num_read tmp: %d\n", num_read);*/

    memcpy(buf_ptr->tmp.buf, mem_ptr, num_read);

    /* update the amount read */
    buf_ptr->tmp.num_read += num_read;

    /* check to see if finished */
    if (buf_ptr->tmp.num_read == buf_ptr->tmp.len)
    {
	dbg_printf("num_read: %d\n", buf_ptr->tmp.num_read);
	/* remove from read queue and insert in completion queue */
	ib_car_dequeue_read(vc_ptr, car_ptr);

	/* If I put it in the completion queue, the car will be freed.  This is bad because
	   it needs to stay in the unexpected queue */
	/*mm_cq_enqueue(car_ptr);*/

	/* So I put the completion queue logic here, minus the freeing of the car */
	ib_post_read_pkt(vc_ptr);
	mm_dec_cc_atomic(car_ptr->request_ptr);
    }
    else
    {
	/* post more read*/
	/*ib_read_data(vc_ptr);*/
	/* How do I know if there are more reads posted or if I need to post them here? */
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_TMP);
    return MPI_SUCCESS;
}

int ib_read_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
#if 0
    int error;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IB_READ_SIMPLE);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_READ_SIMPLE);

    if (buf_ptr->simple.buf == NULL)
    {
	/* get the simple buffer */
	/*car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);*/
	err_printf("Error: ib_read_simple called with NULL simple buffer.\n");
	return -1;
    }

#if 0
    if ((error = sock_post_read(vc_ptr->data.ib.sock, 
	(char*)(buf_ptr->simple.buf) + buf_ptr->simple.num_read, 
	buf_ptr->simple.len - buf_ptr->simple.num_read, NULL)) != SOCK_SUCCESS)
    {
	IB_Process.error = error;
	ib_print_sock_error(error, "ib_read_tmp: sock_read failed.");
	MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_SIMPLE);
	return -1;
    }
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_IB_READ_SIMPLE);
    return MPI_SUCCESS;
}

int ib_handle_read_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, void *mem_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_READ_SIMPLE);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_READ_SIMPLE);
    
    MPIU_DBG_PRINTF(("num_read simple: %d\n", num_read));

    memcpy(buf_ptr->simple.buf, mem_ptr, num_read);

    /* update the amount read */
    buf_ptr->simple.num_read += num_read;

    /* check to see if finished */
    if (buf_ptr->simple.num_read == buf_ptr->simple.len)
    {
	dbg_printf("num_read: %d\n", buf_ptr->simple.num_read);
	/* remove from read queue and insert in completion queue */
	ib_car_dequeue_read(vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
    }
    else
    {
	/* post a read of the rest of the data*/
	/*ib_read_data(vc_ptr);*/
	/* How do I know if there are more reads posted or if I need to post them here? */
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_SIMPLE);
    return MPI_SUCCESS;
}

#endif
