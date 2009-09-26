/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_send_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Handle_send_req(MPIDI_VC_t * vc, MPID_Request * sreq, 
			       int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_SEND_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_SEND_REQ);

    /* Use the associated function rather than switching on the old ca field */
    /* Routines can call the attached function directly */
    reqFn = sreq->dev.OnDataAvail;
    if (!reqFn) {
	MPIU_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
	MPIDI_CH3U_Request_complete(sreq);
        *complete = 1;
    }
    else {
	mpi_errno = reqFn( vc, sreq, complete );
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_SEND_REQ);
    return mpi_errno;
}

/* ----------------------------------------------------------------------- */
/* Here are the functions that implement the actions that are taken when 
 * data is available for a send request (or other completion operations)
 * These include "send" requests that are part of the RMA implementation.
 */
/* ----------------------------------------------------------------------- */

int MPIDI_CH3_ReqHandler_GetSendRespComplete( MPIDI_VC_t *vc ATTRIBUTE((unused)), 
					      MPID_Request *sreq, 
					      int *complete )
{
    int mpi_errno = MPI_SUCCESS;

    /* FIXME: Should this test be an MPIU_Assert? */
    if (sreq->dev.source_win_handle != MPI_WIN_NULL) {
	MPID_Win *win_ptr;
	/* Last RMA operation (get) from source. If active target RMA,
	   decrement window counter. If passive target RMA, 
	   release lock on window and grant next lock in the 
	   lock queue if there is any; no need to send rma done 
	   packet since the last operation is a get. */
	
	MPID_Win_get_ptr(sreq->dev.target_win_handle, win_ptr);
	if (win_ptr->current_lock_type == MPID_LOCK_NONE) {
	    /* FIXME: MT: this has to be done atomically */
	    win_ptr->my_counter -= 1;
	}
	else {
	    mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
	}
    }

    /* mark data transfer as complete and decrement CC */
    MPIDI_CH3U_Request_complete(sreq);
    *complete = TRUE;

    return mpi_errno;
}

int MPIDI_CH3_ReqHandler_SendReloadIOV( MPIDI_VC_t *vc ATTRIBUTE((unused)), MPID_Request *sreq, 
					int *complete )
{
    int mpi_errno;

    /* setting the iov_offset to 0 here is critical, since it is intentionally
     * not set in the _load_send_iov function */
    sreq->dev.iov_offset = 0;
    sreq->dev.iov_count = MPID_IOV_LIMIT;
    mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, sreq->dev.iov, 
						 &sreq->dev.iov_count);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETFATALANDJUMP(mpi_errno, MPI_ERR_OTHER,"**ch3|loadsendiov");
    }
	    
    *complete = FALSE;

 fn_fail:
    return mpi_errno;
}
