/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* Count the number of outstanding close requests */
static volatile int MPIDI_Outstanding_close_ops = 0;

/* FIXME: What is this routine for?
   It appears to be used only in ch3_progress, ch3_progress_connect, or
   ch3_progress_sock files.  Is this a general operation, or does it 
   belong in util/sock ? It appears to be used in multiple channels, 
   but probably belongs in mpid_vc, along with the vc exit code that 
   is currently in MPID_Finalize */

/* FIXME: The only event is event_terminated.  Should this have 
   a different name/expected function? */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_connection
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/*@
  MPIDI_CH3U_Handle_connection - handle connection event

  Input Parameters:
+ vc - virtual connection
. event - connection event

  NOTE:
  At present this function is only used for connection termination
@*/
int MPIDI_CH3U_Handle_connection(MPIDI_VC_t * vc, MPIDI_VC_Event_t event)
{
    int inuse;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_CONNECTION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_CONNECTION);

    switch (event)
    {
	case MPIDI_VC_EVENT_TERMINATED:
	{
	    switch (vc->state)
	    {
		case MPIDI_VC_STATE_CLOSE_ACKED:
		{
		    MPIU_DBG_VCSTATECHANGE(vc,VC_STATE_INACTIVE);
		    vc->state = MPIDI_VC_STATE_INACTIVE;
		    /* FIXME: Decrement the reference count?  Who increments? */
		    /* FIXME: The reference count is often already 0.  But
		       not always */
		    /* MPIU_Object_set_ref(vc, 0); ??? */

                    mpi_errno = MPIDI_CH3_Handle_vc_close(vc);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

		    /*
		     * FIXME: The VC used in connect accept has a NULL 
		     * process group
		     */
		    if (vc->pg != NULL && vc->ref_count == 0)
		    { 
			/* FIXME: Who increments the reference count that
			   this is decrementing? */
			/* When the reference count for a vc becomes zero, 
			   decrement the reference count
			   of the associated process group.  */
			/* FIXME: This should be done when the reference 
			   count of the vc is first decremented */
			MPIDI_PG_release_ref(vc->pg, &inuse);
			if (inuse == 0) {
			    MPIDI_PG_Destroy(vc->pg);
			}
		    }

		    /* MT: this is not thread safe */
		    MPIDI_Outstanding_close_ops -= 1;
		    MPIU_DBG_MSG_D(CH3_DISCONNECT,TYPICAL,
             "outstanding close operations = %d", MPIDI_Outstanding_close_ops);
	    
		    if (MPIDI_Outstanding_close_ops == 0)
		    {
			MPIDI_CH3_Progress_signal_completion();
                        MPIDI_CH3_Channel_close();
		    }

		    break;
		}

		default:
		{
		    MPIU_DBG_MSG_D(CH3_DISCONNECT,TYPICAL,
           "Unhandled connection state %d when closing connection",vc->state);
		    mpi_errno = MPIR_Err_create_code(
			MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, 
                        MPI_ERR_INTERN, "**ch3|unhandled_connection_state",
			"**ch3|unhandled_connection_state %p %d", vc, event);
		    break;
		}
	    }

	    break;
	}
    
	default:
	{
	    break;
	}
    }

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_CONNECTION);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_VC_SendClose
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/*@
  MPIDI_CH3U_VC_SendClose - Initiate a close on a virtual connection
  
  Input Parameters:
+ vc - Virtual connection to close
- i  - rank of virtual connection within a process group (used for debugging)

  Notes:
  The current state of this connection must be either 'MPIDI_VC_STATE_ACTIVE' 
  or 'MPIDI_VC_STATE_REMOTE_CLOSE'.  
  @*/
int MPIDI_CH3U_VC_SendClose( MPIDI_VC_t *vc, int rank )
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_close_t * close_pkt = &upkt.close;
    MPID_Request * sreq;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_VC_SENDCLOSE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_VC_SENDCLOSE);

    /* FIXME: Remove this IFDEF */
