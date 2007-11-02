/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidimpl.h"

/*@
   packer_make_progress - make progress

   Notes:
@*/
int packer_make_progress()
{
    MM_Car *car_ptr, *car_next_ptr;
    MM_Segment_buffer *buf_ptr;
    BOOL finished;
    MM_Car *next_qhead;
    MPIDI_STATE_DECL(MPID_STATE_PACKER_MAKE_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_PACKER_MAKE_PROGRESS);

    /* assign car_ptr to the first non-empty qhead, either
     * readq_head or writeq_head.  Then set next_qhead to
     * the other available qhead or NULL if empty
     */
    car_ptr = MPID_Process.packer_vc_ptr->readq_head;
    if (car_ptr == NULL)
    {
	if (MPID_Process.packer_vc_ptr->writeq_head == NULL)
	{
	    /* shortcut out if the queues are empty */
	    MPIDI_FUNC_EXIT(MPID_STATE_PACKER_MAKE_PROGRESS);
	    return MPI_SUCCESS;
	}
	car_ptr = MPID_Process.packer_vc_ptr->writeq_head;
	next_qhead = NULL;
    }
    else
    {
	next_qhead = MPID_Process.packer_vc_ptr->writeq_head;
    }

    /* Process either or both qheads */
    do
    {
	/* Process all the cars in the current queue */
	do
	{
	    car_next_ptr = car_ptr->vcqnext_ptr;
	    finished = FALSE;
	    buf_ptr = car_ptr->buf_ptr;
	    switch (buf_ptr->type)
	    {
	    case MM_NULL_BUFFER:
		err_printf("error, cannot pack from a null buffer\n");
		break;
	    case MM_SIMPLE_BUFFER:

	        /* This code is copied from the MM_TMP_BUFFER case.
		 * Maybe there is no packing for the simple buffer case.
		 * Maybe a simple buffer is already packed in which case
		 * this code is unnecessary?
		 */

		if (buf_ptr->simple.buf == NULL)
		{
		    /* Is this allowed? Can simple.buf be NULL? */
		    car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);
		    /* set the first variable to zero */
		    /* This variable will be updated each time MPID_Segment_pack is called. */
		    car_ptr->data.packer.first = 0;
		}
		/* set the last variable to the end of the segment */
		car_ptr->data.packer.last = buf_ptr->simple.len;
		/* pack the buffer */
		MPID_Segment_pack(
		    &car_ptr->request_ptr->mm.segment,
		    car_ptr->data.packer.first,
		    &car_ptr->data.packer.last,
		    car_ptr->request_ptr->mm.buf.simple.buf
		    );
		/* update the number of bytes read */
		buf_ptr->simple.num_read += (car_ptr->data.packer.last - car_ptr->data.packer.first);

		/* if the entire buffer is packed then break */
		/*if (car_ptr->data.packer.last == buf_ptr->simple.len)*/ /* the length and the segment last are equal, right? */
		if (car_ptr->data.packer.last == car_ptr->request_ptr->mm.last)
		{
		    finished = TRUE;
		    break;
		}
		/* otherwise there is more packing needed so update the first variable */
		/* The last variable will be updated the next time through this function */
		car_ptr->data.packer.first = car_ptr->data.packer.last;
		break;
	    case MM_TMP_BUFFER:
		if (buf_ptr->tmp.buf == NULL)
		{
		    /* the buffer is empty so get a tmp buffer */
		    car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);
		    /* set the first variable to zero */
		    /* This variable will be updated each time MPID_Segment_pack is called. */
		    car_ptr->data.packer.first = 0;
		    /* ERROR: If the buffer is allocated elsewhere, who will set data.packer.first to zero? */
		}
		/* set the last variable to the end of the segment */
		car_ptr->data.packer.last = buf_ptr->tmp.len;
		/* pack the buffer */
		MPID_Segment_pack(
		    &car_ptr->request_ptr->mm.segment,
		    car_ptr->data.packer.first,
		    &car_ptr->data.packer.last,
		    car_ptr->request_ptr->mm.buf.tmp.buf
		    );
		/* update the number of bytes read */
		buf_ptr->tmp.num_read += (car_ptr->data.packer.last - car_ptr->data.packer.first);

		/* if the entire buffer is packed then break */
		/*if (car_ptr->data.packer.last == buf_ptr->tmp.len)*/ /* the length and the segment last are equal, right? */
		if (car_ptr->data.packer.last == car_ptr->request_ptr->mm.last)
		{
		    finished = TRUE;
		    break;
		}
		/* otherwise there is more packing needed so update the first variable */
		/* The last variable will be updated the next time through this function */
		car_ptr->data.packer.first = car_ptr->data.packer.last;
		break;
	    case MM_VEC_BUFFER:
		/* If there are no write cars outstanding for the current vec data */
		if (buf_ptr->vec.num_cars_outstanding == 0)
		{
		    /* then update vec to point to the next pieces of data */
		    car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);
		    /* In the vector packing case, once the vector is filled by the get_buffers call there is no need to move
		     * any data.  So, num_read can be set to the amount of data that was packed into the vector. */
		    buf_ptr->vec.num_read = buf_ptr->vec.buf_size; /* buf_size = last - first */
		    /* signal the writers that data is available by setting num_cars_outstanding */
		    buf_ptr->vec.num_cars_outstanding = buf_ptr->vec.num_cars;
		}
		/* vec packing is finished when the current vec describes the last piece of the segment */
		if (buf_ptr->vec.last == car_ptr->request_ptr->mm.last)
		    finished = TRUE;
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
#ifdef WITH_MEHTOD_IB
	    case MM_IB_BUFFER:
		break;
#endif
#ifdef WITH_METHOD_NEW
	    case MM_NEW_METHOD_BUFFER:
		break;
#endif
	    default:
		err_printf("illegal buffer type: %d\n", car_ptr->request_ptr->mm.buf.type);
		break;
	    }
	    if (finished)
	    {
		/* the car has finished packing so put it in the completion queue */
		packer_car_dequeue(MPID_Process.packer_vc_ptr, car_ptr);
		mm_cq_enqueue(car_ptr);
	    }
	    /* continue on to the next car whether or not the current car has been completely packed.
	     * packing can occur out of order */
	    car_ptr = car_next_ptr;
	} while (car_ptr);

	/* continue to the next queue */
	car_ptr = next_qhead;
	/* set the next q pointer to NULL, guaranteeing we break out of the loop on the second pass */
	next_qhead = NULL;
    } while (car_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_PACKER_MAKE_PROGRESS);
    return MPI_SUCCESS;
}
