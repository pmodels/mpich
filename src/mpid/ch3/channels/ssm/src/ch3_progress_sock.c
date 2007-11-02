/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ch3i_progress.h"
/* FIXME: This is nowhere set to true.  The name is non-conforming if it is
   not static */
static int shutting_down = FALSE;

#if 0
static int connection_post_send_pkt_and_pgid(MPIDI_CH3I_Connection_t * conn);
#endif
static inline int connection_post_recv_pkt(MPIDI_CH3I_Connection_t * conn);
static inline int connection_post_send_pkt(MPIDI_CH3I_Connection_t * conn);
static inline int connection_post_sendq_req(MPIDI_CH3I_Connection_t * conn);
static int adjust_iov(MPID_IOV ** iovp, int * countp, MPIU_Size_t nb);

extern MPIDI_CH3_PktHandler_Fcn *MPIDI_pktArray[MPIDI_CH3_PKT_END_CH3+1];

#undef FUNCNAME
#define FUNCNAME connection_post_sendq_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int connection_post_sendq_req(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vcch;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SENDQ_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SENDQ_REQ);
    /* post send of next request on the send queue */
    vcch = (MPIDI_CH3I_VC *)conn->vc->channel_private;

    conn->send_active = MPIDI_CH3I_SendQ_head(vcch); /* MT */
    if (conn->send_active != NULL)
    {
	mpi_errno = MPIDU_Sock_post_writev(
	      conn->sock, conn->send_active->dev.iov, 
	      conn->send_active->dev.iov_count, NULL);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**fail");
	}
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SENDQ_REQ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME connection_post_send_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int connection_post_send_pkt(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SEND_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SEND_PKT);
    
    mpi_errno = MPIDU_Sock_post_write(conn->sock, &conn->pkt, 
				      sizeof(conn->pkt), sizeof(conn->pkt), 
				      NULL);
    if (mpi_errno) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**fail");
    }

    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SEND_PKT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME connection_post_recv_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int connection_post_recv_pkt(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_RECV_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_RECV_PKT);

    mpi_errno = MPIDU_Sock_post_read(conn->sock, &conn->pkt, sizeof(conn->pkt),
				     sizeof(conn->pkt), NULL);
    if (mpi_errno) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**fail");
    }

    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_RECV_PKT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME adjust_iov
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int adjust_iov(MPID_IOV ** iovp, int * countp, MPIU_Size_t nb)
{
    MPID_IOV * const iov = *iovp;
    const int count = *countp;
    int offset = 0;

    while (offset < count)
    {
	if (iov[offset].MPID_IOV_LEN <= nb)
	{
	    nb -= iov[offset].MPID_IOV_LEN;
	    offset++;
	}
	else
	{
	    iov[offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *) iov[offset].MPID_IOV_BUF + nb);
	    iov[offset].MPID_IOV_LEN -= nb;
	    break;
	}
    }

    *iovp += offset;
    *countp -= offset;

    return (*countp == 0);
}

