/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ibimpl.h"

#ifdef WITH_METHOD_IB

int ib_post_connect(MPIDI_VC *vc_ptr, char *business_card)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_POST_CONNECT);

    MPIU_DBG_PRINTF(("ib_post_connect\n"));

    MPID_Thread_lock(vc_ptr->lock);
    if ((business_card == NULL) || (strlen(business_card) > 100))
    {
	err_printf("ib_post_connect: invalid business card\n");
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_POST_CONNECT);
	return -1;
    }

    MPID_Thread_unlock(vc_ptr->lock);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_POST_CONNECT);
    return MPI_SUCCESS;
}

int ib_handle_connect(MPIDI_VC *vc_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_CONNECT);
    MPID_Thread_lock(vc_ptr->lock);

    MPID_Thread_unlock(vc_ptr->lock);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_CONNECT);
    return MPI_SUCCESS;
}

int ib_handle_written_ack(MPIDI_VC *vc_ptr, int num_written)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_WRITTEN_ACK);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_WRITTEN_ACK);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_WRITTEN_ACK);
    return MPI_SUCCESS;
}

int ib_handle_written_context_pkt(MPIDI_VC *vc_ptr, int num_written)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_WRITTEN_CONTEXT_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_WRITTEN_CONTEXT_PKT);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_WRITTEN_CONTEXT_PKT);
    return MPI_SUCCESS;
}

#endif
