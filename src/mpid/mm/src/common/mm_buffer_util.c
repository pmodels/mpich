/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_release_buffers_tmp(MPID_Request *request_ptr)
{
    MM_Segment_buffer *buf_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MM_RELEASE_BUFFERS_TMP);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_RELEASE_BUFFERS_TMP);

    buf_ptr = &request_ptr->mm.buf;

#ifdef MPICH_DEV_BUILD
    if (buf_ptr->tmp.buf == NULL)
    {
	err_printf("mm_release_buffers_tmp called on an empty buffer.\n");
	return -1;
    }
    buf_ptr->tmp.len = -1;
#endif
    MPIU_Free(buf_ptr->tmp.buf);
    buf_ptr->tmp.buf = NULL;

    MPIDI_FUNC_EXIT(MPID_STATE_MM_RELEASE_BUFFERS_TMP);
    return MPI_SUCCESS;
}

int mm_get_buffers_tmp(MPID_Request *request_ptr)
{
    MM_Segment_buffer *buf_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MM_GET_BUFFERS_TMP);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_GET_BUFFERS_TMP);

    buf_ptr = &request_ptr->mm.buf;

#ifdef MPICH_DEV_BUILD
    if (buf_ptr->tmp.buf != NULL)
    {
	err_printf("mm_get_buffers_tmp called on a buffer that has already been allocated.\n");
	return -1;
    }
#endif
    buf_ptr->tmp.buf = MPIU_Malloc(request_ptr->mm.size);
    buf_ptr->tmp.len = request_ptr->mm.size;

    MPIDI_FUNC_EXIT(MPID_STATE_MM_GET_BUFFERS_TMP);
    return MPI_SUCCESS;
}

int tmp_buffer_init(MPID_Request *request_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_TMP_BUFFER_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_TMP_BUFFER_INIT);
    
    request_ptr->mm.buf.type = MM_TMP_BUFFER;
    request_ptr->mm.buf.tmp.buf = NULL;
    request_ptr->mm.buf.tmp.len = 0;
    request_ptr->mm.buf.tmp.num_read = 0;
    
    MPIDI_FUNC_EXIT(MPID_STATE_TMP_BUFFER_INIT);
    return MPI_SUCCESS;
}

int simple_buffer_init(MPID_Request *request_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SIMPLE_BUFFER_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_SIMPLE_BUFFER_INIT);

    request_ptr->mm.buf.type = MM_SIMPLE_BUFFER;
    request_ptr->mm.buf.simple.buf = NULL;
    request_ptr->mm.buf.simple.len = 0;
    request_ptr->mm.buf.simple.num_read = 0;

    MPIDI_FUNC_EXIT(MPID_STATE_SIMPLE_BUFFER_INIT);
    return MPI_SUCCESS;
}

int mm_get_buffers_vec(MPID_Request *request_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MM_GET_BUFFERS_VEC);
    MPIDI_FUNC_ENTER(MPID_STATE_MM_GET_BUFFERS_VEC);

    /* set first and last to be the current last and the end of the segment */
    request_ptr->mm.buf.vec.first = request_ptr->mm.buf.vec.last;
    request_ptr->mm.buf.vec.last = request_ptr->mm.last;

    /* pack as much of the segment into the vector as possible */
    /* note: this does not move data, it only fills in the vector buffers and lengths */
    MPID_Segment_pack_vector(
	&request_ptr->mm.segment,
	request_ptr->mm.buf.vec.first,
	&request_ptr->mm.buf.vec.last,
	request_ptr->mm.buf.vec.vec,
	&request_ptr->mm.buf.vec.vec_size);

    /* the size of the current vector is the amount of data that was packed */
    request_ptr->mm.buf.vec.buf_size = request_ptr->mm.buf.vec.last - request_ptr->mm.buf.vec.first;

    MPIDI_FUNC_EXIT(MPID_STATE_MM_GET_BUFFERS_VEC);
    return MPI_SUCCESS;
}

int vec_buffer_init(MPID_Request *request_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_VEC_BUFFER_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_VEC_BUFFER_INIT);

    request_ptr->mm.buf.type = MM_VEC_BUFFER;
    request_ptr->mm.buf.vec.vec_size = MPID_IOV_LIMIT;
    request_ptr->mm.buf.vec.num_read = 0;
    request_ptr->mm.buf.vec.first = 0;
    request_ptr->mm.buf.vec.last = 0;
    request_ptr->mm.buf.vec.segment_last = request_ptr->mm.last;
    request_ptr->mm.buf.vec.buf_size = 0;
    request_ptr->mm.buf.vec.num_cars_outstanding = 0;
    request_ptr->mm.buf.vec.num_cars = 0;

    MPIDI_FUNC_EXIT(MPID_STATE_VEC_BUFFER_INIT);
    return MPI_SUCCESS;
}
