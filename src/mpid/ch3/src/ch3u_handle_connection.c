/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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
#undef utarray_oom
#define utarray_oom() do { goto fn_oom; } while (0)
#include "mpiu_utarray.h"

/* Count the number of outstanding close requests */
static volatile int MPIDI_Outstanding_close_ops = 0;
int MPIDI_Failed_vc_count = 0;

MPID_Group *MPIDI_Failed_procs_group = NULL;
int MPIDI_last_known_failed = MPI_PROC_NULL;
char *MPIDI_failed_procs_string = NULL;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_connection
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
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
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);

    MPIU_Assert( vc->state == MPIDI_VC_STATE_ACTIVE ||
		 vc->state == MPIDI_VC_STATE_REMOTE_CLOSE );

    MPIDI_Pkt_init(close_pkt, MPIDI_CH3_PKT_CLOSE);
    close_pkt->ack = (vc->state == MPIDI_VC_STATE_ACTIVE) ? FALSE : TRUE;
    
    /* MT: this is not thread safe, the POBJ CS is scoped to the vc and
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
		
    mpi_errno = MPIDI_CH3_iStartMsg(vc, close_pkt, sizeof(*close_pkt), &sreq);
    MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|send_close_ack");
    
    if (sreq != NULL) {
	/* There is still another reference being held by the channel.  It
	   will not be released until the pkt is actually sent. */
	MPID_Request_release(sreq);
    }

 fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);

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
#define FCNAME MPL_QUOTE(FUNCNAME)
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
	mpi_errno = MPIDI_CH3_iStartMsg(vc, resp_pkt, sizeof(*resp_pkt), &resp_sreq);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|send_close_ack");
	
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
	    if (vc->state != MPIDI_VC_STATE_ACTIVE)
            {
		MPIU_DBG_MSG_FMT(CH3_DISCONNECT, TYPICAL, (MPIU_DBG_FDEST, "Unexpected state %s in vc %p (rank=%d) (expecting MPIDI_VC_STATE_ACTIVE)\n", MPIDI_VC_GetStateString(vc->state), vc, vc->pg_rank ));
	    MPIU_DBG_MSG_D(CH3_DISCONNECT,TYPICAL,
                     "received close(FALSE) from %d, moving to REMOTE_CLOSE.",
				   vc->pg_rank);
            }
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
	mpi_errno = MPIDI_CH3_Connection_terminate(vc);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
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
	    MPIR_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**ch3|close_progress");
	    break;
	}
	/* --END ERROR HANDLING-- */
    }
    MPID_Progress_end(&progress_state);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_VC_WAITFORCLOSE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME terminate_failed_VCs
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int terminate_failed_VCs(MPID_Group *new_failed_group)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_TERMINATE_FAILED_VCS);

    MPIDI_FUNC_ENTER(MPID_STATE_TERMINATE_FAILED_VCS);

    for (i = 0; i < new_failed_group->size; ++i) {
        MPIDI_VC_t *vc;
        /* terminate the VC */
        /* FIXME: This won't work for dynamic procs */
        MPIDI_PG_Get_vc(MPIDI_Process.my_pg, new_failed_group->lrank_to_lpid[i].lpid, &vc);
        mpi_errno = MPIDI_CH3_Connection_terminate(vc);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_TERMINATE_FAILED_VCS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#define parse_rank(r_p) do {                                                                    \
        while (isspace(*c)) /* skip spaces */                                                   \
            ++c;                                                                                \
        MPIR_ERR_CHKINTERNAL(!isdigit(*c), mpi_errno, "error parsing failed process list");     \
        *(r_p) = (int)strtol(c, &c, 0);                                                         \
        while (isspace(*c)) /* skip spaces */                                                   \
            ++c;                                                                                \
    } while (0)

