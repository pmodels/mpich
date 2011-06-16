/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif

/* Count the number of outstanding close requests */
static volatile int MPIDI_Outstanding_close_ops = 0;
int MPIDI_Failed_vc_count = 0;

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
  This routine is used to transition the VC state.

  The only events currently handled are TERMINATED events.  This
  routine should be called (with TERMINATED) whenever a connection is
  terminated whether normally (in MPIDI_CH3_Connection_terminate() ),
  or abnormally.

  FIXME: Currently state transitions resulting from receiving CLOSE
  packets are performed in MPIDI_CH3_PktHandler_Close().  Perhaps that
  should move here.
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
		case MPIDI_VC_STATE_CLOSED:
                    /* Normal termination. */
                    MPIDI_CHANGE_VC_STATE(vc, INACTIVE);

		    /* MT: this is not thread safe */
		    MPIDI_Outstanding_close_ops -= 1;
		    MPIU_DBG_MSG_D(CH3_DISCONNECT,TYPICAL,
             "outstanding close operations = %d", MPIDI_Outstanding_close_ops);
	    
		    if (MPIDI_Outstanding_close_ops == 0)
		    {
			MPIDI_CH3_Progress_signal_completion();
                        mpi_errno = MPIDI_CH3_Channel_close();
                        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
		    }

		    break;

                case MPIDI_VC_STATE_INACTIVE:
                    /* VC was terminated before it was activated.
                       This can happen if a failed process was
                       detected before the process used the VC. */
                    MPIU_DBG_MSG(CH3_DISCONNECT,TYPICAL, "VC terminated before it was activated.  We probably got a failed"
                                 " process notification.");
                    MPIDI_CH3U_Complete_posted_with_error(vc);
                    ++MPIDI_Failed_vc_count;
                    MPIDI_CHANGE_VC_STATE(vc, MORIBUND);

                    break;

                    
                case MPIDI_VC_STATE_ACTIVE:
                case MPIDI_VC_STATE_REMOTE_CLOSE:
                    /* This is a premature termination.  This process
                       has not started the close protocol.  There may
                       be outstanding sends or receives on the local
                       side, remote side or both. */
                    
 		    MPIU_DBG_MSG(CH3_DISCONNECT,TYPICAL, "Connection closed prematurely.");

                    MPIDI_CH3U_Complete_posted_with_error(vc);
                    ++MPIDI_Failed_vc_count;
                    
                    MPIDU_Ftb_publish_vc(MPIDU_FTB_EV_UNREACHABLE, vc);
                    MPIDI_CHANGE_VC_STATE(vc, MORIBUND);

                    break;
                    
                case MPIDI_VC_STATE_LOCAL_CLOSE:
                    /* This is a premature termination.  This process
                       has started the close protocol, but hasn't
                       received a CLOSE packet from the remote side.
                       This process may not have been able to send the
                       CLOSE ack=F packet to the remote side.  There
                       may be outstanding sends or receives on the
                       local or remote sides. */
                case MPIDI_VC_STATE_CLOSE_ACKED:
                    /* This is a premature termination.  Both sides
                       have started the close protocol.  This process
                       has received CLOSE ack=F, but not CLOSE ack=t.
                       This process may not have been able to send
                       CLOSE ack=T.  There should not be any
                       outstanding sends or receives on either
                       side. */

 		    MPIU_DBG_MSG_D(CH3_DISCONNECT,TYPICAL, "Connection closed prematurely during close protocol.  "
                                   "Outstanding close operations = %d", MPIDI_Outstanding_close_ops);

                    MPIDI_CH3U_Complete_posted_with_error(vc);
                    ++MPIDI_Failed_vc_count;

                    MPIDU_Ftb_publish_vc(MPIDU_FTB_EV_UNREACHABLE, vc);
                    MPIDI_CHANGE_VC_STATE(vc, MORIBUND);
                    
		    /* MT: this is not thread safe */
		    MPIDI_Outstanding_close_ops -= 1;
	    
		    if (MPIDI_Outstanding_close_ops == 0) {
			MPIDI_CH3_Progress_signal_completion();
                        mpi_errno = MPIDI_CH3_Channel_close();
                        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
		    }
                    
                    break;

		default:
		{
		    MPIU_DBG_MSG_D(CH3_DISCONNECT,TYPICAL, "Unhandled connection state %d when closing connection",vc->state);
		    mpi_errno = MPIR_Err_create_code(
			MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, 
                        MPI_ERR_INTERN, "**ch3|unhandled_connection_state",
			"**ch3|unhandled_connection_state %p %d", vc, vc->state);
                    goto fn_fail;
		    break;
		}
	    }

            /* FIXME: Decrement the reference count?  Who increments? */
            /* FIXME: The reference count is often already 0.  But
               not always */
            /* MPIU_Object_set_ref(vc, 0); ??? */

            /*
             * FIXME: The VC used in connect accept has a NULL 
             * process group
             */
            /* XXX DJG FIXME-MT should we be checking this ref_count? */
            if (vc->pg != NULL && (MPIU_Object_get_ref(vc) == 0))
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

    MPIU_THREAD_CS_ENTER(CH3COMM,vc);

    MPIU_Assert( vc->state == MPIDI_VC_STATE_ACTIVE ||
		 vc->state == MPIDI_VC_STATE_REMOTE_CLOSE );

    MPIDI_Pkt_init(close_pkt, MPIDI_CH3_PKT_CLOSE);
    close_pkt->ack = (vc->state == MPIDI_VC_STATE_ACTIVE) ? FALSE : TRUE;
    
    /* MT: this is not thread safe, the CH3COMM CS is scoped to the vc and
     * doesn't protect this global correctly */
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
        MPIDI_CHANGE_VC_STATE(vc, LOCAL_CLOSE);
    }
    else {
	MPIU_Assert( vc->state == MPIDI_VC_STATE_REMOTE_CLOSE );
        MPIDI_CHANGE_VC_STATE(vc, CLOSE_ACKED);
    }
		
    mpi_errno = MPIU_CALL(MPIDI_CH3,iStartMsg(vc, close_pkt, 
					      sizeof(*close_pkt), &sreq));
    MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|send_close_ack");
    
    if (sreq != NULL) {
	/* There is still another reference being held by the channel.  It
	   will not be released until the pkt is actually sent. */
	MPID_Request_release(sreq);
    }

 fn_exit:
    MPIU_THREAD_CS_EXIT(CH3COMM,vc);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_VC_SENDCLOSE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Here is the matching code that processes a close packet when it is 
   received */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Close
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
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
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|send_close_ack");
	
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
            MPIDI_CHANGE_VC_STATE(vc, CLOSE_ACKED);
	}
	else /* (vc->state == MPIDI_VC_STATE_ACTIVE) */
        {
	    /* FIXME: Debugging */
	    if (vc->state != MPIDI_VC_STATE_ACTIVE) {
		printf( "Unexpected state %s in vc %p (expecting MPIDI_VC_STATE_ACTIVE)\n", MPIDI_VC_GetStateString(vc->state), vc );
		fflush(stdout);
	    }
	    MPIU_DBG_MSG_D(CH3_DISCONNECT,TYPICAL,
                     "received close(FALSE) from %d, moving to REMOTE_CLOSE.",
				   vc->pg_rank);
            
	    MPIU_Assert(vc->state == MPIDI_VC_STATE_ACTIVE);
            MPIDI_CHANGE_VC_STATE(vc, REMOTE_CLOSE);
	}
    }
    else /* (close_pkt->ack == TRUE) */
    {
	MPIU_DBG_MSG_D(CH3_DISCONNECT,TYPICAL,
                       "received close(TRUE) from %d, moving to CLOSED.", 
			       vc->pg_rank);
	MPIU_Assert (vc->state == MPIDI_VC_STATE_LOCAL_CLOSE || 
		     vc->state == MPIDI_VC_STATE_CLOSE_ACKED);
        MPIDI_CHANGE_VC_STATE(vc, CLOSED);
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
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**ch3|close_progress");
	    break;
	}
	/* --END ERROR HANDLING-- */
    }
    MPID_Progress_end(&progress_state);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_VC_WAITFORCLOSE);
    return mpi_errno;
}

