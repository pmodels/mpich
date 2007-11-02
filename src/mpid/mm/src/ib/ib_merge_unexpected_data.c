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

#ifdef WITH_METHOD_SHM
int ib_merge_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
#endif
#ifdef WITH_METHOD_VIA
int ib_merge_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
#endif
#ifdef WITH_METHOD_VIA_RDMA
int ib_merge_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
#endif
int ib_merge_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
int ib_merge_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
int ib_merge_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
#ifdef WITH_METHOD_IB
int ib_merge_ib(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
#endif
#ifdef WITH_METHOD_NEW
int ib_merge_new(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
#endif

int ib_merge_unexpected_data(MPIDI_VC *vc_ptr, MM_Car *car_ptr, char *buffer, int length)
{
    int ret_val;
    MM_Segment_buffer *buf_ptr;
    MPIDI_STATE_DECL(MPID_STATE_IB_MERGE_UNEXPECTED_DATA);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_MERGE_UNEXPECTED_DATA);

    assert(car_ptr->type & MM_READ_CAR);

    buf_ptr = car_ptr->buf_ptr;
    switch (buf_ptr->type)
    {
    case MM_VEC_BUFFER:
	ret_val = ib_merge_vec(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
    case MM_SIMPLE_BUFFER:
	ret_val = ib_merge_simple(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
    case MM_TMP_BUFFER:
	ret_val = ib_merge_tmp(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#ifdef WITH_METHOD_SHM
    case MM_SHM_BUFFER:
	ret_val = ib_merge_shm(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_IB
    case MM_IB_BUFFER:
	ret_val = ib_merge_ib(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA
    case MM_VIA_BUFFER:
	ret_val = ib_merge_via(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    case MM_VIA_RDMA_BUFFER:
	ret_val = ib_merge_via_rdma(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD_BUFFER:
	ret_val = ib_merge_new(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#endif
    case MM_NULL_BUFFER:
	err_printf("Error: ib_merge_unexpected_data called on a null MM_Segment_buffer\n");
	break;
    default:
	err_printf("Error: ib_merge_unexpected_data: unknown or unsupported buffer type: %d\n", car_ptr->buf_ptr->type);
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_UNEXPECTED_DATA);
    return length;
}

#ifdef WITH_METHOD_SHM
int ib_merge_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_MERGE_SHM);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_MERGE_SHM);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_SHM);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA
int ib_merge_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_MERGE_VIA);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_MERGE_VIA);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_VIA);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA_RDMA
int ib_merge_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_MERGE_VIA_RDMA);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_MERGE_VIA_RDMA);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_VIA_RDMA);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_IB
int ib_merge_ib(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_MERGE_IB);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_MERGE_IB);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_IB);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_NEW
int ib_merge_new(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_MERGE_NEW);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_MERGE_NEW);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_NEW);
    return MPI_SUCCESS;
}
#endif

int ib_merge_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_MERGE_VEC);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_MERGE_VEC);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_VEC);
    return 0;
}

int ib_merge_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    int num_read;
    MPIDI_STATE_DECL(MPID_STATE_IB_MERGE_TMP);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_MERGE_TMP);

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
	/*ib_car_dequeue(vc_ptr, car_ptr);*/
	mm_cq_enqueue(car_ptr);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_TMP);
    return MPI_SUCCESS;
}

int ib_merge_simple(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    int num_read;
    MPIDI_STATE_DECL(MPID_STATE_IB_MERGE_SIMPLE);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_MERGE_SIMPLE);

    if (buf_ptr->simple.buf == NULL)
    {
	/* get the simple buffer */
	/*car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);*/
	err_printf("Error: ib_merge_simple called with NULL simple pointer\n");
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
	/*ib_car_dequeue(vc_ptr, car_ptr);*/
	mm_cq_enqueue(car_ptr);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_MERGE_SIMPLE);
    return MPI_SUCCESS;
}

#endif
