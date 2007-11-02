/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

int MPIDI_CH3I_inside_handler;
gasnet_token_t MPIDI_CH3I_gasnet_token;

volatile unsigned int MPIDI_CH3I_progress_completions = 0;

struct MPID_Request *MPIDI_CH3I_sendq_head[CH3_NUM_QUEUES];
struct MPID_Request *MPIDI_CH3I_sendq_tail[CH3_NUM_QUEUES];
struct MPID_Request *MPIDI_CH3I_active_send[CH3_NUM_QUEUES];

static int send_enqueuedv (MPIDI_VC_t *vc, MPID_Request *sreq);
static int do_put (MPIDI_VC_t * vc, MPID_Request *sreq);

#if !defined(MPIDI_CH3_Progress_start)
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_start
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Progress_start(MPID_Progress_state * state)
{
    /* MT - This function is empty for the single-threaded implementation */
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_START);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_START);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_START);
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress(int is_blocking)
{
    int rc;
    unsigned completions = MPIDI_CH3I_progress_completions;
    int mpi_errno = MPI_SUCCESS;
    int complete;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS);

#if defined(MPICH_DBG_OUTPUT)
    {
	if (is_blocking)
	{
	    MPIDI_DBG_PRINTF((50, FCNAME, "entering, blocking=%s",
			      is_blocking ? "true" : "false"));
	}
    }
#endif

    /* we better not be inside the handler: we can't send from inside
     * the handler, so we wouldn't be able to make progress */
    MPIU_Assert (!MPIDI_CH3I_inside_handler);

    do
    {
	MPID_Request *sreq;
	gasnet_AMPoll ();
	sreq = MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE];
	if (sreq)
	{
	    complete = 0;
	    mpi_errno = MPIDI_CH3_iWrite (sreq->gasnet.vc, sreq);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		/* currently iwrite always returns MPI_SUCCESS */
		MPID_Abort(NULL, mpi_errno, -1, "Internal error");
	    }
	    
	    if (sreq->dev.iov_count == 0)
	    {
		mpi_errno = MPIDI_CH3U_Handle_send_req (sreq->gasnet.vc,
							sreq, &complete);
		if (mpi_errno != MPI_SUCCESS)
		{
		    MPID_Abort(NULL, mpi_errno, -1, "Internal error");
		}
	    }
	    
	    if (complete)
	    {
		MPIDI_CH3I_SendQ_dequeue (CH3_NORMAL_QUEUE);
		MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] = NULL;
	    }
	}
	else
	{
	    sreq = MPIDI_CH3I_SendQ_head (CH3_NORMAL_QUEUE);
	    if (sreq)
	    {
		complete = 0;
		mpi_errno = send_enqueuedv (sreq->gasnet.vc, sreq);
		if (mpi_errno != MPI_SUCCESS)
		{
		    MPID_Abort(NULL, mpi_errno, -1, "Internal error");
		}
		if (sreq->dev.iov_count == 0)
		{
		    mpi_errno = MPIDI_CH3U_Handle_send_req (sreq->gasnet.vc,
							    sreq, &complete);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			MPID_Abort(NULL, mpi_errno, -1, "Internal error");
		    }
		}
		
		if (complete)
		{
		    MPIDI_CH3I_SendQ_dequeue (CH3_NORMAL_QUEUE);
		}
		else
		{
		    MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] = sreq;
		}
	    }
	}

	/* handle rendezvous puts */
	sreq = MPIDI_CH3I_SendQ_head (CH3_RNDV_QUEUE);
	if (sreq)
	{
	    MPIDI_DBG_PRINTF((55, FCNAME, "handle rendezvous puts\n"));
	    DUMP_REQUEST(sreq);
	    switch (sreq->gasnet.rndv_state)
	    {
	    case MPIDI_CH3_RNDV_NEW:
		gasnet_begin_nbi_accessregion ();
	    case MPIDI_CH3_RNDV_CURRENT:
		do_put (sreq->gasnet.vc, sreq);
		if (sreq->gasnet.remote_iov_count == 0 ||
		    sreq->dev.iov_count == 0)
		{
		    sreq->gasnet.rndv_state = MPIDI_CH3_RNDV_WAIT;
		    sreq->gasnet.rndv_handle = gasnet_end_nbi_accessregion ();
		}
		else
		{
		    sreq->gasnet.rndv_state = MPIDI_CH3_RNDV_CURRENT;
		}
		break;
	    case MPIDI_CH3_RNDV_WAIT:
		if (gasnet_try_syncnb (sreq->gasnet.rndv_handle) == GASNET_OK)
		{
		    if (sreq->gasnet.remote_iov_count == 0)
		    {
			int gn_errno;
			gn_errno =
			    gasnet_AMRequestShort2 (sreq->gasnet.vc->lpid,
						    MPIDI_CH3_reload_IOV_or_done_handler_id,
						    sreq->gasnet.remote_req_id,
						    sreq->handle);
			if (gn_errno != GASNET_OK)
			{
			    MPID_Abort (NULL, MPI_SUCCESS, -1,
					"GASNet send failed");
			}
			
			MPIDI_CH3I_SendQ_dequeue (CH3_RNDV_QUEUE);
		    }
		    
		    if (sreq->dev.iov_count == 0)
		    {
			MPIDI_CH3U_Handle_send_req (sreq->gasnet.vc, sreq,
						    &complete);
			sreq->gasnet.iov_offset = 0;
		    }
		}
		break;
	    default:
		MPID_Abort (NULL, MPI_SUCCESS, -1, "Internal error");
	    }    
	}
    }
    while (completions == MPIDI_CH3I_progress_completions && is_blocking);
    
