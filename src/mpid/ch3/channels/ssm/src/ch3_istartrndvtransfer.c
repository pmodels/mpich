/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#ifdef MPIDI_CH3_CHANNEL_RNDV

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartRndvTransferShm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3_iStartRndvTransferShm(MPIDI_VC_t * vc, MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
#ifndef USE_SHM_RDMA_GET
    MPIDI_CH3_Pkt_t pkt;
    MPID_Request *request_ptr;
#endif
#ifdef MPICH_DBG_OUTPUT
    int i;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTRNDVTRANSFERSHM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTRNDVTRANSFERSHM);

#ifdef USE_SHM_RDMA_GET

#ifdef MPICH_DBG_OUTPUT
    for (i=0; i<rreq->dev.iov_count; i++)
    {
	MPIU_DBG_PRINTF(("iStartRndvTransfer: recv buf[%d] = %p, len = %d\n", i, rreq->dev.iov[i].MPID_IOV_BUF, rreq->dev.iov[i].MPID_IOV_LEN));
	MPIU_DBG_PRINTF(("iStartRndvTransfer: send buf[%d] = %p, len = %d\n", i, rreq->dev.rdma_iov[i].MPID_IOV_BUF, rreq->dev.rdma_iov[i].MPID_IOV_LEN));
    }
#endif
    mpi_errno = MPIDI_CH3I_SHM_rdma_readv(vc, rreq);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

#else

    MPIDI_Pkt_init(&pkt.cts_iov, MPIDI_CH3_PKT_CTS_IOV);
    pkt.cts_iov.sreq = rreq->dev.sender_req_id;
    pkt.cts_iov.rreq = rreq->handle;
    pkt.cts_iov.iov_len = rreq->dev.iov_count;

    rreq->dev.rdma_iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&pkt;
    rreq->dev.rdma_iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    rreq->dev.rdma_iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rreq->dev.iov;
    rreq->dev.rdma_iov[1].MPID_IOV_LEN = rreq->dev.iov_count * sizeof(MPID_IOV);

#ifdef MPICH_DBG_OUTPUT
    for (i=0; i<rreq->dev.iov_count; i++)
    {
	MPIU_DBG_PRINTF(("iStartRndvTransfer: recv buf[%d] = %p, len = %d\n", i, rreq->dev.iov[i].MPID_IOV_BUF, rreq->dev.iov[i].MPID_IOV_LEN));
    }
#endif
    mpi_errno = MPIDI_CH3_iStartMsgv(vc, rreq->dev.rdma_iov, 2, &request_ptr);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_Object_set_ref(rreq, 0);
	MPIDI_CH3_Request_destroy(rreq);
	rreq = NULL;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|ctspkt", 0);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    if (request_ptr != NULL)
    {
	/* The sender doesn't need to know when the message has been sent.  So release the request immediately */
	MPID_Request_release(request_ptr);
    }

#endif

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTRNDVTRANSFERSHM);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartRndvTransfer
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartRndvTransfer(MPIDI_VC_t * vc, MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTRNDVTRANSFER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTRNDVTRANSFER);

    if (vc->ch.bShm)
    {
	mpi_errno = MPIDI_CH3_iStartRndvTransferShm(vc, rreq);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
    }
    else
    {
	MPID_Request * cts_req;
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_rndv_clr_to_send_t * cts_pkt = &upkt.rndv_clr_to_send;

	MPIDI_DBG_PRINTF((15, FCNAME, "rndv RTS in the request, sending rndv CTS"));

	MPIDI_Pkt_init(cts_pkt, MPIDI_CH3_PKT_RNDV_CLR_TO_SEND);
	cts_pkt->sender_req_id = rreq->dev.sender_req_id;
	cts_pkt->receiver_req_id = rreq->handle;
	mpi_errno = MPIDI_CH3_iStartMsg(vc, cts_pkt, sizeof(*cts_pkt), &cts_req);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**ch3|ctspkt");
	}
	/* --END ERROR HANDLING-- */
	if (cts_req != NULL)
	{
	    /* FIXME: Ideally we could specify that a req not be returned.  This would avoid our having to decrement the
	              reference count on a req we don't want/need. */
	    /* --BEGIN ERROR HANDLING-- */
	    if (cts_req->status.MPI_ERROR != MPI_SUCCESS)
	    {
		MPIU_Object_set_ref(rreq, 0);
		MPIDI_CH3_Request_destroy(rreq);
		mpi_errno = MPIR_Err_create_code(cts_req->status.MPI_ERROR, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|ctspkt", 0);
		MPID_Request_release(cts_req);
		goto fn_fail;
	    }
	    /* --END ERROR HANDLING-- */
	    MPID_Request_release(cts_req);
	}
    }

fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTRNDVTRANSFER);
    return mpi_errno;
}

#endif /*MPIDI_CH3_CHANNEL_RNDV*/
