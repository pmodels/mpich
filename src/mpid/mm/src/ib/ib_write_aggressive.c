/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ibimpl.h"

#ifdef WITH_METHOD_IB

/* prototypes */
#ifdef WITH_METHOD_SHM
int ib_stuff_vector_shm(MPID_IOV *, int *, MM_Car *, MM_Segment_buffer *);
#endif
#ifdef WITH_METHOD_VIA
int ib_stuff_vector_via(MPID_IOV *, int *, MM_Car *, MM_Segment_buffer *);
#endif
#ifdef WITH_METHOD_VIA_RDMA
int ib_stuff_vector_via_rdma(MPID_IOV *, int *, MM_Car *, MM_Segment_buffer *);
#endif
#ifdef WITH_METHOD_IB
int ib_stuff_vector_ib(MPID_IOV *, int *, MM_Car *, MM_Segment_buffer *);
#endif
#ifdef WITH_METHOD_NEW
int ib_stuff_vector_new(MPID_IOV *, int *, MM_Car *, MM_Segment_buffer *);
#endif
int ib_stuff_vector_vec(MPID_IOV *, int *, MM_Car *, MM_Segment_buffer *);
int ib_stuff_vector_tmp(MPID_IOV *, int *, MM_Car *, MM_Segment_buffer *);
int ib_stuff_vector_simple(MPID_IOV *, int *, MM_Car *, MM_Segment_buffer *);
int ib_update_car_num_written(MM_Car *, int *);

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifdef WITH_METHOD_SHM
int ib_stuff_vector_shm(MPID_IOV *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_STUFF_VECTOR_SHM);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_STUFF_VECTOR_SHM);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_STUFF_VECTOR_SHM);
    return FALSE;
}
#endif

#ifdef WITH_METHOD_VIA
int ib_stuff_vector_via(MPID_IOV *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_STUFF_VECTOR_VIA);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_STUFF_VECTOR_VIA);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_STUFF_VECTOR_VIA);
    return FALSE;
}
#endif

#ifdef WITH_METHOD_VIA_RDMA
int ib_stuff_vector_via_rdma(MPID_IOV *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_STUFF_VECTOR_VIA_RDMA);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_STUFF_VECTOR_VIA_RDMA);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_STUFF_VECTOR_VIA_RDMA);
    return FALSE;
}
#endif