#if defined(MPICH_DBG_OUTPUT)
    {
	if (is_blocking)
	{
	    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
	}
    }
#endif
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_start_packet_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void
MPIDI_CH3_start_packet_handler (gasnet_token_t token, void* buf, size_t data_sz)
{
    int mpi_errno;
    int gn_errno;
    gasnet_node_t sender;
    MPIDI_VC_t *vc;
    MPID_Request *rreq;
    
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_START_PACKET_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_START_PACKET_HANDLER);
    
    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    MPIDI_CH3I_inside_handler = 1;
    MPIDI_CH3I_gasnet_token = token;
    gn_errno = gasnet_AMGetMsgSource (token, &sender);
    if (gn_errno != GASNET_OK)
    {
	MPID_Abort(NULL, MPI_SUCCESS, -1, "GASNet AMGetMesgSource failed");
    }
    MPIDI_DBG_PRINTF((55, FCNAME, "  sender = %d\n", sender));
    MPIDI_PG_Get_vc(MPIDI_Process.my_pg, sender, &vc);
    vc->gasnet.data = (void *) ((char *)buf + sizeof (MPIDI_CH3_Pkt_t));
    vc->gasnet.data_sz = data_sz - sizeof (MPIDI_CH3_Pkt_t);
    
    mpi_errno = MPIDI_CH3U_Handle_recv_pkt (vc, (MPIDI_CH3_Pkt_t *)buf, &rreq);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPID_Abort(NULL, mpi_errno, -1, "Internal error");
    }
    
    if (rreq)
    {
	mpi_errno = MPIDI_CH3_iRead (vc, rreq);
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPID_Abort(NULL, mpi_errno, -1, "Internal error");
	}
    }
    
    MPIDI_CH3I_inside_handler = 0;
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_START_PACKET_HANDLER);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_continue_packet_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void
MPIDI_CH3_continue_packet_handler (gasnet_token_t token, void* buf,
				   size_t data_sz)
{
    int mpi_errno;
    int gn_errno;
    gasnet_node_t sender;
    MPIDI_VC_t *vc;
    MPID_Request * rreq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_CONTINUE_PACKET_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_CONTINUE_PACKET_HANDLER);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    MPIDI_CH3I_inside_handler = 1;
    MPIDI_CH3I_gasnet_token = token;

    gn_errno = gasnet_AMGetMsgSource (token, &sender);
    if (gn_errno != GASNET_OK)
    {
	MPID_Abort(NULL, MPI_SUCCESS, -1, "GASNet AMGetMsgSource failed");
    }
    MPIDI_DBG_PRINTF((55, FCNAME, "  sender = %d\n", sender));
    MPIDI_PG_Get_vc(MPIDI_Process.my_pg, sender, &vc);
    vc->gasnet.data = buf;
    vc->gasnet.data_sz = data_sz;
    rreq = vc->gasnet.recv_active;
    
    MPIU_Assert (rreq);

    mpi_errno = MPIDI_CH3_iRead (vc, rreq);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPID_Abort(NULL, mpi_errno, -1, "Internal error");
    }
    
    MPIDI_CH3I_inside_handler = 0;
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_CONTINUE_PACKET_HANDLER);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_CTS_packet_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void
MPIDI_CH3_CTS_packet_handler (gasnet_token_t token, void* buf, size_t buf_sz,
			      MPI_Request sreq_id, MPI_Request rreq_id,
			      int remote_data_sz, int n_iov)
{
#if 0
    int mpi_errno;
    int gn_errno;
    gasnet_node_t sender;
    MPIDI_VC_t *vc;
    MPID_Request *sreq;
    MPID_IOV *iov = (MPID_IOV *)buf;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_CTS_PACKET_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_CTS_PACKET_HANDLER);

    MPIU_Assert (n_iov * sizeof (MPID_IOV) == buf_sz);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    MPIDI_CH3I_inside_handler = 1;
    MPIDI_CH3I_gasnet_token = token;

    MPID_Request_get_ptr(sreq_id, sreq);

    /* check if message needs to be truncated */
    if (remote_data_sz < sreq->dev.segment_size)
    {
	MPID_Segment_init(sreq->dev.user_buf, sreq->dev.user_count,
			  sreq->dev.datatype, &sreq->dev.segment, 0);
	sreq->dev.iov_count = MPID_IOV_LIMIT;
	sreq->dev.segment_first = 0;
	sreq->dev.segment_size = remote_data_sz;
	mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &sreq->dev.iov[0],
						     &sreq->dev.iov_count);
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPID_Abort(NULL, mpi_errno, -1, "Internal error");
	}
    }

    for (i = 0; i < n_iov; ++i)
    {
	sreq->gasnet.remote_iov[i] = iov[i];
    }
    sreq->gasnet.remote_iov_count = n_iov;
    sreq->gasnet.remote_req_id = rreq_id;
    sreq->gasnet.iov_bytes = 0;
    sreq->gasnet.remote_iov_bytes = 0;   
    sreq->gasnet.iov_offset = 0;
    sreq->gasnet.remote_iov_offset = 0;
    sreq->gasnet.rndv_state = MPIDI_CH3_RNDV_NEW;
    
    MPIDI_CH3I_SendQ_enqueue(sreq, CH3_RNDV_QUEUE);
    
    MPIDI_CH3I_inside_handler = 0;
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_CTS_PACKET_HANDLER);
#else
    abort ();