#define parse_rank(r_p) do {                                                                    \
        while (isspace(*c)) /* skip spaces */                                                   \
            ++c;                                                                                \
        MPIU_ERR_CHKINTERNAL(!isdigit(*c), mpi_errno, "error parsing failed process list");     \
        *(r_p) = strtol(c, &c, 0);                                                              \
        while (isspace(*c)) /* skip spaces */                                                   \
            ++c;                                                                                \
    } while (0)

#define ALLOC_STEP 10

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Check_for_failed_procs
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_CH3U_Check_for_failed_procs(void)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    char *val;
    int *attr_val = NULL, *ret;
    char *c;
    int len;
    char *kvsname;
    int rank, rank_hi;
    int i;
    int alloc_len;
    MPIU_CHKLMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_CHECK_FOR_FAILED_PROCS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_CHECK_FOR_FAILED_PROCS);
    mpi_errno = MPIDI_PG_GetConnKVSname(&kvsname);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
#ifdef USE_PMI2_API
    {
        int vallen = 0;
        MPIU_CHKLMEM_MALLOC(val, char *, PMI2_MAX_VALLEN, mpi_errno, "val");
        pmi_errno = PMI2_KVS_Get(kvsname, PMI2_ID_NULL, "PMI_dead_processes", val, PMI2_MAX_VALLEN, &vallen);
        MPIU_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get");
    }