#if defined(MPIDI_CH3_USES_SSHM) && 0
    MPIU_Assert( vc->state == MPIDI_VC_STATE_ACTIVE || 
		 vc->state == MPIDI_VC_STATE_REMOTE_CLOSE 
		 /* sshm queues are uni-directional.  A VC that is connected 
		 * in the read direction is marked MPIDI_VC_STATE_INACTIVE
		 * so that a connection will be formed on the first write.  
		 * Since the other side is marked MPIDI_VC_STATE_ACTIVE for 
		 * writing 
		 * we need to initiate the close protocol on the read side 
		 * even if the write state is MPIDI_VC_STATE_INACTIVE. */
		 || ((vc->state == MPIDI_VC_STATE_INACTIVE) && 
		     ((MPIDI_CH3I_VC *)(vc->channel_private))->shm_read_connected) );
#else
    MPIU_Assert( vc->state == MPIDI_VC_STATE_ACTIVE || 
		 vc->state == MPIDI_VC_STATE_REMOTE_CLOSE );
#endif

    MPIDI_Pkt_init(close_pkt, MPIDI_CH3_PKT_CLOSE);
    close_pkt->ack = (vc->state == MPIDI_VC_STATE_ACTIVE) ? FALSE : TRUE;
    
    /* MT: this is not thread safe */
    MPIDI_Outstanding_close_ops += 1;
    MPIU_DBG_MSG_FMT(CH3_DISCONNECT,TYPICAL,(MPIU_DBG_FDEST,
		  "sending close(%s) on vc (pg=%p) %p to rank %d, ops = %d", 
		  close_pkt->ack ? "TRUE" : "FALSE", vc->pg, vc, 
		  rank, MPIDI_Outstanding_close_ops));
		    

    /*
     * A close packet acknowledging this close request could be
     * received during iStartMsg, therefore the state must
     * be changed before the close packet is sent.
     */
    if (vc->state == MPIDI_VC_STATE_ACTIVE) {
	MPIU_DBG_VCSTATECHANGE(vc,VC_STATE_LOCAL_CLOSE);
	vc->state = MPIDI_VC_STATE_LOCAL_CLOSE;
    }
    else {
	MPIU_Assert( vc->state == MPIDI_VC_STATE_REMOTE_CLOSE );
	MPIU_DBG_VCSTATECHANGE(vc,VC_STATE_CLOSE_ACKED);
	vc->state = MPIDI_VC_STATE_CLOSE_ACKED;
    }
		
    mpi_errno = MPIU_CALL(MPIDI_CH3,iStartMsg(vc, close_pkt, 
					      sizeof(*close_pkt), &sreq));
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,
		     "**ch3|send_close_ack");
    }
    
    if (sreq != NULL) {
	/* There is still another reference being held by the channel.  It
	   will not be released until the pkt is actually sent. */
	MPID_Request_release(sreq);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_VC_SENDCLOSE);
    return mpi_errno;
}

/* Here is the matching code that processes a close packet when it is 
   received */