#endif
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_reload_IOV_or_done_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void
MPIDI_CH3_reload_IOV_or_done_handler (gasnet_token_t token, int rreq_id,
				      int sreq_id)
{

    MPID_Request *rreq;
    int gn_errno;
    gasnet_node_t sender;
    int complete;
    MPIDI_VC_t *vc;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_RELOAD_IOV_OR_DONE_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_RELOAD_IOV_OR_DONE_HANDLER);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    MPIDI_CH3I_inside_handler = 1;
    MPIDI_CH3I_gasnet_token = token;
  
    gn_errno = gasnet_AMGetMsgSource (token, &sender);
    if (gn_errno != GASNET_OK)
    {
	MPID_Abort (NULL, MPI_SUCCESS, -1, "GASNet AMGetMsgSource failed");
    }
    MPIDI_PG_Get_vc(MPIDI_Process.my_pg, sender, &vc);

    MPID_Request_get_ptr (rreq_id, rreq);

    MPIDI_CH3U_Handle_recv_req (vc, rreq, &complete);

    if (!complete)
    {
	gn_errno = gasnet_AMReplyMedium2 (token,
					  MPIDI_CH3_reload_IOV_reply_handler_id,
					  rreq->dev.iov,
					  rreq->dev.iov_count * sizeof (MPID_IOV),
					  sreq_id, rreq->dev.iov_count);
	if (gn_errno != GASNET_OK)
	{
	    MPID_Abort (NULL, MPI_SUCCESS, -1, "GASNet send failed");
	}
    }
    
    MPIDI_CH3I_inside_handler = 0;
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_RELOAD_IOV_OR_DONE_HANDLER);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_reload_IOV_reply_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void
MPIDI_CH3_reload_IOV_reply_handler (gasnet_token_t token, void *buf, int buf_sz,
				    int sreq_id, int n_iov)
{
    MPID_Request *sreq;
    MPID_IOV *iov = (MPID_IOV *)buf;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_RELOAD_IOV_REPLY_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_RELOAD_IOV_REPLY_HANDLER);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    MPIDI_CH3I_inside_handler = 1;
    MPIDI_CH3I_gasnet_token = token;

    MPIU_Assert (buf_sz = n_iov * sizeof (MPID_IOV));
    
    MPID_Request_get_ptr (sreq_id, sreq);

    for (i = 0; i < n_iov; ++i)
    {
	sreq->gasnet.remote_iov[i] = iov[i];
    }
    sreq->gasnet.remote_iov_count = n_iov;
    sreq->gasnet.remote_iov_offset = 0;

    sreq->gasnet.rndv_state = MPIDI_CH3_RNDV_NEW;
    MPIDI_CH3I_SendQ_enqueue (sreq, CH3_RNDV_QUEUE);
    
    MPIDI_CH3I_inside_handler = 0;
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_RELOAD_IOV_REPLY_HANDLER);
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_poke
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Progress_poke()
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    
    mpi_errno = MPIDI_CH3I_Progress(FALSE);
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    return mpi_errno;
}