#if 0
#undef FUNCNAME
#define FUNCNAME connection_post_send_pkt_and_pgid
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int connection_post_send_pkt_and_pgid(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);
    
    conn->iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) &conn->pkt;
    conn->iov[0].MPID_IOV_LEN = (int) sizeof(conn->pkt);

    conn->iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) MPIDI_Process.my_pg->id;
    conn->iov[1].MPID_IOV_LEN = (int) strlen(MPIDI_Process.my_pg->id) + 1;

    mpi_errno = MPIDU_Sock_post_writev(conn->sock, conn->iov, 2, NULL);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**fail");
    }

    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);
    return mpi_errno;
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_handle_sock_event
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_handle_sock_event(MPIDU_Sock_event_t * event)
{
    int complete;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_HANDLE_SOCK_EVENT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_HANDLE_SOCK_EVENT);

    switch (event->op_type)
    {
	case MPIDU_SOCK_OP_READ:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event->user_ptr;
		
	    MPID_Request * rreq = conn->recv_active;

	    /* --BEGIN ERROR HANDLING-- */
	    if (event->error != MPI_SUCCESS)
	    {
		/* FIXME: the following should be handled by the close 
		   protocol */
		if (!shutting_down || MPIR_ERR_GET_CLASS(event->error) != MPIDU_SOCK_ERR_CONN_CLOSED)
		{
		    mpi_errno = event->error;
		    MPIU_ERR_POP(mpi_errno);
		}
		    
		break;
	    }
	    /* --END ERROR HANDLING-- */

	    if (conn->state == CONN_STATE_CONNECTED)
	    {
		if (conn->recv_active == NULL)
		{
                    MPIDI_msg_sz_t buflen = sizeof (MPIDI_CH3_Pkt_t);
		    MPIU_Assert(conn->pkt.type < MPIDI_CH3_PKT_END_CH3);
			
		    mpi_errno = 
			MPIDI_pktArray[conn->pkt.type]( conn->vc, &conn->pkt,
							&buflen, &rreq );
		    if (mpi_errno != MPI_SUCCESS) {
			MPIU_ERR_POP(mpi_errno);
		    }
                    MPIU_Assert(buflen == sizeof (MPIDI_CH3_Pkt_t));

		    if (rreq == NULL)
		    {
			if (conn->state != CONN_STATE_CLOSING)
			{
			    /* conn->recv_active = NULL;  -- already set to NULL */
			    mpi_errno = connection_post_recv_pkt(conn);
			    if (mpi_errno != MPI_SUCCESS) {
				MPIU_ERR_POP(mpi_errno);
			    }
			}
		    }
		    else
		    {
			for(;;)
			{
			    MPID_IOV * iovp;
			    MPIU_Size_t nb;
				
			    iovp = rreq->dev.iov;
			    
			    mpi_errno = MPIDU_Sock_readv(conn->sock, iovp, 
						   rreq->dev.iov_count, &nb);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
								 "**ch3|sock|immedread", "ch3|sock|immedread %p %p %p",
								 rreq, conn, conn->vc);
				goto fn_fail;
			    }
			    /* --END ERROR HANDLING-- */

			    MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,
    (MPIU_DBG_FDEST,"immediate readv, vc=%p nb=" MPIDI_MSG_SZ_FMT ", rreq=0x%08x",
     conn->vc, nb, rreq->handle));

			    if (nb > 0 && adjust_iov(&iovp, &rreq->dev.iov_count, nb))
			    {
				int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
				reqFn = rreq->dev.OnDataAvail;
				if (!reqFn) {
				    MPIDI_CH3U_Request_complete(rreq);
				    complete = TRUE;
				}
				else {
				    mpi_errno = reqFn( conn->vc, rreq, &complete );
				    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
				}

				if (complete)
				{
				    /* conn->recv_active = NULL; -- already set to NULL */
				    mpi_errno = connection_post_recv_pkt(conn);
				    if (mpi_errno != MPI_SUCCESS) {
					MPIU_ERR_POP(mpi_errno);
				    }

				    break;
				}
			    }
			    else
			    {
				MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,
        (MPIU_DBG_FDEST,"posting readv, vc=%p, rreq=0x%08x", 
	 conn->vc, rreq->handle));
				conn->recv_active = rreq;
				mpi_errno = MPIDU_Sock_post_readv(conn->sock, iovp, rreq->dev.iov_count, NULL);
				/* --BEGIN ERROR HANDLING-- */
				if (mpi_errno != MPI_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(
					mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postread",
					"ch3|sock|postread %p %p %p", rreq, conn, conn->vc);
				    goto fn_fail;
				}
				/* --END ERROR HANDLING-- */

				break;
			    }
			}
		    }
		}
		else /* incoming data */
		{
		    int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
		    reqFn = rreq->dev.OnDataAvail;
		    if (!reqFn) {
			MPIDI_CH3U_Request_complete(rreq);
			complete = TRUE;
		    }
		    else {
			mpi_errno = reqFn( conn->vc, rreq, &complete );
			if (mpi_errno) MPIU_ERR_POP(mpi_errno);
		    }

		    if (complete)
		    {
			conn->recv_active = NULL;
			mpi_errno = connection_post_recv_pkt(conn);
			if (mpi_errno != MPI_SUCCESS) {
			    MPIU_ERR_POP(mpi_errno);
			}
		    }
		    else /* more data to be read */
		    {
			for(;;)
			{
			    MPID_IOV * iovp;
			    MPIU_Size_t nb;
				
			    iovp = rreq->dev.iov;
			    
			    mpi_errno = MPIDU_Sock_readv(conn->sock, iovp, rreq->dev.iov_count, &nb);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
								 "**ch3|sock|immedread", "ch3|sock|immedread %p %p %p",
								 rreq, conn, conn->vc);
				goto fn_fail;
			    }
			    /* --END ERROR HANDLING-- */

			    MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,
        (MPIU_DBG_FDEST,"immediate readv, vc=%p nb=" MPIDI_MSG_SZ_FMT ", rreq=0x%08x",
	 conn->vc, nb, rreq->handle));
				
			    if (nb > 0 && adjust_iov(&iovp, &rreq->dev.iov_count, nb))
			    {
				int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
				reqFn = rreq->dev.OnDataAvail;
				if (!reqFn) {
				    MPIDI_CH3U_Request_complete(rreq);
				    complete = TRUE;
				}
				else {
				    mpi_errno = reqFn( conn->vc, rreq, &complete );
				    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
				}

				if (complete)
				{
				    conn->recv_active = NULL;
				    mpi_errno = connection_post_recv_pkt(conn);
				    if (mpi_errno != MPI_SUCCESS) {
					MPIU_ERR_POP(mpi_errno);
				    }

				    break;
				}
			    }
			    else
			    {
				MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,
     (MPIU_DBG_FDEST,"posting readv, vc=%p, rreq=0x%08x", 
      conn->vc, rreq->handle));
				/* conn->recv_active = rreq;  -- already set to current request */
				mpi_errno = MPIDU_Sock_post_readv(conn->sock, iovp, rreq->dev.iov_count, NULL);
				/* --BEGIN ERROR HANDLING-- */
				if (mpi_errno != MPI_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(
					mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postread",
					"ch3|sock|postread %p %p %p", rreq, conn, conn->vc);
				    goto fn_fail;
				}
				/* --END ERROR HANDLING-- */

				break;
			    }
			}
		    }
		}
	    }
	    else if (conn->state == CONN_STATE_OPEN_LRECV_DATA)
	    {
		mpi_errno = MPIDI_CH3_Sockconn_handle_connopen_event( conn );
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
						     "**ch3|sock|open_lrecv_data", NULL);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
	    }
	    else /* Handling some internal connection establishment or 
		    tear down packet */
	    { 
		mpi_errno = MPIDI_CH3_Sockconn_handle_conn_event( conn );
		if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
#if 0
		if (conn->pkt.type == MPIDI_CH3I_PKT_SC_OPEN_REQ)
		{
		    conn->state = CONN_STATE_OPEN_LRECV_DATA;
		    mpi_errno = MPIDU_Sock_post_read(conn->sock, conn->pg_id, conn->pkt.sc_open_req.pg_id_len, 
						     conn->pkt.sc_open_req.pg_id_len, NULL);   
		    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
		}
		else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_CONN_ACCEPT)
		{
		    MPIDI_VC_t *vc; 
		    MPIDI_CH3I_VC *vcch;
		    int port_name_tag = conn->pkt.sc_conn_accept.port_name_tag;

		    vc = (MPIDI_VC_t *) MPIU_Malloc(sizeof(MPIDI_VC_t));
		    /* --BEGIN ERROR HANDLING-- */
		    if (vc == NULL)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**nomem", NULL);
			goto fn_exit;
		    }
		    /* --END ERROR HANDLING-- */
		    /* FIXME - where does this vc get freed? */

		    vcch = (MPIDI_CH3I_VC *)vc->channel_private;
		    /* Initialize the device fields */
		    MPIDI_VC_Init(vc, NULL, 0);

		    MPIU_DBG_VCCHSTATECHANGE(vc,VC_STATE_CONNECTING);
		    vcch->state = MPIDI_CH3I_VC_STATE_CONNECTING;
		    vcch->sock = conn->sock;
		    vcch->conn = conn;
		    conn->vc = vc;


		    MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_OPEN_RESP);
		    conn->pkt.sc_open_resp.ack = TRUE;
                        
		    conn->state = CONN_STATE_OPEN_LSEND;
		    mpi_errno = connection_post_send_pkt(conn);
		    /* --BEGIN ERROR HANDLING-- */
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							 "**ch3|sock|scconnaccept", NULL);
			goto fn_exit;
		    }
		    /* --END ERROR HANDLING-- */

		    /* ENQUEUE vc */
		    MPIDI_CH3I_Acceptq_enqueue(vc, port_name_tag);

		}
		else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_OPEN_RESP)
		{
		    if (conn->pkt.sc_open_resp.ack)
		    {
			MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)conn->vc->channel_private;
			conn->state = CONN_STATE_CONNECTED;
			vcch->state = MPIDI_CH3I_VC_STATE_CONNECTED;
			MPIU_Assert(vcch->conn == conn);
			MPIU_Assert(vcch->sock == conn->sock);
			    
			mpi_errno = connection_post_recv_pkt(conn);
			if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
			mpi_errno = connection_post_sendq_req(conn);
			if (mpi_errno != MPI_SUCCESS) {
			    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_INTERN,
				    "**ch3|sock|scopenresp");
			}
		    }
		    else
		    {
			conn->vc = NULL;
			conn->state = CONN_STATE_CLOSING;
			mpi_errno = MPIDU_Sock_post_close(conn->sock);
			if (mpi_errno != MPI_SUCCESS) {
			    MPIU_ERR_POP(mpi_errno);
			}
		    }
		}
		/* --BEGIN ERROR HANDLING-- */
		else
		{
		    MPIDI_DBG_Print_packet(&conn->pkt);
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
						     "**ch3|sock|badpacket", "**ch3|sock|badpacket %d", conn->pkt.type);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