int ib_stuff_vector_vec(MPID_IOV *vec, int *cur_pos_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    int cur_pos, cur_index, num_avail, final_segment;
    MPID_IOV *car_vec, *buf_vec;
    int num_left, i;
    MPIDI_STATE_DECL(MPID_STATE_IB_STUFF_VECTOR_VEC);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_STUFF_VECTOR_VEC);

    /* check to see that there is space in the vector to put data */
    if (*cur_pos_ptr == MPID_IOV_LIMIT)
    {
	/* cur_pos has run off the end of the vector */
	MPIDI_FUNC_EXIT(MPID_STATE_IB_STUFF_VECTOR_VEC);
	return FALSE;
    }

    cur_index = car_ptr->data.ib.buf.vec_write.cur_index;
    car_vec = car_ptr->data.ib.buf.vec_write.vec;
    if (car_ptr->data.ib.buf.vec_write.num_read_copy != buf_ptr->vec.num_read)
    {
	/* update car vector */
	buf_vec = buf_ptr->vec.vec;
	
	/* update num_read_copy */
	car_ptr->data.ib.buf.vec_write.num_read_copy = buf_ptr->vec.num_read;
	
	/* copy the buf vector into the car vector from the current index to the end */
	memcpy(&car_vec[cur_index], &buf_vec[cur_index], 
	    (buf_ptr->vec.vec_size - cur_index) * sizeof(MPID_IOV));
	car_vec[cur_index].MPID_IOV_BUF = 
	    (char*)car_vec[cur_index].MPID_IOV_BUF + car_ptr->data.ib.buf.vec_write.num_written_at_cur_index;
	car_vec[cur_index].MPID_IOV_LEN = car_vec[cur_index].MPID_IOV_LEN - car_ptr->data.ib.buf.vec_write.num_written_at_cur_index;
	
	/* modify the vector copied from buf_ptr->vec to represent only the data that has been read 
	* This is done by traversing the vector, subtracting the lengths of each buffer until all the read
	* data is accounted for.
	*/
	
	/* set the size of the car vector to zero */
	car_ptr->data.ib.buf.vec_write.vec_size = 0;
	
	/* add vector elements to the size until all the read data is accounted for */
	num_left = car_ptr->data.ib.buf.vec_write.num_read_copy - car_ptr->data.ib.buf.vec_write.cur_num_written;
	i = cur_index;
	while (num_left > 0)
	{
	    car_ptr->data.ib.buf.vec_write.vec_size++;
	    num_left -= car_vec[i].MPID_IOV_LEN;
	    i++;
	}
	/* if the last vector buffer is larger than the amount of data read into that buffer,
	update the length field in the car's copy of the vector to represent only the read data */
	if (num_left < 0)
	{
	    car_vec[i].MPID_IOV_LEN += num_left;
	}
	
	/* at this point the vec in the car describes all the currently read data */
    }

    if (car_ptr->data.ib.buf.vec_write.cur_num_written == car_ptr->data.ib.buf.vec_write.num_read_copy)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IB_STUFF_VECTOR_VEC);
	return FALSE;
    }

    /* copy as much of the car vector into the stuffed vector as possible */
    cur_pos = *cur_pos_ptr;
    /*cur_index = car_ptr->data.ib.buf.vec_write.cur_index;*/
    /* The amount available is the amount read minus the amount previously written. */
    num_avail = buf_ptr->vec.num_read - car_ptr->data.ib.buf.vec_write.cur_num_written;
    /* If the amount available for writing plus the total amount previously written is equal to the total size
       of the segment (segment_last) then set final_segment to TRUE */
    final_segment = (num_avail + car_ptr->data.ib.buf.vec_write.total_num_written) == buf_ptr->vec.segment_last;

    /*msg_printf("ib_stuff_vector_vec: num_avail = %d\n", num_avail);*/

    while ((cur_pos < MPID_IOV_LIMIT) && num_avail)
    {
	vec[cur_pos].MPID_IOV_BUF = car_vec[cur_index].MPID_IOV_BUF;
	num_avail -= (vec[cur_pos].MPID_IOV_LEN = car_vec[cur_index].MPID_IOV_LEN);
	cur_index++;
	cur_pos++;
    }
    *cur_pos_ptr = cur_pos;

    /* return TRUE if this segment is completely copied into the vec array */
    if (num_avail == 0 && final_segment)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IB_STUFF_VECTOR_VEC);
	return TRUE;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_STUFF_VECTOR_VEC);
    return FALSE;
}

int ib_stuff_vector_tmp(MPID_IOV *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_STUFF_VECTOR_TMP);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_STUFF_VECTOR_TMP);

    /* check to see that there is data available and space in the vector to put it */
    if ((*cur_pos == MPID_IOV_LIMIT) ||
        (car_ptr->data.ib.buf.tmp.num_written == buf_ptr->tmp.num_read) ||
	(buf_ptr->tmp.num_read == 0))
    {
	/* cur_pos has run off the end of the vector OR
	   If num_written is equal to num_read then there is no data available to write. OR
	   No data has been read */
	MPIDI_FUNC_EXIT(MPID_STATE_IB_STUFF_VECTOR_TMP);
	return FALSE;
    }

    /*msg_printf("ib_stuff_vector_tmp: added %d byte buffer\n", buf_ptr->tmp.num_read - car_ptr->data.ib.buf.tmp.num_written);*/

    /* add the tmp buffer to the vector */
    vec[*cur_pos].MPID_IOV_BUF = (char*)(buf_ptr->tmp.buf) + car_ptr->data.ib.buf.tmp.num_written;
    vec[*cur_pos].MPID_IOV_LEN = buf_ptr->tmp.num_read - car_ptr->data.ib.buf.tmp.num_written;
    (*cur_pos)++;

    /* if the entire segment is in the vector then return true else false */
    if (buf_ptr->tmp.num_read == buf_ptr->tmp.len)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IB_STUFF_VECTOR_TMP);
	return TRUE;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_STUFF_VECTOR_TMP);
    return FALSE;
}