#if !defined(MPIDI_CH3_Progress_end)
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_end
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Progress_end(MPID_Progress_state * state)
{
    /* MT: This function is empty for the single-threaded implementation */
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_END);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_END);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_END);
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_init()
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    for (i = 0; i < CH3_NUM_QUEUES; ++i)
    {
	MPIDI_CH3I_sendq_head[i] = NULL;
	MPIDI_CH3I_sendq_tail[i] = NULL;
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME send_enqueuedv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int
send_enqueuedv (MPIDI_VC_t * vc, MPID_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    int gn_errno;
    int msg_sz;
    int i, j;
    MPID_IOV tmp_iov;
    MPID_IOV * iov = sreq->dev.iov;
    int n_iov = sreq->dev.iov_count;
    MPIDI_STATE_DECL(MPID_STATE_SEND_ENQUEUEDV);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_ENQUEUEDV);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    MPIU_Assert(n_iov <= MPID_IOV_LIMIT);
    MPIU_Assert(iov[0].MPID_IOV_LEN <= sizeof(MPIDI_CH3_Pkt_t));
    /* The channel uses a fixed length header, the size of which is
     * the maximum of all possible packet headers */

    iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)iov[0].MPID_IOV_BUF);
    
    /* get an iov that has no more than MPIDI_CH3_packet_len of data */
    msg_sz = 0;
    for (i = 0; i < n_iov; ++i)
    {
	if (msg_sz + iov[i].MPID_IOV_LEN > MPIDI_CH3_packet_len)
	    break;
	msg_sz += iov[i].MPID_IOV_LEN;
    }
    if (i < n_iov)
    {
	tmp_iov = iov[i];
	iov[i].MPID_IOV_LEN = MPIDI_CH3_packet_len - msg_sz;
	
	MPIDI_DBG_PRINTF((55, FCNAME, "  sending %d bytes\n", msg_sz));
	gn_errno = gasnet_AMRequestMediumv0(vc->lpid,
					    MPIDI_CH3_start_packet_handler_id,
					    iov, i+1);
	if (gn_errno != GASNET_OK)
	{
	    MPID_Abort(NULL, MPI_SUCCESS, -1, "GASNet send failed");
	}

	sreq->dev.iov[i].MPID_IOV_BUF = (void *)((char *)tmp_iov.MPID_IOV_BUF +
						 iov[i].MPID_IOV_LEN);
	sreq->dev.iov[i].MPID_IOV_LEN = tmp_iov.MPID_IOV_LEN -
	    iov[i].MPID_IOV_LEN;
	    
	sreq->gasnet.iov_offset = i;
	sreq->dev.iov_count = n_iov - i;
    }
    else
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "  sending %d bytes\n", msg_sz));
	gn_errno = gasnet_AMRequestMediumv0(vc->lpid,
					    MPIDI_CH3_start_packet_handler_id,
					    iov, n_iov);
	if (gn_errno != GASNET_OK)
	{
	    MPID_Abort(NULL, MPI_SUCCESS, -1, "GASNet send failed");
	}
	sreq->gasnet.iov_offset = 0;
	sreq->dev.iov_count = 0;
    }

    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));

    MPIDI_FUNC_EXIT(MPID_STATE_SEND_ENQUEUEDV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME do_put
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int do_put (MPIDI_VC_t *vc, MPID_Request *sreq)
{
    int mpi_errno = MPI_SUCCESS;
    int gn_errno;
    int s_bytes, r_bytes;
    int s, r;
    int s_iov_len, r_iov_len;
    MPID_IOV *s_iov, *r_iov;
    int len;
    MPIDI_STATE_DECL(MPID_STATE_DO_PUT);

    MPIDI_FUNC_ENTER(MPID_STATE_DO_PUT);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));

    s_bytes = sreq->gasnet.iov_bytes;
    r_bytes = sreq->gasnet.remote_iov_bytes;
    
    s = sreq->gasnet.iov_offset;
    r = sreq->gasnet.remote_iov_offset;

    s_iov_len = sreq->dev.iov_count + s;
    r_iov_len = sreq->gasnet.remote_iov_count + r;

    s_iov = sreq->dev.iov;
    r_iov = sreq->gasnet.remote_iov;
    
    while (s < s_iov_len && r < r_iov_len)
    {
	if (s_iov[s].MPID_IOV_LEN - s_bytes <= r_iov[r].MPID_IOV_LEN - r_bytes)
	{
	    len = s_iov[s].MPID_IOV_LEN - s_bytes;
	}
	else
	{
	    len = r_iov[r].MPID_IOV_LEN - r_bytes;
	}
	
	gasnet_put_nbi_bulk (vc->lpid, (char *)r_iov[r].MPID_IOV_BUF + r_bytes,
			     (char *)s_iov[s].MPID_IOV_BUF + s_bytes, len);
	s_bytes += len;
	r_bytes += len;

	if (s_bytes >= s_iov[s].MPID_IOV_LEN)
	{
	    ++s;
	    s_bytes = 0;
	}

	if (r_bytes >= r_iov[r].MPID_IOV_LEN)
	{
	    ++r;
	    r_bytes = 0;
	}

    }
    
    sreq->gasnet.iov_bytes = s_bytes;
    sreq->gasnet.remote_iov_bytes = r_bytes;
    
    sreq->gasnet.iov_offset = s;
    sreq->gasnet.remote_iov_offset = r;
    
    sreq->dev.iov_count = s_iov_len - s;
    sreq->gasnet.remote_iov_count = r_iov_len - s;
    
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_DO_PUT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Connection_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Connection_terminate(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    mpi_errno = MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED);
    return mpi_errno;
}
/* end MPIDI_CH3_Connection_terminate() */