#endif
	    }
	    break;
	}
	    
	case MPIDU_SOCK_OP_WRITE:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event->user_ptr;

	    /* --BEGIN ERROR HANDLING-- */
	    if (event->error != MPI_SUCCESS) {
		mpi_errno = event->error;
		MPIU_ERR_POP(mpi_errno);
	    }
	    /* --END ERROR HANDLING-- */
		
	    if (conn->send_active)
	    {
		MPID_Request * sreq = conn->send_active;

		int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
		reqFn = sreq->dev.OnDataAvail;
		if (!reqFn) {
		    MPIU_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
		    MPIDI_CH3U_Request_complete(sreq);
		    complete = TRUE;
		}
		else {
		    mpi_errno = reqFn( conn->vc, sreq, &complete );
		    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
		}

		if (complete)
		{
		    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)conn->vc->channel_private;
		    MPIDI_CH3I_SendQ_dequeue(vcch);
		    mpi_errno = connection_post_sendq_req(conn);
		    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
		}
		else /* more data to send */
		{
		    for(;;)
		    {
			MPID_IOV * iovp;
			MPIU_Size_t nb;
				
			iovp = sreq->dev.iov;
			    
			mpi_errno = MPIDU_Sock_writev(conn->sock, iovp, sreq->dev.iov_count, &nb);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							     "**ch3|sock|immedwrite", "ch3|sock|immedwrite %p %p %p",
							     sreq, conn, conn->vc);
			    goto fn_exit;
			}
			/* --END ERROR HANDLING-- */

			MPIDI_DBG_PRINTF((55, FCNAME, "immediate writev, vc=0x%p, sreq=0x%08x, nb=%d",
					  conn->vc, sreq->handle, nb));

			if (nb > 0 && adjust_iov(&iovp, &sreq->dev.iov_count, nb))
			{
			    int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
			    reqFn = sreq->dev.OnDataAvail;
			    if (!reqFn) {
				MPIDI_CH3U_Request_complete(sreq);
				complete = TRUE;
			    }
			    else {
				mpi_errno = reqFn( conn->vc, sreq, &complete );
				if (mpi_errno) MPIU_ERR_POP(mpi_errno);
			    }

			    if (complete)
			    {
				MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)conn->vc->channel_private;
				MPIDI_CH3I_SendQ_dequeue(vcch);
				mpi_errno = connection_post_sendq_req(conn);
				if (mpi_errno) {
				    MPIU_ERR_POP(mpi_errno);
				}
				break;
			    }
			}
			else
			{
			    MPIDI_DBG_PRINTF((55, FCNAME, "posting writev, vc=0x%p, sreq=0x%08x", conn->vc, sreq->handle));
			    mpi_errno = MPIDU_Sock_post_writev(conn->sock, iovp, sreq->dev.iov_count, NULL);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(
				    mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postwrite",
				    "ch3|sock|postwrite %p %p %p", sreq, conn, conn->vc);
				goto fn_exit;
			    }
			    /* --END ERROR HANDLING-- */

			    break;
			}
		    }
		}
	    }
	    else /* finished writing internal packet header */
	    {
		/* the connection is not active yet */
		mpi_errno = MPIDI_CH3_Sockconn_handle_connwrite( conn );
		if (mpi_errno) { MPIU_ERR_POP( mpi_errno ); }
#if 0
		if (conn->state == CONN_STATE_OPEN_CSEND)
		{
		    /* finished sending open request packet */
		    /* post receive for open response packet */
		    conn->state = CONN_STATE_OPEN_CRECV;
		    mpi_errno = connection_post_recv_pkt(conn);
		    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
		}
		else if (conn->state == CONN_STATE_OPEN_LSEND)
		{
		    /* finished sending open response packet */
		    if (conn->pkt.sc_open_resp.ack == TRUE)
		    { 
			MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)conn->vc->channel_private;
			/* post receive for packet header */
			conn->state = CONN_STATE_CONNECTED;
			vcch->state = MPIDI_CH3I_VC_STATE_CONNECTED;
			mpi_errno = connection_post_recv_pkt(conn);
			if (mpi_errno != MPI_SUCCESS) { 
			    MPIU_ERR_POP(mpi_errno); 
			}

			mpi_errno = connection_post_sendq_req(conn);
			if (mpi_errno != MPI_SUCCESS) {
			    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_INTERN,
						"**ch3|sock|openlsend");
			}
		    }
		    else
		    {
			/* head-to-head connections - close this connection */
			conn->state = CONN_STATE_CLOSING;
			mpi_errno = MPIDU_Sock_post_close(conn->sock);
			if (mpi_errno != MPI_SUCCESS) {
			    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
						"**sock_post_close");
			}
		    }
		}
#endif
	    }

	    break;
	}

	case MPIDU_SOCK_OP_ACCEPT:
	{
	    mpi_errno = MPIDI_CH3_Sockconn_handle_accept_event();
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	    break;
	}

	case MPIDU_SOCK_OP_CONNECT:
	{
	    mpi_errno = MPIDI_CH3_Sockconn_handle_connect_event( 
		(MPIDI_CH3I_Connection_t *) event->user_ptr, event->error );
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	    break;
	}

	case MPIDU_SOCK_OP_CLOSE:
	{
	    mpi_errno = MPIDI_CH3_Sockconn_handle_close_event( 
			      (MPIDI_CH3I_Connection_t *) event->user_ptr );
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	    break;
	}

	case MPIDU_SOCK_OP_WAKEUP:
	{
	    MPIDI_CH3_Progress_signal_completion();
	    /* MPIDI_CH3I_progress_completion_count++; */
	    break;
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_HANDLE_SOCK_EVENT);
    return mpi_errno;

 fn_fail:
    goto fn_exit;
}
