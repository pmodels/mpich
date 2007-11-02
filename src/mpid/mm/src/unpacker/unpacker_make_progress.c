/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidimpl.h"

#ifdef WITH_METHOD_SHM
int unpacker_write_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
#ifdef WITH_METHOD_VIA
int unpacker_write_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
#ifdef WITH_METHOD_VIA_RDMA
int unpacker_write_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
int unpacker_write_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int unpacker_write_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int unpacker_write_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#ifdef WITH_METHOD_IB
int unpacker_write_ib(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
#ifdef WITH_METHOD_NEW
int unpacker_write_new(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif

/*@
   unpacker_make_progress - make progress

   Notes:
@*/
int unpacker_make_progress()
{
    MM_Car *car_ptr, *car_next_ptr;
    MM_Segment_buffer *buf_ptr;
    MPIDI_VC *vc_ptr;
    MPIDI_STATE_DECL(MPID_STATE_UNPACKER_MAKE_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_UNPACKER_MAKE_PROGRESS);

    if (MPID_Process.unpacker_vc_ptr->writeq_head == NULL)
    {
	/* shortcut out if the queue is empty */
	MPIDI_FUNC_EXIT(MPID_STATE_UNPACKER_MAKE_PROGRESS);
	return MPI_SUCCESS;
    }

    vc_ptr = MPID_Process.unpacker_vc_ptr;
    car_ptr = MPID_Process.unpacker_vc_ptr->writeq_head;
    
    do
    {
	/* save the next car pointer because writing the current car_ptr may
	 * modify its next pointer */
	car_next_ptr = car_ptr->vcqnext_ptr;
	buf_ptr = car_ptr->buf_ptr;
	switch (buf_ptr->type)
	{
#ifdef WITH_METHOD_SHM
	case MM_SHM_BUFFER:
	    unpacker_write_shm(vc_ptr, car_ptr, buf_ptr);
	    break;
#endif
#ifdef WITH_METHOD_VIA
	case MM_VIA_BUFFER:
	    unpacker_write_via(vc_ptr, car_ptr, buf_ptr);
	    break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	case MM_VIA_RDMA_BUFFER:
	    unpacker_write_via_rdma(vc_ptr, car_ptr, buf_ptr);
	    break;
#endif
	case MM_VEC_BUFFER:
	    if (buf_ptr->vec.num_cars_outstanding > 0)
		unpacker_write_vec(vc_ptr, car_ptr, buf_ptr);
	    break;
	case MM_SIMPLE_BUFFER:
	    unpacker_write_simple(vc_ptr, car_ptr, buf_ptr);
	    break;
	case MM_TMP_BUFFER:
	    unpacker_write_tmp(vc_ptr, car_ptr, buf_ptr);
	    break;
#ifdef WITH_METHOD_IB
	case MM_IB_BUFFER:
	    unpacker_write_ib(vc_ptr, car_ptr, buf_ptr);
	    break;
#endif
#ifdef WITH_METHOD_NEW
	case MM_NEW_METHOD_BUFFER:
	    unpacker_write_new(vc_ptr, car_ptr, buf_ptr);
	    break;
#endif
	case MM_NULL_BUFFER:
	    err_printf("error, cannot unpack from a null buffer\n");
	    break;
	default:
	    err_printf("illegal buffer type: %d\n", car_ptr->request_ptr->mm.buf.type);
	    break;
	}
	car_ptr = car_next_ptr;
    } while (car_ptr);
    
    MPIDI_FUNC_EXIT(MPID_STATE_UNPACKER_MAKE_PROGRESS);
    return MPI_SUCCESS;
}

#ifdef WITH_METHOD_SHM
int unpacker_write_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_UNPACKER_WRITE_SHM);
    MPIDI_FUNC_ENTER(MPID_STATE_UNPACKER_WRITE_SHM);
    MPIDI_FUNC_EXIT(MPID_STATE_UNPACKER_WRITE_SHM);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA
int unpacker_write_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_UNPACKER_WRITE_VIA);
    MPIDI_FUNC_ENTER(MPID_STATE_UNPACKER_WRITE_VIA);
    MPIDI_FUNC_EXIT(MPID_STATE_UNPACKER_WRITE_VIA);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA_RDMA
int unpacker_write_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_UNPACKER_WRITE_VIA_RDMA);
    MPIDI_FUNC_ENTER(MPID_STATE_UNPACKER_WRITE_VIA_RDMA);
    MPIDI_FUNC_EXIT(MPID_STATE_UNPACKER_WRITE_VIA_RDMA);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_IB
int unpacker_write_ib(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_UNPACKER_WRITE_IB);
    MPIDI_FUNC_ENTER(MPID_STATE_UNPACKER_WRITE_IB);
    MPIDI_FUNC_EXIT(MPID_STATE_UNPACKER_WRITE_IB);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_NEW
int unpacker_write_new(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_UNPACKER_WRITE_NEW);
    MPIDI_FUNC_ENTER(MPID_STATE_UNPACKER_WRITE_NEW);
    MPIDI_FUNC_EXIT(MPID_STATE_UNPACKER_WRITE_NEW);
    return MPI_SUCCESS;
}
#endif

/*
 * unpacking a vector doesn't do much because the data has already been copied to its
 * final destination by the reader.  this function merely keeps track of when the read
 * has finished and claims to be finished also 
 */
