/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_iWrite()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iWrite
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iWrite(MPIDI_VC * vc, MPID_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    int nb;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_IWRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_IWRITE);

    MPIDI_DBG_PRINTF((71, FCNAME, "entering"));

    req->ch.iov_offset = 0;
    vc->ch.send_active = req;
    mpi_errno = (req->dev.iov_count == 1) ?
	ibu_write(vc->ch.ibu, req->dev.iov, req->dev.iov->MPID_IOV_LEN, &nb) :
	ibu_writev(vc->ch.ibu, req->dev.iov, req->dev.iov_count, &nb);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ibwrite", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IWRITE);
	return mpi_errno;
    }

    if (nb > 0)
    {
	if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	{
	    /* Write operation complete */
	    MPIDI_CA_t ca = req->dev.ca;
	    
	    vc->ch.send_active = NULL;
	    
	    if (ca == MPIDI_CH3_CA_COMPLETE)
	    {
		if (MPIDI_CH3I_SendQ_head(vc) == req)
		{
		    MPIDI_CH3I_SendQ_dequeue(vc);
		}
		vc->ch.send_active = MPIDI_CH3I_SendQ_head(vc);
		/* mark data transfer as complete and decrment CC */
		req->dev.iov_count = 0;
		MPIDI_CH3U_Request_complete(req);
	    }
	    else if (ca == MPIDI_CH3I_CA_HANDLE_PKT)
	    {
		MPIDI_CH3_Pkt_t * pkt = &req->ch.pkt;
		
		if (pkt->type < MPIDI_CH3_PKT_END_CH3)
		{
		    vc->ch.send_active = MPIDI_CH3I_SendQ_head(vc);
		}
		else
		{
		    MPIDI_DBG_PRINTF((71, FCNAME, "unknown packet type %d", pkt->type));
		}
	    }
	    else if (ca < MPIDI_CH3_CA_END_CH3)
	    {
		MPIDI_DBG_PRINTF((71, FCNAME, "finished sending iovec, calling CH3U_Handle_send_req()"));
		MPIDI_CH3U_Handle_send_req(vc, req);
		if (req->dev.iov_count == 0)
		{
		    /* NOTE: This code assumes that if another write is not posted by the device during the callback, then the
		       device has completed the current request.  As a result, the current request is dequeded and next request
		       in the queue is processed. */
		    if (MPIDI_CH3I_SendQ_head(vc) == req)
		    {
			MPIDI_DBG_PRINTF((71, FCNAME, "request (assumed) complete, dequeuing req and posting next send"));
			MPIDI_CH3I_SendQ_dequeue(vc);
		    }
		    vc->ch.send_active = MPIDI_CH3I_SendQ_head(vc);
		}
	    }
	    else
	    {
		assert(ca < MPIDI_CH3I_CA_END_IB);
	    }
	}
	else
	{
	    assert(req->ch.iov_offset < req->dev.iov_count);
	}
    }
    else if (nb == 0)
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "unable to write, enqueuing"));
	MPIDI_CH3I_SendQ_enqueue(vc, req);
    }
    else
    {
	/* Connection just failed.  Mark the request complete and return an error. */
	vc->ch.state = MPIDI_CH3I_VC_STATE_FAILED;
	/* TODO: Create an appropriate error message based on the value of errno */
	req->status.MPI_ERROR = MPI_ERR_INTERN;
	/* MT - CH3U_Request_complete performs write barrier */
	MPIDI_CH3U_Request_complete(req);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IWRITE);
    return mpi_errno;
}
