/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This is code for handling unordered delivery of packets.  It is currently 
 * unused but saved for reference.
 */

#if defined(MPIDI_CH3_MSGS_UNORDERED)
int MPIDI_CH3U_Handle_unordered_recv_pkt(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt);
#define MPIDI_CH3U_Handle_unordered_recv_pkt MPIDI_CH3U_Handle_recv_pkt
#else
#define MPIDI_CH3U_Handle_ordered_recv_pkt MPIDI_CH3U_Handle_recv_pkt 
#endif

#if defined(MPIDI_CH3_MSGS_UNORDERED)

#define MPIDI_CH3U_Pkt_send_container_alloc() (MPIU_Malloc(sizeof(MPIDI_CH3_Pkt_send_container_t)))
#define MPIDI_CH3U_Pkt_send_container_free(pc_) MPIU_Free(pc_)

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_unordered_recv_pkt
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3U_Handle_unordered_recv_pkt(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t * pkt,
					 MPID_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_UNORDERED_RECV_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_UNORDERED_RECV_PKT);

    /* FIXME: This should probably be *rreqp = NULL? */
    rreqp = NULL;
    
    switch(pkt->type)
    {
	case MPIDI_CH3_PKT_EAGER_SEND:
	case MPIDI_CH3_PKT_EAGER_SYNC_SEND:
	case MPIDI_CH3_PKT_READY_SEND:
	case MPIDI_CH3_PKT_RNDV_REQ_TO_SEND:
	{
	    MPIDI_CH3_Pkt_send_t * send_pkt = (MPIDI_CH3_Pkt_send_t *) pkt;
	    MPIDI_CH3_Pkt_send_container_t * pc_cur;
	    MPIDI_CH3_Pkt_send_container_t * pc_last;
	    
	    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,
			 "received (potentially) out-of-order send pkt");
	    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
	          "rank=%d, tag=%d, context=%d seqnum=%d",
		  send_pkt->match.rank, send_pkt->match.tag, 
		  send_pkt->match.context_id, send_pkt->seqnum));
	    MPIU_DBG_MSG_FMAT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
              "vc - seqnum_send=%d seqnum_recv=%d reorder_msg_queue=0x%08lx",
	      vc->seqnum_send, vc->seqnum_recv, 
	      (unsigned long) vc->msg_reorder_queue));
	    
	    if (send_pkt->seqnum == vc->seqnum_recv)
	    {
		mpi_errno = MPIDI_CH3U_Handle_ordered_recv_pkt(vc, pkt, rreqp);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
		vc->seqnum_recv++;
		pc_cur = vc->msg_reorder_queue;
		while(pc_cur != NULL && vc->seqnum_recv == pc_cur->pkt.seqnum)
		{
		    pkt = (MPIDI_CH3_Pkt_t *) &pc_cur->pkt;
		    mpi_errno = MPIDI_CH3U_Handle_ordered_recv_pkt(vc, pkt, rreqp);
		    /* --BEGIN ERROR HANDLING-- */
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**ch3|pktordered", 0);
			goto fn_exit;
		    }
		    /* --END ERROR HANDLING-- */
		    vc->seqnum_recv++;
		    pc_last = pc_cur;
		    pc_cur = pc_cur->next;
		    MPIDI_CH3U_Pkt_send_container_free(pc_last);
		}
		vc->msg_reorder_queue = pc_cur;
	    }
	    else
	    {
		MPIDI_CH3_Pkt_send_container_t * pc_new;
	
		/* allocate container and copy packet */
		pc_new = MPIDI_CH3U_Pkt_send_container_alloc();
		/* --BEGIN ERROR HANDLING-- */
		if (pc_new == NULL)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**ch3|nopktcontainermem", 0);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
		pc_new->pkt = *send_pkt;

		/* insert packet into reorder queue */
		pc_last = NULL;
		pc_cur = vc->msg_reorder_queue;
		while (pc_cur != NULL)
		{
		    /* the current recv seqnum is subtracted from both the 
		       seqnums prior to comparision so as to remove any wrap
		       around effects. */
		    if (pc_new->pkt.seqnum - vc->seqnum_recv < 
			pc_cur->pkt.seqnum - vc->seqnum_recv)
		    {
			break;
		    }

		    pc_last = pc_cur;
		    pc_cur = pc_cur->next;
		}

		if (pc_last == NULL)
		{
		    pc_new->next = pc_cur;
		    vc->msg_reorder_queue = pc_new;
		}
		else
		{
		    pc_new->next = pc_cur;
		    pc_last->next = pc_new;
		}
	    }

	    break;
	}

	case MPIDI_CH3_PKT_CANCEL_SEND_REQ:
	{
	    /* --BEGIN ERROR HANDLING-- */
	    /* FIXME: processing send cancel requests requires that we be 
	       aware of pkts in the reorder queue */
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**ch3|ooocancelreq", 0);
	    goto fn_exit;
	    break;
	    /* --END ERROR HANDLING-- */
	}
	
	default:
	{
	    mpi_errno = MPIDI_CH3U_Handle_ordered_recv_pkt(vc, pkt, rreqp);
	    break;
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_UNORDERED_RECV_PKT);
    return mpi_errno;
}
#endif /* defined(MPIDI_CH3_MSGS_UNORDERED) */
