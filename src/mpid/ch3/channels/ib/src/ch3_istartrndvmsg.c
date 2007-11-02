/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#ifdef MPIDI_CH3_CHANNEL_RNDV

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Switch_rndv_to_eager
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Switch_rndv_to_eager(MPIDI_VC_t * vc, MPID_Request * sreq, MPIDI_CH3_Pkt_t* pkt)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_IOV iov[MPID_IOV_LIMIT]; 
    int iov_n;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIDI_msg_sz_t data_sz;
    MPID_Datatype * dt_ptr = NULL;

    MPIDI_CH3_Pkt_rndv_eager_send_t * const rndv_eager_pkt = &(pkt->rndv_eager_send);

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SWITCH_RNDV_TO_EAGER);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SWITCH_RNDV_TO_EAGER);

    /* Switch to EAGER*/
    MPIU_DBG_PRINTF(("switching to EAGER from RNDV.\n"));
    MPIDI_Datatype_get_info(sreq->dev.user_count, sreq->dev.datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    /* Set the eager packet that is going to be sent */
    /*rndv_eager_pkt->type = MPIDI_CH3_PKT_RNDV_EAGER_SEND;*/
    rndv_eager_pkt->data_sz = data_sz;
    /* comm, tag check already filled and union's struct is in the same position */

    iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rndv_eager_pkt;
    iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);

    if (dt_contig)
    {
	MPIDI_DBG_PRINTF((30, FCNAME, "sending contiguous eager data, data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
	sreq->dev.ca = MPIDI_CH3_CA_COMPLETE;
	iov_n = 2;
	iov[1].MPID_IOV_BUF = sreq->dev.iov[0].MPID_IOV_BUF;
	iov[1].MPID_IOV_LEN = sreq->dev.iov[0].MPID_IOV_LEN;
    } 
    else 
    {
	/* sreq's segment initialization and size already set in MPID_xSend.
	   Need only to set the segment first to its position */
	sreq->dev.segment_first = 0;

	iov_n = MPID_IOV_LIMIT - 1; /* an entry is saved for iov[0] - pkt header */
	mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &iov[1], &iov_n);
	iov_n += 1; /* This is Eager, need to add the iov[0] to the count */
	if (mpi_errno != MPI_SUCCESS)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    MPIU_Object_set_ref(sreq, 0);
	    MPIDI_CH3_Request_destroy(sreq);
	    sreq = NULL;
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadsendiov", 0);
	    goto fn_exit;
	    /* --END ERROR HANDLING-- */
	}		
    }
    /* Send the message as EAGER now..*/
    mpi_errno = MPIDI_CH3_iSendv(vc, sreq, iov, iov_n);

    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_Object_set_ref(sreq, 0);
	MPIDI_CH3_Request_destroy(sreq);
	sreq = NULL;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rtspkt", 0);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SWITCH_RNDV_TO_EAGER);
    return mpi_errno;

}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartRndvMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartRndvMsg(MPIDI_VC_t * vc, MPID_Request * sreq, MPIDI_CH3_Pkt_t * rts_pkt)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * rts_sreq;
    int i;
    int ibu_reg_status = IBU_SUCCESS;
#ifdef USE_RDMA_GET
    MPIDI_CH3_Pkt_t pkt;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTRNDVMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTRNDVMSG);