#ifndef MPIDI_CH3_GASNET_TAKES_IOV

/* I modified a version of gasnet and added this function there, but
   then was too lazy to make the same modifications on a newer version
   of gasnet.  I'm also too lazy to remove all of the references to
   gasnet_AMRequestMediumv0() from the channel code (and then
   re-verify the new code), so I'm just including this function
   here. --Darius
   FIXME: Remove references to gasnet_AMRequestMediumv0 --DARIUS
*/
int
gasnet_AMRequestMediumv0 (gasnet_node_t dest, gasnet_handler_t handler,
			  struct iovec *iov, size_t n_iov)
{
    int i;
    int len;
    char *buf;

    buf = (char *)MPIDI_CH3_packet_buffer;
    len = MPIDI_CH3_packet_len;
    
    for (i = 0; i < n_iov; ++i)
    {
	if (len < iov[i].MPID_IOV_LEN)
	    return GASNET_ERR_BAD_ARG;
	memcpy (buf, iov[i].MPID_IOV_BUF, iov[i].MPID_IOV_LEN);
	buf += iov[i].MPID_IOV_LEN;
	len -= iov[i].MPID_IOV_LEN;
    }
    
    return gasnet_AMRequestMedium0 (dest, handler, MPIDI_CH3_packet_buffer,
				    MPIDI_CH3_packet_len - len);
}
#endif
