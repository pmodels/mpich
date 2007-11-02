/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int packer_reset_car(MM_Car *car_ptr)
{
    MM_Segment_buffer *buf_ptr;
    MPIDI_STATE_DECL(MPID_STATE_PACKER_RESET_CAR);

    MPIDI_FUNC_ENTER(MPID_STATE_PACKER_RESET_CAR);

    buf_ptr = car_ptr->buf_ptr;
    if (buf_ptr == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_PACKER_RESET_CAR);
	return -1;
    }

    switch (buf_ptr->type)
    {
    case MM_NULL_BUFFER:
	break;
    case MM_SIMPLE_BUFFER:
	car_ptr->data.packer.first = 0;
	car_ptr->data.packer.last = 0;
	break;
    case MM_TMP_BUFFER:
	car_ptr->data.packer.first = 0;
	car_ptr->data.packer.last = 0;
	break;
    case MM_VEC_BUFFER:
	car_ptr->data.packer.first = 0;
	car_ptr->data.packer.last = 0;
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
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_PACKER_RESET_CAR);
    return MPI_SUCCESS;
}