#ifdef USE_RDMA_GET

    MPIDI_Pkt_init(&pkt.rts_iov, MPIDI_CH3_PKT_RTS_IOV);
    pkt.rts_iov.sreq = sreq->handle;
    pkt.rts_iov.iov_len = sreq->dev.iov_count;

    sreq->dev.rdma_iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&pkt;
    sreq->dev.rdma_iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    sreq->dev.rdma_iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)sreq->dev.iov;
    sreq->dev.rdma_iov[1].MPID_IOV_LEN = sreq->dev.iov_count * sizeof(MPID_IOV);
    sreq->dev.rdma_iov[2].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->ch.local_iov_mem[0];
    sreq->dev.rdma_iov[2].MPID_IOV_LEN = sreq->dev.iov_count * sizeof(ibu_mem_t);
    /* Save a copy of the rts packet in case it is not completely written by the iStartMsgv call */
    sreq->ch.pkt = *rts_pkt;
    sreq->dev.rdma_iov[3].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->ch.pkt;
    sreq->dev.rdma_iov[3].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);

    MPIU_DBG_PRINTF(("registering the sender's iov.\n"));
    sreq->ch.rndv_status = IBU_RNDV_NO_DEREG;
    for (i=0; i<sreq->dev.iov_count; i++)
    {
	sreq->ch.rndv_status = IBU_RNDV_SUCCESS;
	ibu_reg_status = ibu_register_memory(
	    sreq->dev.iov[i].MPID_IOV_BUF,
	    sreq->dev.iov[i].MPID_IOV_LEN,
	    &sreq->ch.local_iov_mem[i]);
	if (ibu_reg_status != IBU_SUCCESS) break;
    }
    if (ibu_reg_status != IBU_SUCCESS)
    {
	sreq->ch.rndv_status = IBU_RNDV_RTS_IOV_FAIL;
	/* Deregister all register buffers we are not going to use */ 
	MPIU_DBG_PRINTF(("ibu_register_memory failed. deregistering the sender's iov.\n"));
	if (i > 0) 
	{
	    for (i-=1; i==0; i--) /* take last i's value one down, since last did not succeed*/
	    {
		ibu_reg_status = ibu_deregister_memory(
		    sreq->dev.iov[i].MPID_IOV_BUF, 
		    sreq->dev.iov[i].MPID_IOV_LEN, 
		    &sreq->ch.local_iov_mem[i]);
		/* --BEGIN ERROR HANDLING-- */
		if (ibu_reg_status != IBU_SUCCESS)
		{
		    MPIU_Object_set_ref(sreq, 0);
		    MPIDI_CH3_Request_destroy(sreq);
		    sreq = NULL;
		    MPIU_DBG_PRINTF(("failed deregistering sender's iov.\n"));
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rtspkt", 0);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
	    }
	}

	mpi_errno = MPIDI_CH3_iStartMsg(vc, rts_pkt, sizeof(*rts_pkt), &rts_sreq);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_Object_set_ref(sreq, 0);
	    MPIDI_CH3_Request_destroy(sreq);
	    sreq = NULL;
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rtspkt", 0);
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */
	if (rts_sreq != NULL)
	{
	    MPID_Request_release(rts_sreq);
	}
	goto fn_exit;
    }


#ifdef MPICH_DBG_OUTPUT
    for (i=0; i<sreq->dev.iov_count; i++)
    {
	MPIU_DBG_PRINTF(("iStartRndvMsg: send buf[%d] = %p, len = %d\n",
			 i, sreq->dev.iov[i].MPID_IOV_BUF, sreq->dev.iov[i].MPID_IOV_LEN));
    }
#endif
    mpi_errno = MPIDI_CH3_iStartMsgv(vc, sreq->dev.rdma_iov, 4, &rts_sreq);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_Object_set_ref(sreq, 0);
	MPIDI_CH3_Request_destroy(sreq);
	sreq = NULL;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rtspkt", 0);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    if (rts_sreq != NULL)
    {
	/* The sender doesn't need to know when the message has been sent.
	   So release the request immediately */
	MPID_Request_release(rts_sreq);
    }

#else

#ifdef MPICH_DBG_OUTPUT
    for (i=0; i<sreq->dev.iov_count; i++)
    {
	MPIU_DBG_PRINTF(("iStartRndvMsg: send buf[%d] = %p, len = %d\n",
	    i, sreq->dev.iov[i].MPID_IOV_BUF, sreq->dev.iov[i].MPID_IOV_LEN));
    }
#endif
    sreq->ch.riov_offset = 0;
    rts_pkt->type = MPIDI_CH3_PKT_RTS_PUT;
    mpi_errno = MPIDI_CH3_iStartMsg(vc, rts_pkt, sizeof(*rts_pkt), &rts_sreq);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_Object_set_ref(sreq, 0);
	MPIDI_CH3_Request_destroy(sreq);
	sreq = NULL;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rtspkt", 0);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    if (rts_sreq != NULL)
    {
	/* The sender doesn't need to know when the packet has been sent.
	   So release the request immediately */
	MPID_Request_release(rts_sreq);
    }
    MPIU_DBG_PRINTF(("registering the sender's iov.\n"));
    sreq->ch.rndv_status = IBU_RNDV_NO_DEREG;
    for (i=0; i<sreq->dev.iov_count; i++)
    {
	sreq->ch.rndv_status = IBU_RNDV_SUCCESS;
	ibu_reg_status = ibu_register_memory(
	    sreq->dev.iov[i].MPID_IOV_BUF,
	    sreq->dev.iov[i].MPID_IOV_LEN,
	    &sreq->ch.local_iov_mem[i]);
	if (ibu_reg_status != IBU_SUCCESS) break;
    }
    if (ibu_reg_status != IBU_SUCCESS)
    {
	sreq->ch.rndv_status = IBU_RNDV_RTS_FAIL;
	/* Deregister all register buffers we are not going to use */ 
	MPIU_DBG_PRINTF(("ibu_register_memory failed. deregistering the sender's iov.\n"));
	if (i > 0) 
	{
	    for (i-=1; i==0; i--) /* take last i's value one down, since last did not succeed*/
	    {
		ibu_reg_status = ibu_deregister_memory(
		    sreq->dev.iov[i].MPID_IOV_BUF, 
		    sreq->dev.iov[i].MPID_IOV_LEN, 
		    &sreq->ch.local_iov_mem[i]);
		/* --BEGIN ERROR HANDLING-- */
		if (ibu_reg_status != IBU_SUCCESS)
		{
		    MPIU_Object_set_ref(sreq, 0);
		    MPIDI_CH3_Request_destroy(sreq);
		    sreq = NULL;
		    MPIU_DBG_PRINTF(("failed deregistering sender's iov.\n"));
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rtspkt", 0);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
	    }
	}		
    }

#endif

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTRNDVMSG);
    return mpi_errno;
}

#endif /*MPIDI_CH3_CHANNEL_RNDV*/