int ib_stuff_vector_simple(MPID_IOV *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_STUFF_VECTOR_SIMPLE);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_STUFF_VECTOR_SIMPLE);

    /* check to see that there is data available and space in the vector to put it */
    if ((*cur_pos == MPID_IOV_LIMIT) ||
        (car_ptr->data.ib.buf.simple.num_written == buf_ptr->simple.num_read) ||
	(buf_ptr->simple.num_read == 0))
    {
	/* cur_pos has run off the end of the vector OR
	   If num_written is equal to num_read then there is no data available to write. OR
	   No data has been read */
	MPIDI_FUNC_EXIT(MPID_STATE_IB_STUFF_VECTOR_SIMPLE);
	return FALSE;
    }

    /*msg_printf("ib_stuff_vector_simple: added %d byte buffer\n", buf_ptr->simple.num_read - car_ptr->data.ib.buf.simple.num_written);*/

    /* add the simple buffer to the vector */
    vec[*cur_pos].MPID_IOV_BUF = (char*)(buf_ptr->simple.buf) + car_ptr->data.ib.buf.simple.num_written;
    vec[*cur_pos].MPID_IOV_LEN = buf_ptr->simple.num_read - car_ptr->data.ib.buf.simple.num_written;
    (*cur_pos)++;

    /* if the entire segment is in the vector then return true else false */
    if (buf_ptr->simple.num_read == buf_ptr->simple.len)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IB_STUFF_VECTOR_SIMPLE);
	return TRUE;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_STUFF_VECTOR_SIMPLE);
    return FALSE;
}

#ifdef WITH_METHOD_IB
int ib_stuff_vector_ib(MPID_IOV *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_STUFF_VECTOR_IB);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_STUFF_VECTOR_IB);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_STUFF_VECTOR_IB);
    return FALSE;
}
#endif

#ifdef WITH_METHOD_NEW
int ib_stuff_vector_new(MPID_IOV *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_STUFF_VECTOR_NEW);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_STUFF_VECTOR_NEW);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_STUFF_VECTOR_NEW);
    return FALSE;
}
#endif