#else
    pmi_errno = PMI_KVS_Get_value_length_max(&len);
    MPIU_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_value_length_max");
    MPIU_CHKLMEM_MALLOC(val, char *, len, mpi_errno, "val");
    pmi_errno = PMI_KVS_Get(kvsname, "PMI_dead_processes", val, len);
    MPIU_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get");
#endif
    
    MPIU_DBG_MSG_S(CH3_DISCONNECT, TYPICAL, "Received proc fail notification: %s", val);
    
    if (*val == '\0') {
        /* there are no failed processes */
        attr_val = MPIU_Malloc(sizeof(int));
        if (!attr_val) { MPIU_CHKMEM_SETERR(mpi_errno, sizeof(int), "attr_val"); goto fn_fail; }
        *attr_val = MPI_PROC_NULL;
        mpi_errno = MPIR_Comm_set_attr_impl(MPIR_Process.comm_world, MPICH_ATTR_FAILED_PROCESSES, attr_val, MPIR_ATTR_PTR);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    attr_val = MPIU_Malloc(sizeof(int) * ALLOC_STEP);
    alloc_len = ALLOC_STEP;
    
    /* parse list of failed processes.  This is a comma separated list
       of ranks or ranges of ranks (e.g., "1, 3-5, 11") */
    i = 0;
    c = val;
    while(1) {
        parse_rank(&rank);
        if (*c == '-') {
            ++c; /* skip '-' */
            parse_rank(&rank_hi);
        } else
            rank_hi = rank;
        while (rank <= rank_hi) {
            MPIDI_VC_t *vc;
            /* terminate the VC */
            MPIDI_PG_Get_vc(MPIDI_Process.my_pg, rank, &vc);
            mpi_errno = MPIU_CALL(MPIDI_CH3,Connection_terminate(vc));
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            /* update the dead node attribute list */
            if (alloc_len <= i) {
                /* allocate more space */
                ret = MPIU_Realloc(attr_val, sizeof(int) * (alloc_len + ALLOC_STEP));
                if (!ret) { MPIU_CHKMEM_SETERR(mpi_errno, sizeof(int) * (alloc_len + ALLOC_STEP), "attr_val"); goto fn_fail; }
                attr_val = ret;
            }
            attr_val[i] = rank;
            ++i;
            ++rank;
        }
        MPIU_ERR_CHKINTERNAL(*c != ',' && *c != '\0', mpi_errno, "error parsing failed process list");
        if (*c == '\0')
            break;
        ++c; /* skip ',' */
    }
    /* terminate dead node attribute list with an MPI_PROC_NULL */
    if (alloc_len <= i) {
        /* allocate more space */
        ret = MPIU_Realloc(attr_val, alloc_len + ALLOC_STEP);
        if (!ret) { MPIU_CHKMEM_SETERR(mpi_errno, alloc_len + ALLOC_STEP, attr_val); goto fn_fail; }
        attr_val = ret;
    }
    attr_val[i] = MPI_PROC_NULL;

    mpi_errno = MPIR_Comm_set_attr_impl(MPIR_Process.comm_world, MPICH_ATTR_FAILED_PROCESSES, attr_val, MPIR_ATTR_PTR);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_CHECK_FOR_FAILED_PROCS);
    return mpi_errno;
 fn_fail:
    if (attr_val)
        MPIU_Free(attr_val);
    goto fn_exit;
}

/* for debugging */
int MPIDI_CH3U_Dump_vc_states(void);
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Dump_vc_states
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_CH3U_Dump_vc_states(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    printf("VC States\n");
    for (i = 0; i < MPIDI_Process.my_pg->size; ++i)
        printf("  %3d   %s\n", i, MPIDI_VC_GetStateString(MPIDI_Process.my_pg->vct[i].state));
        
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
