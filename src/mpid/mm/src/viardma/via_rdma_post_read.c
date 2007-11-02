/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "via_rdmaimpl.h"

#ifdef WITH_METHOD_VIA_RDMA

int via_rdma_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_VIA_RDMA_POST_READ);
    MPIDI_FUNC_ENTER(MPID_STATE_VIA_RDMA_POST_READ);
    MPIDI_FUNC_EXIT(MPID_STATE_VIA_RDMA_POST_READ);
    return MPI_SUCCESS;
}

int via_rdma_merge_with_unexpected(MM_Car *car_ptr, MM_Car *unex_car_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_VIA_RDMA_MERGE_WITH_UNEXPECTED);
    MPIDI_FUNC_ENTER(MPID_STATE_VIA_RDMA_MERGE_WITH_UNEXPECTED);
    MPIDI_FUNC_EXIT(MPID_STATE_VIA_RDMA_MERGE_WITH_UNEXPECTED);
    return MPI_SUCCESS;
}

#endif