int ib_update_car_num_written(MM_Car *car_ptr, int *num_written_ptr)
{
    MM_Segment_buffer *buf_ptr;
    int num_written;
    int num_left, i;
    MPIDI_STATE_DECL(MPID_STATE_IB_UPDATE_CAR_NUM_WRITTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_UPDATE_CAR_NUM_WRITTEN);

#ifdef MPICH_DEV_BUILD
    if (car_ptr == NULL)
    {
	err_printf("Error: ib_update_car_num_written called on a NULL car pointer, num_written = %d\n", *num_written_ptr);
    }
#endif

    buf_ptr = car_ptr->buf_ptr;
    num_written = *num_written_ptr;

    switch (buf_ptr->type)
    {
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
    case MM_VEC_BUFFER:
	num_written = min(
	    /* the maximum amout of buffer space in this buffer that could have been written */
	    buf_ptr->vec.segment_last - car_ptr->data.ib.buf.vec_write.total_num_written,
	    /* the actual amount written */
	    num_written);

	/*msg_printf("num_written vec: %d\n", num_written);*/
	
	/* update vector */
	car_ptr->data.ib.buf.vec_write.cur_num_written += num_written;
	car_ptr->data.ib.buf.vec_write.total_num_written += num_written;
	if (car_ptr->data.ib.buf.vec_write.cur_num_written == buf_ptr->vec.buf_size)
	{
	    /* reset this car */
	    car_ptr->data.ib.buf.vec_write.cur_index = 0;
	    car_ptr->data.ib.buf.vec_write.num_read_copy = 0;
	    car_ptr->data.ib.buf.vec_write.cur_num_written = 0;
	    car_ptr->data.ib.buf.vec_write.num_written_at_cur_index = 0;
	    car_ptr->data.ib.buf.vec_write.vec_size = 0;
	    /* signal that we have finished writing the current vector */
	    mm_dec_atomic(&(buf_ptr->vec.num_cars_outstanding));
	}
	else
	{
	    num_left = num_written;
	    i = car_ptr->data.ib.buf.vec_write.cur_index;
	    while (num_left > 0)
	    {
		/* subtract the length of the current vector */
		num_left -= car_ptr->data.ib.buf.vec_write.vec[i].MPID_IOV_LEN;
		if (num_left > 0)
		{
		    /* the entire vector was written so move to the next index */
		    i++;
		}
		else
		{
		    /* this vector was only partially written, so update the buf and len fields */
		    car_ptr->data.ib.buf.vec_write.vec[i].MPID_IOV_BUF = 
			(char*)(car_ptr->data.ib.buf.vec_write.vec[i].MPID_IOV_BUF) +
			car_ptr->data.ib.buf.vec_write.vec[i].MPID_IOV_LEN + 
			num_left;
		    car_ptr->data.ib.buf.vec_write.num_written_at_cur_index = 
			car_ptr->data.ib.buf.vec_write.vec[i].MPID_IOV_LEN + num_left;
		    car_ptr->data.ib.buf.vec_write.vec[i].MPID_IOV_LEN = -num_left;
		}
	    }
	    car_ptr->data.ib.buf.vec_write.cur_index = i;
	}

	/* if the entire mpi segment has been written, enqueue the car in the completion queue */
	if (car_ptr->data.ib.buf.vec_write.total_num_written == buf_ptr->vec.segment_last)
	{
#ifdef MPICH_DEV_BUILD
	    if (car_ptr != car_ptr->vc_ptr->writeq_head)
	    {
		err_printf("Error: ib_update_car_num_written not dequeueing the head write car.\n");
	    }
#endif
	    ib_car_dequeue_write(car_ptr->vc_ptr);
	    /*printf("dec cc: written vec: %d\n", num_written);fflush(stdout);*/
	    mm_dec_cc_atomic(car_ptr->request_ptr);
	    mm_car_free(car_ptr);
	}
	else
	{
	    /*msg_printf("partial buffer written, total_num_written %d, segment_last %d\n", car_ptr->data.ib.buf.vec_write.total_num_written, buf_ptr->vec.segment_last);*/
	}
	break;
    case MM_TMP_BUFFER:
	num_written = min(
	    /* the amout of buffer space available to have been written */
	    buf_ptr->tmp.len - car_ptr->data.ib.buf.tmp.num_written,
	    /* the actual amount written */
	    num_written);
	
	/*msg_printf("num_written tmp: %d\n", num_written);*/

	/* update the amount written */
	car_ptr->data.ib.buf.tmp.num_written += num_written;
	
	/* check to see if finished */
	if (car_ptr->data.ib.buf.tmp.num_written == buf_ptr->tmp.len)
	{
	    dbg_printf("num_written: %d\n", car_ptr->data.ib.buf.tmp.num_written);
	    /* remove from write queue and insert in completion queue */
#ifdef MPICH_DEV_BUILD
	    if (car_ptr != car_ptr->vc_ptr->writeq_head)
	    {
		err_printf("Error: ib_update_car_num_written not dequeueing the head write car.\n");
	    }
#endif
	    ib_car_dequeue_write(car_ptr->vc_ptr);
	    /*printf("dec cc: written tmp buffer: %d\n", num_written);fflush(stdout);*/
	    mm_dec_cc_atomic(car_ptr->request_ptr);
	    mm_car_free(car_ptr);
	}
	break;
    case MM_SIMPLE_BUFFER:
	num_written = min(
	    /* the amout of buffer space available to have been written */
	    buf_ptr->simple.len - car_ptr->data.ib.buf.simple.num_written,
	    /* the actual amount written */
	    num_written);
	
	/*msg_printf("num_written simple: %d\n", num_written);*/

	/* update the amount written */
	car_ptr->data.ib.buf.simple.num_written += num_written;
	
	/* check to see if finished */
	if (car_ptr->data.ib.buf.simple.num_written == buf_ptr->simple.len)
	{
	    dbg_printf("num_written: %d\n", car_ptr->data.ib.buf.simple.num_written);
	    /* remove from write queue and insert in completion queue */
#ifdef MPICH_DEV_BUILD
	    if (car_ptr != car_ptr->vc_ptr->writeq_head)
	    {
		err_printf("Error: ib_update_car_num_written not dequeueing the head write car.\n");
	    }
#endif
	    ib_car_dequeue_write(car_ptr->vc_ptr);
	    /*printf("dec cc: written simple buffer: %d\n", num_written);fflush(stdout);*/
	    mm_dec_cc_atomic(car_ptr->request_ptr);
	    mm_car_free(car_ptr);
	}
	break;
#ifdef WITH_METHOD_IB
    case MM_IB_BUFFER:
	break;
#endif
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD_BUFFER:
	break;
#endif
    case MM_NULL_BUFFER:
	err_printf("Error: ib_update_car_num_written called on a null buffer\n");
	break;
    default:
	err_printf("Error: ib_update_car_num_written: unknown or unsupported buffer type: %d\n", buf_ptr->type);
	break;
    }

    /* update num_written */
    (*num_written_ptr) -= num_written;

    MPIDI_FUNC_EXIT(MPID_STATE_IB_UPDATE_CAR_NUM_WRITTEN);
    return MPI_SUCCESS;
}