int MPIDI_CH3_PktHandler_Close( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, 
				MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_close_t * close_pkt = &pkt->close;
    int mpi_errno = MPI_SUCCESS;
	    
    if (vc->state == MPIDI_VC_STATE_LOCAL_CLOSE)
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_close_t * resp_pkt = &upkt.close;
	MPID_Request * resp_sreq;
	
	MPIDI_Pkt_init(resp_pkt, MPIDI_CH3_PKT_CLOSE);
	resp_pkt->ack = TRUE;
	
	MPIU_DBG_MSG_D(CH3_DISCONNECT,TYPICAL,"sending close(TRUE) to %d",
		       vc->pg_rank);
	mpi_errno = MPIU_CALL(MPIDI_CH3,iStartMsg(vc, resp_pkt, 
					  sizeof(*resp_pkt), &resp_sreq));
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
				"**ch3|send_close_ack");
	}
	
	if (resp_sreq != NULL)
	{
	    /* There is still another reference being held by the channel.  It
	       will not be released until the pkt is actually sent. */
	    MPID_Request_release(resp_sreq);
	}
    }
    
    if (close_pkt->ack == FALSE)
    {
	if (vc->state == MPIDI_VC_STATE_LOCAL_CLOSE)
	{
	    MPIU_DBG_MSG_D(CH3_DISCONNECT,TYPICAL,
		   "received close(FALSE) from %d, moving to CLOSE_ACKED.",
		   vc->pg_rank);
	    MPIU_DBG_VCSTATECHANGE(vc,VC_STATE_CLOSE_ACKED);
	    vc->state = MPIDI_VC_STATE_CLOSE_ACKED;
	}
#if 0
	else if (vc->state == MPIDI_VC_STATE_CLOSE_ACKED) {
	    /* FIXME: This situation has been seen with the ssm device,
	       when running on a single process.  It may indicate that the
	       close protocol, for shared memory connections, can
	       occasionally reach this state when both sides start
	       closing the connection.  We will act as if this is
	       duplicate information can be ignored (rather than triggering
	       the Assert in the next case) */
	    MPIU_DBG_MSG(CH3_DISCONNECT,TYPICAL,
			 "Saw CLOSE_ACKED while already in that state");
	    vc->state = MPIDI_VC_STATE_REMOTE_CLOSE;
	    /* We need this terminate to decrement the outstanding closes */
	    /* For example, with sockets, Connection_terminate will close
	       the socket */
	    mpi_errno = MPIU_CALL(MPIDI_CH3,Connection_terminate(vc));
	}
#endif
	else /* (vc->state == MPIDI_VC_STATE_ACTIVE) */
	{
	    /* FIXME: Debugging */
	    if (vc->state != MPIDI_VC_STATE_ACTIVE) {
		printf( "Unexpected state %d in vc %p\n", vc->state, vc );
		fflush(stdout);
	    }
	    MPIU_DBG_MSG_D(CH3_DISCONNECT,TYPICAL,
                     "received close(FALSE) from %d, moving to REMOTE_CLOSE.",
				   vc->pg_rank);
	    MPIU_Assert(vc->state == MPIDI_VC_STATE_ACTIVE);
	    MPIU_DBG_VCSTATECHANGE(vc,VC_STATE_REMOTE_CLOSE);
	    vc->state = MPIDI_VC_STATE_REMOTE_CLOSE;
	}
    }
    else /* (close_pkt->ack == TRUE) */
    {
	MPIU_DBG_MSG_D(CH3_DISCONNECT,TYPICAL,
                       "received close(TRUE) from %d, moving to CLOSE_ACKED.", 
			       vc->pg_rank);
	MPIU_Assert (vc->state == MPIDI_VC_STATE_LOCAL_CLOSE || 
		     vc->state == MPIDI_VC_STATE_CLOSE_ACKED);
	MPIU_DBG_VCSTATECHANGE(vc,VC_STATE_CLOSE_ACKED);
	
	vc->state = MPIDI_VC_STATE_CLOSE_ACKED;
	/* For example, with sockets, Connection_terminate will close
	   the socket */
	mpi_errno = MPIU_CALL(MPIDI_CH3,Connection_terminate(vc));
    }
    
    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;

 fn_fail:
    return mpi_errno;
}

#ifdef MPICH_DBG_OUTPUT
int MPIDI_CH3_PktPrint_Close( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_CLOSE\n"));
    MPIU_DBG_PRINTF((" ack ......... %s\n", pkt->close.ack ? "TRUE" : "FALSE"));
    return MPI_SUCCESS;
}
#endif

/* 
 * This routine can be called to progress until all pending close operations
 * (initiated in the SendClose routine above) are completed.  It is 
 * used in MPID_Finalize and MPID_Comm_disconnect.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_VC_WaitForClose
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/*@
  MPIDI_CH3U_VC_WaitForClose - Wait for all virtual connections to close
  @*/
int MPIDI_CH3U_VC_WaitForClose( void )
{
    MPID_Progress_state progress_state;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_VC_WAITFORCLOSE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_VC_WAITFORCLOSE);

    MPID_Progress_start(&progress_state);
    while(MPIDI_Outstanding_close_ops > 0) {
	MPIU_DBG_MSG_D(CH3_DISCONNECT,TYPICAL,
		       "Waiting for %d close operations",
		       MPIDI_Outstanding_close_ops);
	mpi_errno = MPID_Progress_wait(&progress_state);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS) {
	    MPID_Progress_end(&progress_state);
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**ch3|close_progress");
	    break;
	}
	/* --END ERROR HANDLING-- */
    }
    MPID_Progress_end(&progress_state);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_VC_WAITFORCLOSE);
    return mpi_errno;
}

