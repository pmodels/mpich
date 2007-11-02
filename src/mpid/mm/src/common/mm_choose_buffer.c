/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   mm_choose_buffer - choose buffer

   Parameters:
+  MPID_Request *request_ptr - request pointer

   Notes:
   Choose the best buffer that is compatible with
   all the read and write cars attached to the segment (request)
@*/
int mm_choose_buffer(MPID_Request *request_ptr)
{
    MM_Car *car_ptr;
    MM_Segment_buffer *buf_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MM_CHOOSE_BUFFER);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_CHOOSE_BUFFER);

    dbg_printf("mm_choose_buffer\n");

    /* look at the read car and all of the write cars */
    /* pick the best buffer type that everyone can handle */
    /* if there are incompatible cars, allocate other requests and 
       connect them together with copier methods */
    /* pat your head and rub your tummy */

    if (request_ptr->mm.rcar[0].type != MM_NULL_CAR)
    {
	/* choose the tmp buffer type */
#ifdef CHOOSE_TMP_BUFFER
	tmp_buffer_init(request_ptr);
	/* count the cars that read/write data */
	/*
	car_ptr = request_ptr->mm.wcar;
	while (car_ptr)
	{
	    if (!(car_ptr->type & MM_HEAD_CAR) || (car_ptr->type & MM_PACKER_CAR) || (car_ptr->type & MM_UNPACKER_CAR))
		request_ptr->mm.buf.tmp.num_cars++;
	    car_ptr = car_ptr->opnext_ptr;
	}
	*/

#else
	vec_buffer_init(request_ptr);
	/* count the cars that read/write data */
	car_ptr = request_ptr->mm.wcar;
	while (car_ptr)
	{
	    if (!(car_ptr->type & MM_HEAD_CAR) || (car_ptr->type & MM_PACKER_CAR) || (car_ptr->type & MM_UNPACKER_CAR))
		request_ptr->mm.buf.vec.num_cars++;
	    car_ptr = car_ptr->opnext_ptr;
	}
#endif

	/* choose the buffers for the head write cars */
	car_ptr = request_ptr->mm.wcar;
	while (car_ptr)
	{
	    if (car_ptr->type & MM_HEAD_CAR)
	    {
		buf_ptr = &car_ptr->msg_header.buf;
		buf_ptr->type = MM_SIMPLE_BUFFER;
		buf_ptr->simple.buf = (void*)&car_ptr->msg_header.pkt;
		buf_ptr->simple.len = sizeof(MPID_Packet);
		buf_ptr->simple.num_read = sizeof(MPID_Packet);
		/*
		buf_ptr->type = MM_VEC_BUFFER;
		buf_ptr->vec.vec[0].MPID_IOV_BUF = (void*)&car_ptr->msg_header.pkt;
		buf_ptr->vec.vec[0].MPID_IOV_LEN = sizeof(MPID_Packet);
		buf_ptr->vec.vec_size = 1;
		buf_ptr->vec.num_read = sizeof(MPID_Packet);
		buf_ptr->vec.first = 0;
		buf_ptr->vec.last = sizeof(MPID_Packet);
		buf_ptr->vec.segment_last = sizeof(MPID_Packet);
		buf_ptr->vec.buf_size = sizeof(MPID_Packet);
		buf_ptr->vec.num_cars = 1;
		buf_ptr->vec.num_cars_outstanding = 1;
		*/
	    }
	    car_ptr = car_ptr->opnext_ptr;
	}

#ifdef CHOOSE_TMP_BUFFER
	request_ptr->mm.get_buffers = mm_get_buffers_tmp;
	request_ptr->mm.release_buffers = mm_release_buffers_tmp;
#else
	request_ptr->mm.get_buffers = mm_get_buffers_vec;
	request_ptr->mm.release_buffers = NULL;
#endif
    }
    else
    {
	request_ptr->mm.buf.type = MM_NULL_BUFFER;
	request_ptr->mm.get_buffers = NULL;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MM_CHOOSE_BUFFER);
    return MPI_SUCCESS;
}