/* There are three possible input values for `last_rank:
 *
 * < -1 = All failures regardless of acknowledgement
 * -1 (MPI_PROC_NULL) = No failures have been acknowledged yet (return MPID_Group_empty)
 * >= 0 = The last failure acknowledged. All failures returned will have
 *        been acknowledged previously.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Get_failed_group
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3U_Get_failed_group(int last_rank, MPID_Group **failed_group)
{
    char *c;
    int i, mpi_errno = MPI_SUCCESS, rank;
    UT_array *failed_procs = NULL;
    MPID_Group *world_group;
    MPIDI_STATE_DECL(MPID_STATE_GET_FAILED_GROUP);

    MPIDI_FUNC_ENTER(MPID_STATE_GET_FAILED_GROUP);

    MPIU_DBG_MSG_D(CH3_OTHER, VERBOSE, "Getting failed group with %d as last acknowledged\n", last_rank);

    if (-1 == last_rank) {
        MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "No failure acknowledged");
        *failed_group = MPID_Group_empty;
        goto fn_exit;
    }

    if (*MPIDI_failed_procs_string == '\0') {
        MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "Found no failed ranks");
        *failed_group = MPID_Group_empty;
        goto fn_exit;
    }

    utarray_new(failed_procs, &ut_int_icd);

    /* parse list of failed processes.  This is a comma separated list
       of ranks or ranges of ranks (e.g., "1, 3-5, 11") */
    i = 0;
    c = MPIDI_failed_procs_string;
    while(1) {
        parse_rank(&rank);
        ++i;
        MPIU_DBG_MSG_D(CH3_OTHER, VERBOSE, "Found failed rank: %d", rank);
        utarray_push_back(failed_procs, &rank);
        MPIDI_last_known_failed = rank;
        MPIR_ERR_CHKINTERNAL(*c != ',' && *c != '\0', mpi_errno, "error parsing failed process list");
        if (*c == '\0' || last_rank == rank)
            break;
        ++c; /* skip ',' */
    }

    /* Create group of failed processes for comm_world.  Failed groups for other
       communicators can be created from this one using group_intersection. */
    mpi_errno = MPIR_Comm_group_impl(MPIR_Process.comm_world, &world_group);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Group_incl_impl(world_group, i, ut_int_array(failed_procs), failed_group);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Group_release(world_group);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_GET_FAILED_GROUP);
    if (failed_procs)
        utarray_free(failed_procs);
    return mpi_errno;
fn_oom:
    MPIR_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "utarray");
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Check_for_failed_procs
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3U_Check_for_failed_procs(void)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int len;
    char *kvsname;
    MPID_Group *prev_failed_group, *new_failed_group;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_CHECK_FOR_FAILED_PROCS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_CHECK_FOR_FAILED_PROCS);

    /* FIXME: Currently this only handles failed processes in
       comm_world.  We need to fix hydra to include the pgid along
       with the rank, then we need to create the failed group from
       something bigger than comm_world. */
    mpi_errno = MPIDI_PG_GetConnKVSname(&kvsname);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#ifdef USE_PMI2_API
    {
        int vallen = 0;
        pmi_errno = PMI2_KVS_Get(kvsname, PMI2_ID_NULL, "PMI_dead_processes", MPIDI_failed_procs_string, PMI2_MAX_VALLEN, &vallen);
        MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get");
    }
#else
    pmi_errno = PMI_KVS_Get_value_length_max(&len);
    MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_value_length_max");
    pmi_errno = PMI_KVS_Get(kvsname, "PMI_dead_processes", MPIDI_failed_procs_string, len);
    MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get");
#endif

    if (*MPIDI_failed_procs_string == '\0') {
        /* there are no failed processes */
        MPIDI_Failed_procs_group = MPID_Group_empty;
        goto fn_exit;
    }

    MPIU_DBG_MSG_S(CH3_OTHER, TYPICAL, "Received proc fail notification: %s", MPIDI_failed_procs_string);

    /* save reference to previous group so we can identify new failures */
    prev_failed_group = MPIDI_Failed_procs_group;

    /* Parse the list of failed processes */
    MPIDI_CH3U_Get_failed_group(-2, &MPIDI_Failed_procs_group);

    /* get group of newly failed processes */
    mpi_errno = MPIR_Group_difference_impl(MPIDI_Failed_procs_group, prev_failed_group, &new_failed_group);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    if (new_failed_group != MPID_Group_empty) {
        mpi_errno = MPIDI_CH3I_Comm_handle_failed_procs(new_failed_group);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        mpi_errno = terminate_failed_VCs(new_failed_group);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Group_release(new_failed_group);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    /* free prev group */
    if (prev_failed_group != MPID_Group_empty) {
        mpi_errno = MPIR_Group_release(prev_failed_group);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_CHECK_FOR_FAILED_PROCS);
    return mpi_errno;

 fn_oom: /* out-of-memory handler for utarray operations */
    MPIR_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "utarray");
 fn_fail:
    goto fn_exit;
}

/* for debugging */
int MPIDI_CH3U_Dump_vc_states(void);
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Dump_vc_states
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