int unpacker_write_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    int num_written;
    MPIDI_STATE_DECL(MPID_STATE_UNPACKER_WRITE_VEC);

    MPIDI_FUNC_ENTER(MPID_STATE_UNPACKER_WRITE_VEC);

#ifdef MPICH_DEV_BUILD
    /* this function assumes that buf_ptr->vec.num_cars_outstanding > 0 */
    if (buf_ptr->vec.num_cars_outstanding == 0)
    {
	err_printf("Error: unpacker_write_vec called while num_cars_outstanding == 0.\n");
    }
#endif

    /* if num_read_copy != num_read then some new data has been read */
    if (car_ptr->data.unpacker.buf.vec_write.num_read_copy != buf_ptr->vec.num_read)
    {
	/* update num_read_copy and num_written */
	num_written = buf_ptr->vec.num_read - car_ptr->data.unpacker.buf.vec_write.num_read_copy;
	car_ptr->data.unpacker.buf.vec_write.num_read_copy = buf_ptr->vec.num_read;

	/* update vector */
	car_ptr->data.unpacker.buf.vec_write.cur_num_written += num_written;
	car_ptr->data.unpacker.buf.vec_write.total_num_written += num_written;
	if (car_ptr->data.unpacker.buf.vec_write.cur_num_written == buf_ptr->vec.buf_size)
	{
	    /* reset this car */
	    car_ptr->data.unpacker.buf.vec_write.num_read_copy = 0;
	    car_ptr->data.unpacker.buf.vec_write.cur_num_written = 0;
	    /* signal that we have finished writing the current vector */
	    mm_dec_atomic(&(buf_ptr->vec.num_cars_outstanding));
	}
    }
    
    /* if the entire mpi segment has been written, enqueue the car in the completion queue */
    if (car_ptr->data.unpacker.buf.vec_write.total_num_written == buf_ptr->vec.segment_last)
    {
	unpacker_car_dequeue(car_ptr->vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_UNPACKER_WRITE_VEC);
    return MPI_SUCCESS;
}

int unpacker_write_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_UNPACKER_WRITE_TMP);
    MPIDI_FUNC_ENTER(MPID_STATE_UNPACKER_WRITE_TMP);

    if ((car_ptr->data.unpacker.buf.tmp.last == buf_ptr->tmp.num_read) || (buf_ptr->tmp.buf == NULL))
    {
	/* no new data available or
	 * no buffer provided by the reader */
	MPIDI_FUNC_EXIT(MPID_STATE_UNPACKER_WRITE_TMP);
	return MPI_SUCCESS;
    }
    /* set the last variable to the number of bytes read */
    car_ptr->data.unpacker.buf.tmp.last = buf_ptr->tmp.num_read;
    /* unpack the buffer */
    MPID_Segment_unpack(
	&car_ptr->request_ptr->mm.segment,
	car_ptr->data.unpacker.buf.tmp.first,
	&car_ptr->data.unpacker.buf.tmp.last,
	buf_ptr->tmp.buf
	);
    
    if (car_ptr->data.packer.last == car_ptr->request_ptr->mm.last)
    {
	/* the entire buffer is unpacked */
	unpacker_car_dequeue(car_ptr->vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_UNPACKER_WRITE_TMP);
	return MPI_SUCCESS;
    }

    /* There is more unpacking needed so update the first variable */
    /* The last variable will be updated the next time through this function */
    car_ptr->data.unpacker.buf.tmp.first = car_ptr->data.unpacker.buf.tmp.last;

    MPIDI_FUNC_EXIT(MPID_STATE_UNPACKER_WRITE_TMP);
    return MPI_SUCCESS;
}

int unpacker_write_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_UNPACKER_WRITE_SIMPLE);
    MPIDI_FUNC_ENTER(MPID_STATE_UNPACKER_WRITE_SIMPLE);

    if ((car_ptr->data.unpacker.buf.simple.last == buf_ptr->simple.num_read) || (buf_ptr->simple.buf == NULL))
    {
	/* no new data available or
	 * no buffer provided by the reader */
	MPIDI_FUNC_EXIT(MPID_STATE_UNPACKER_WRITE_SIMPLE);
	return MPI_SUCCESS;
    }
    /* set the last variable to the number of bytes read */
    car_ptr->data.unpacker.buf.simple.last = buf_ptr->simple.num_read;
    /* unpack the buffer */
    MPID_Segment_unpack(
	&car_ptr->request_ptr->mm.segment,
	car_ptr->data.unpacker.buf.simple.first,
	&car_ptr->data.unpacker.buf.simple.last,
	buf_ptr->simple.buf
	);
    
    if (car_ptr->data.packer.last == car_ptr->request_ptr->mm.last)
    {
	/* the entire buffer is unpacked */
	unpacker_car_dequeue(car_ptr->vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_UNPACKER_WRITE_SIMPLE);
	return MPI_SUCCESS;
    }

    /* There is more unpacking needed so update the first variable */
    /* The last variable will be updated the next time through this function */
    car_ptr->data.unpacker.buf.simple.first = car_ptr->data.unpacker.buf.simple.last;

    MPIDI_FUNC_EXIT(MPID_STATE_UNPACKER_WRITE_SIMPLE);
    return MPI_SUCCESS;
}