/*@
   ib_write_aggressive - write as many cars as possible

   Parameters:
+  MPIDI_VC *vc_ptr - vc

   Notes:
@*/
int ib_write_aggressive(MPIDI_VC *vc_ptr)
{
    int error;
    MM_Car *car_ptr;
    MM_Segment_buffer *buf_ptr;

    /* This needs to be available until the posted write finishes */
    /* A quick fix was to copy the array into the ib structure */
    /* Maybe this array can be kept in the VC */
    MPID_IOV vec[MPID_IOV_LIMIT];

    int cur_pos = 0;
    BOOL stop = FALSE;
    MPIDI_STATE_DECL(MPID_STATE_IB_WRITE_AGGRESSIVE);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_WRITE_AGGRESSIVE);

    MPIU_DBG_PRINTF(("ib_write_aggressive\n"));

    if (vc_ptr->writeq_head == NULL)
    {
	/*msg_printf("ib_write_aggressive: write signalled with no car's in the write queue.\n");*/
	MPIDI_FUNC_EXIT(MPID_STATE_IB_WRITE_AGGRESSIVE);
	return MPI_SUCCESS;
    }

    car_ptr = vc_ptr->writeq_head;

    /* pack as many cars into a vector as possible */
    do
    {
	buf_ptr = car_ptr->buf_ptr;
	switch (buf_ptr->type)
	{
#ifdef WITH_METHOD_SHM
	case MM_SHM_BUFFER:
	    stop = !ib_stuff_vector_shm(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
#endif
#ifdef WITH_METHOD_VIA
	case MM_VIA_BUFFER:
	    stop = !ib_stuff_vector_via(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	case MM_VIA_RDMA_BUFFER:
	    stop = !ib_stuff_vector_via_rdma(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
#endif
	case MM_VEC_BUFFER:
	    if (buf_ptr->vec.num_cars_outstanding > 0)
	    {
		/* num_cars_outstanding > 0 means that the reader has provided data and is waiting for the writers to complete */
		stop = !ib_stuff_vector_vec(vec, &cur_pos, car_ptr, buf_ptr);
	    }
	    else
	    {
		/* the reader hasn't read any data yet, so we can't write anything. */
		stop = TRUE;
	    }
	    break;
	case MM_TMP_BUFFER:
	    stop = !ib_stuff_vector_tmp(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
	case MM_SIMPLE_BUFFER:
	    stop = !ib_stuff_vector_simple(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
#ifdef WITH_METHOD_IB
	case MM_IB_BUFFER:
	    stop = !ib_stuff_vector_ib(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
#endif
#ifdef WITH_METHOD_NEW
	case MM_NEW_METHOD_BUFFER:
	    stop = !ib_stuff_vector_new(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
#endif
	case MM_NULL_BUFFER:
	    err_printf("Error: ib_write_aggressive called on a null buffer\n");
	    break;
	default:
	    err_printf("Error: ib_write_aggressive: unknown or unsupported buffer type: %d\n", buf_ptr->type);
	    break;
	}
	if (stop)
	    break;
#define REALLY_AGGRESSIVE_WRITE
#ifdef REALLY_AGGRESSIVE_WRITE
	car_ptr = (car_ptr->next_ptr) ? 
	    car_ptr->next_ptr : /* go to the next car in the list */
	    car_ptr->vcqnext_ptr; /* else go to the next enqueued list */
#else
	car_ptr = car_ptr->next_ptr;
#endif
    } while (car_ptr);

    if (cur_pos > 0)
    {
	/* post a write of the data */
	if (cur_pos == 1)
	{
	    MPIU_DBG_PRINTF(("ibu_post_write, %d bytes\n", vec[0].MPID_IOV_LEN));
	    if ((error = ibr_post_write(vc_ptr, vec[0].MPID_IOV_BUF, vec[0].MPID_IOV_LEN, NULL)) != IB_SUCCESS)
	    {
		err_printf("ib_write_aggressive: ibu_post_write failed, error %d\n", error);
		MPIDI_FUNC_EXIT(MPID_STATE_IB_WRITE_AGGRESSIVE);
		return -1;
	    }
	}
	else
	{
	    /*** debugging printout */
	    {
		char str[1024], *s = str;
		int i, n=0;
		s += sprintf(s, "ibu_post_writev ");
		for (i=0; i<cur_pos; i++)
		{
		    s += sprintf(s, "%d+", vec[i].MPID_IOV_LEN);
		    n += vec[i].MPID_IOV_LEN;
		}
		s--;
		sprintf(s, "=%d bytes\n", n);
		MPIU_DBG_PRINTF(("%s", str));
	    }
	    /*** end debugging printout */

	    if ((error = ibr_post_writev(vc_ptr, vec, cur_pos, NULL)) != IB_SUCCESS)
	    {
		err_printf("ib_write_aggressive: ib_writev failed, error %d\n", error);
		MPIDI_FUNC_EXIT(MPID_STATE_IB_WRITE_AGGRESSIVE);
		return -1;
	    }
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_WRITE_AGGRESSIVE);
    return MPI_SUCCESS;
}

int ib_handle_written(MPIDI_VC *vc_ptr, void *mem_ptr, int num_written)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_WRITTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_WRITTEN);

    if (vc_ptr == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_WRITTEN);
	return MPI_SUCCESS;
    }

    MPIU_DBG_PRINTF(("ib_handle_written - %d bytes\n", num_written));

    /* update all the cars and buffers affected by the ib_write action */
    while (num_written)
    {
        /* ib_update_car_num_written causes the head to get dequeued and the next car to take its place 
	 * until all the data is accounted for or there is an error. */
	if (ib_update_car_num_written(vc_ptr->writeq_head, &num_written) != MPI_SUCCESS)
	{
	    err_printf("ib_write_aggressive:ib_update_car_num_written failed.\n");
	    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_WRITTEN);
	    return -1;
	}
	/*msg_printf("write_aggressive: num_written updated %d\n", num_written);*/
    }

    /* if there are more cars in the queue, post the next ib_write action */
    if (vc_ptr->writeq_head != NULL)
	ib_write_aggressive(vc_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_WRITTEN);
    return MPI_SUCCESS;
}

#endif
