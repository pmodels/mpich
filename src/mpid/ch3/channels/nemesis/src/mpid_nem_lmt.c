/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : FT
      description : cvars that control behavior of fault tolerance

cvars:
   - name       : MPIR_CVAR_ENABLE_FT
     category   : FT
     type       : boolean
     default    : false
     class      : device
     verbosity  : MPI_T_VERBOSITY_USER_BASIC
     scope      : MPI_T_SCOPE_ALL_EQ
     description : >-
       Enable fault tolerance functions

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#define set_request_info(rreq_, pkt_, msg_type_)		\
{								\
    (rreq_)->status.MPI_SOURCE = (pkt_)->match.parts.rank;	\
    (rreq_)->status.MPI_TAG = (pkt_)->match.parts.tag;		\
    MPIR_STATUS_SET_COUNT((rreq_)->status, (pkt_)->data_sz);		\
    (rreq_)->dev.sender_req_id = (pkt_)->sender_req_id;		\
    (rreq_)->dev.recv_data_sz = (pkt_)->data_sz;		\
    MPIDI_Request_set_seqnum((rreq_), (pkt_)->seqnum);		\
    MPIDI_Request_set_msg_type((rreq_), (msg_type_));		\
}

/* request completion actions */
static int do_cts(MPIDI_VC_t *vc, MPIR_Request *rreq, int *complete);
static int do_send(MPIDI_VC_t *vc, MPIR_Request *rreq, int *complete);
static int do_cookie(MPIDI_VC_t *vc, MPIR_Request *rreq, int *complete);

/* packet handlers */
static int pkt_RTS_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data, intptr_t *buflen, MPIR_Request **rreqp);
static int pkt_CTS_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data, intptr_t *buflen, MPIR_Request **rreqp);
static int pkt_DONE_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data, intptr_t *buflen, MPIR_Request **rreqp);
static int pkt_COOKIE_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data, intptr_t *buflen, MPIR_Request **rreqp);

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_pkthandler_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_lmt_pkthandler_init(MPIDI_CH3_PktHandler_Fcn *pktArray[], int arraySize)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_LMT_PKTHANDLER_INIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_LMT_PKTHANDLER_INIT);

    /* Check that the array is large enough */
    if (arraySize <= MPIDI_CH3_PKT_END_ALL) {
	MPIR_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_INTERN, "**ch3|pktarraytoosmall");
    }

    pktArray[MPIDI_NEM_PKT_LMT_RTS] = pkt_RTS_handler;
    pktArray[MPIDI_NEM_PKT_LMT_CTS] = pkt_CTS_handler;
    pktArray[MPIDI_NEM_PKT_LMT_DONE] = pkt_DONE_handler;
    pktArray[MPIDI_NEM_PKT_LMT_COOKIE] = pkt_COOKIE_handler;

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_LMT_PKTHANDLER_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* MPID_nem_lmt_RndvSend - Send a request to perform a rendezvous send */
#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_RndvSend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_lmt_RndvSend(MPIR_Request **sreq_p, const void * buf, MPI_Aint count,
                          MPI_Datatype datatype, int dt_contig ATTRIBUTE((unused)),
                          intptr_t data_sz, MPI_Aint dt_true_lb ATTRIBUTE((unused)),
                          int rank, int tag, MPIR_Comm * comm, int context_offset)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_PKT_DECL_CAST(upkt, MPID_nem_pkt_lmt_rts_t, rts_pkt);
    MPIDI_VC_t *vc;
    MPIR_Request *sreq =*sreq_p;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_LMT_RNDVSEND);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_LMT_RNDVSEND);

    MPIDI_Comm_get_vc_set_active(comm, rank, &vc);

    /* if the lmt functions are not set, fall back to the default rendezvous code */
    if (vc->ch.lmt_initiate_lmt == NULL)
    {
        mpi_errno = MPIDI_CH3_RndvSend(sreq_p, buf, count, datatype, dt_contig, data_sz, dt_true_lb, rank, tag, comm, context_offset);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,VERBOSE,
		   "sending lmt RTS, data_sz=%" PRIdPTR, data_sz);
    sreq->dev.partner_request = NULL;
    sreq->ch.lmt_tmp_cookie.MPL_IOV_LEN = 0;
	
    MPIDI_Pkt_init(rts_pkt, MPIDI_NEM_PKT_LMT_RTS);
    rts_pkt->match.parts.rank	      = comm->rank;
    rts_pkt->match.parts.tag	      = tag;
    rts_pkt->match.parts.context_id = comm->context_id + context_offset;
    rts_pkt->sender_req_id    = sreq->handle;
    rts_pkt->data_sz	      = data_sz;

    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(rts_pkt, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);
    sreq->ch.vc = vc;

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    mpi_errno = vc->ch.lmt_initiate_lmt(vc, &upkt.p, sreq);
    if (MPIR_CVAR_ENABLE_FT) {
        if (MPI_SUCCESS == mpi_errno)
            MPID_nem_lmt_rtsq_enqueue(&vc->ch.lmt_rts_queue, sreq);
    }
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_LMT_RNDVSEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/*
 * This routine processes a rendezvous message once the message is matched.
 * It is used in mpid_recv and mpid_irecv.
 */
#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_RndvRecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_lmt_RndvRecv(MPIDI_VC_t *vc, MPIR_Request *rreq)
{
    int mpi_errno = MPI_SUCCESS;
    int complete = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_LMT_RNDVRECV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_LMT_RNDVRECV);

    /* if the lmt functions are not set, fall back to the default rendezvous code */
    if (vc->ch.lmt_initiate_lmt == NULL)
    {
        mpi_errno = MPIDI_CH3_RecvRndv(vc, rreq);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE, "lmt RTS in the request");

    mpi_errno = do_cts(vc, rreq, &complete);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    MPIR_Assert(complete);

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_LMT_RNDVRECV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME pkt_RTS_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int pkt_RTS_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data, intptr_t *buflen, MPIR_Request **rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request * rreq;
    int found;
    MPID_nem_pkt_lmt_rts_t * const rts_pkt = (MPID_nem_pkt_lmt_rts_t *)pkt;
    intptr_t data_len;
    MPIR_CHKPMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PKT_RTS_HANDLER);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PKT_RTS_HANDLER);

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);

    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST, "received LMT RTS pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d, data_sz=%" PRIdPTR,
                                        rts_pkt->sender_req_id, rts_pkt->match.parts.rank, rts_pkt->match.parts.tag, rts_pkt->match.parts.context_id,
                                        rts_pkt->data_sz));

    rreq = MPIDI_CH3U_Recvq_FDP_or_AEU(&rts_pkt->match, &found);
    MPIR_ERR_CHKANDJUMP1(!rreq, mpi_errno,MPI_ERR_OTHER, "**nomemreq", "**nomemuereq %d", MPIDI_CH3U_Recvq_count_unexp());

    /* If the completion counter is 0, that means that the communicator to
     * which this message is being sent has been revoked and we shouldn't
     * bother finishing this. */
    if (!found && MPIR_cc_get(rreq->cc) == 0) {
        *rreqp = NULL;
        goto fn_exit;
    }

    set_request_info(rreq, rts_pkt, MPIDI_REQUEST_RNDV_MSG);

    rreq->ch.lmt_req_id = rts_pkt->sender_req_id;
    rreq->ch.lmt_data_sz = rts_pkt->data_sz;

    data_len = *buflen;



    if (data_len < rts_pkt->cookie_len)
    {
        /* set for the cookie to be received into the tmp_cookie in the request */
        MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"haven't received entire cookie");
        MPIR_CHKPMEM_MALLOC(rreq->ch.lmt_tmp_cookie.MPL_IOV_BUF, char *, rts_pkt->cookie_len, mpi_errno, "tmp cookie buf");
        rreq->ch.lmt_tmp_cookie.MPL_IOV_LEN = rts_pkt->cookie_len;

        rreq->dev.iov[0] = rreq->ch.lmt_tmp_cookie;
        rreq->dev.iov_count = 1;
        *rreqp = rreq;
        *buflen = 0;

        if (found)
        {
            /* set do_cts() to be called once we've received the entire cookie */
            MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"posted request found");
            rreq->dev.OnDataAvail = do_cts;
        }
        else
        {
            /* receive the rest of the cookie and wait for a match */
            MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"unexpected request allocated");
            rreq->dev.OnDataAvail = 0;
            MPIDI_CH3_Progress_signal_completion();
        }
    }
    else
    {
        if (rts_pkt->cookie_len == 0)
        {
            /* there's no cookie to receive */
            rreq->ch.lmt_tmp_cookie.MPL_IOV_LEN = 0;
            rreq->dev.iov_count = 0;
            *buflen = 0;
            *rreqp = NULL;
        }
        else
        {
            /* receive cookie into tmp_cookie in the request */
            MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"received entire cookie");
            MPIR_CHKPMEM_MALLOC(rreq->ch.lmt_tmp_cookie.MPL_IOV_BUF, char *, rts_pkt->cookie_len, mpi_errno, "tmp cookie buf");
            rreq->ch.lmt_tmp_cookie.MPL_IOV_LEN = rts_pkt->cookie_len;
        
            MPIR_Memcpy(rreq->ch.lmt_tmp_cookie.MPL_IOV_BUF, data, rts_pkt->cookie_len);
            *buflen = rts_pkt->cookie_len;
            *rreqp = NULL;
        }
        
        if (found)
        {
            /* have a matching request and the entire cookie (if any), call do_cts() */
            int complete;
            MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"posted request found");
            mpi_errno = do_cts(vc, rreq, &complete);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_Assert(complete);
        }
        else
        {
            MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"unexpected request allocated");
            rreq->dev.OnDataAvail = 0;
            MPIDI_CH3_Progress_signal_completion();
        }

    }

    
    MPIR_CHKPMEM_COMMIT();
 fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PKT_RTS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME pkt_CTS_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int pkt_CTS_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data, intptr_t *buflen, MPIR_Request **rreqp)
{
    MPID_nem_pkt_lmt_cts_t * const cts_pkt = (MPID_nem_pkt_lmt_cts_t *)pkt;
    MPIR_Request *sreq;
    MPIR_Request *rts_sreq;
    intptr_t data_len;
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKPMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PKT_CTS_HANDLER);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PKT_CTS_HANDLER);

    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"received rndv CTS pkt");

    data_len = *buflen;

    MPIR_Request_get_ptr(cts_pkt->sender_req_id, sreq);

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    if (MPIR_CVAR_ENABLE_FT) {
        /* Remove the request from the VC RTS queue. */
        MPID_nem_lmt_rtsq_search_remove(&vc->ch.lmt_rts_queue, cts_pkt->sender_req_id, &rts_sreq);
    }
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    sreq->ch.lmt_req_id = cts_pkt->receiver_req_id;
    sreq->ch.lmt_data_sz = cts_pkt->data_sz;

    /* Release the RTS request if one exists.
       MPIR_Request_fetch_and_clear_rts_sreq() needs to be atomic to
       prevent cancel send from cancelling the wrong (future) request.
       If MPIR_Request_fetch_and_clear_rts_sreq() returns a NULL
       rts_sreq, then MPID_Cancel_send() is responsible for releasing
       the RTS request object. */
    MPIDI_Request_fetch_and_clear_rts_sreq(sreq, &rts_sreq);
    if (rts_sreq != NULL)
        MPIR_Request_free(rts_sreq);

    if (cts_pkt->cookie_len != 0)
    {
        if (data_len >= cts_pkt->cookie_len)
        {
            /* if whole cookie has been received, start the send */
            sreq->ch.lmt_tmp_cookie.MPL_IOV_BUF = data;
            sreq->ch.lmt_tmp_cookie.MPL_IOV_LEN = cts_pkt->cookie_len;
            MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
            mpi_errno = vc->ch.lmt_start_send(vc, sreq, sreq->ch.lmt_tmp_cookie);
            MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
            if (mpi_errno) MPIR_ERR_POP (mpi_errno);
            sreq->ch.lmt_tmp_cookie.MPL_IOV_LEN = 0;
            *buflen = cts_pkt->cookie_len;
            *rreqp = NULL;
        }
        else
        {
            /* create a recv req and set up to receive the cookie into the sreq's tmp_cookie */
            MPIR_Request *rreq;

            MPIR_CHKPMEM_MALLOC(sreq->ch.lmt_tmp_cookie.MPL_IOV_BUF, char *, cts_pkt->cookie_len, mpi_errno, "tmp cookie buf");
            sreq->ch.lmt_tmp_cookie.MPL_IOV_LEN = cts_pkt->cookie_len;

            MPIDI_Request_create_rreq(rreq, mpi_errno, goto fn_fail);
            /* FIXME:  where does this request get freed? */

            rreq->dev.iov[0] = sreq->ch.lmt_tmp_cookie;
            rreq->dev.iov_count = 1;
            rreq->ch.lmt_req = sreq;
            rreq->dev.OnDataAvail = do_send;
            *buflen = 0;
            *rreqp = rreq;
        }
    }
    else
    {
        MPL_IOV cookie = {0,0};
        MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
        mpi_errno = vc->ch.lmt_start_send(vc, sreq, cookie);
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
        if (mpi_errno) MPIR_ERR_POP (mpi_errno);
        *buflen = 0;
        *rreqp = NULL;
    }

 fn_exit:
    MPIR_CHKPMEM_COMMIT();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PKT_CTS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME pkt_DONE_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int pkt_DONE_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data ATTRIBUTE((unused)),
                            intptr_t *buflen, MPIR_Request **rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_pkt_lmt_done_t * const done_pkt = (MPID_nem_pkt_lmt_done_t *)pkt;
    MPIR_Request *req;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PKT_DONE_HANDLER);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PKT_DONE_HANDLER);

    *buflen = 0;
    MPIR_Request_get_ptr(done_pkt->req_id, req);

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    switch (MPIDI_Request_get_type(req))
    {
    case MPIDI_REQUEST_TYPE_RECV:
        mpi_errno = vc->ch.lmt_done_recv(vc, req);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        break;
    case MPIDI_REQUEST_TYPE_SEND:
    case MPIDI_REQUEST_TYPE_RSEND:
    case MPIDI_REQUEST_TYPE_SSEND:
    case MPIDI_REQUEST_TYPE_BSEND:
        mpi_errno = vc->ch.lmt_done_send(vc, req);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        break;
    default:
        MPIR_ERR_INTERNALANDJUMP(mpi_errno, "unexpected request type");
        break;
    }

    *rreqp = NULL;

 fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PKT_DONE_HANDLER);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME pkt_COOKIE_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int pkt_COOKIE_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data, intptr_t *buflen, MPIR_Request **rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_pkt_lmt_cookie_t * const cookie_pkt = (MPID_nem_pkt_lmt_cookie_t *)pkt;
    MPIR_Request *req;
    intptr_t data_len;
    MPIR_CHKPMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PKT_COOKIE_HANDLER);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PKT_COOKIE_HANDLER);

    data_len = *buflen;

    if (cookie_pkt->from_sender) {
        MPIR_Request_get_ptr(cookie_pkt->receiver_req_id, req);
        MPIR_Assert(req != NULL);
        req->ch.lmt_req_id = cookie_pkt->sender_req_id;
    }
    else {
        MPIR_Request_get_ptr(cookie_pkt->sender_req_id, req);
        MPIR_Assert(req != NULL);
        req->ch.lmt_req_id = cookie_pkt->receiver_req_id;
    }

    if (cookie_pkt->cookie_len != 0)
    {
        if (data_len >= cookie_pkt->cookie_len)
        {
            /* call handle cookie with cookie data in receive buffer */
            MPL_IOV cookie;

            cookie.MPL_IOV_BUF = data;
            cookie.MPL_IOV_LEN = cookie_pkt->cookie_len;
            MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
            mpi_errno = vc->ch.lmt_handle_cookie(vc, req, cookie);
            MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            *rreqp = NULL;
            *buflen = cookie_pkt->cookie_len;
        }
        else
        {
            /* create a recv req and set up to receive the cookie into the rreq's tmp_cookie */
            MPIR_Request *rreq;

            MPIDI_Request_create_rreq(rreq, mpi_errno, goto fn_fail);
            MPIR_CHKPMEM_MALLOC(rreq->ch.lmt_tmp_cookie.MPL_IOV_BUF, char *, cookie_pkt->cookie_len, mpi_errno, "tmp cookie buf");
            /* FIXME:  where does this request get freed? */
            rreq->ch.lmt_tmp_cookie.MPL_IOV_LEN = cookie_pkt->cookie_len;

            rreq->dev.iov[0] = rreq->ch.lmt_tmp_cookie;
            rreq->dev.iov_count = 1;
            rreq->ch.lmt_req = req;
            rreq->dev.OnDataAvail = do_cookie;
            *rreqp = rreq;
            *buflen = 0;
        }
    }
    else
    {
        MPL_IOV cookie = {0,0};

        MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
        mpi_errno = vc->ch.lmt_handle_cookie(vc, req, cookie);
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        *buflen = 0;
        *rreqp = NULL;
    }

 fn_exit:
    MPIR_CHKPMEM_COMMIT();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PKT_COOKIE_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME do_cts
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int do_cts(MPIDI_VC_t *vc, MPIR_Request *rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    intptr_t data_sz;
    int dt_contig ATTRIBUTE((unused));
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPIDU_Datatype* dt_ptr;
    MPL_IOV s_cookie;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_DO_CTS);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_DO_CTS);

    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"posted request found");

    /* determine amount of data to be transfered and check for truncation */
    MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    if (rreq->ch.lmt_data_sz > data_sz)
    {
        MPIR_ERR_SET2(rreq->status.MPI_ERROR, MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d", rreq->ch.lmt_data_sz, data_sz);
        rreq->ch.lmt_data_sz = data_sz;
    }

    s_cookie = rreq->ch.lmt_tmp_cookie;

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    mpi_errno = vc->ch.lmt_start_recv(vc, rreq, s_cookie);
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* free cookie buffer allocated in RTS handler */
    if (rreq->ch.lmt_tmp_cookie.MPL_IOV_LEN)
    {
        MPL_free(rreq->ch.lmt_tmp_cookie.MPL_IOV_BUF);
        rreq->ch.lmt_tmp_cookie.MPL_IOV_LEN = 0;
    }

    *complete = TRUE;

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DO_CTS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME do_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int do_send(MPIDI_VC_t *vc, MPIR_Request *rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPL_IOV r_cookie;
    MPIR_Request * const sreq = rreq->ch.lmt_req;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_DO_SEND);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_DO_SEND);

    r_cookie = sreq->ch.lmt_tmp_cookie;

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    mpi_errno = vc->ch.lmt_start_send(vc, sreq, r_cookie);
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* free cookie buffer allocated in CTS handler */
    MPL_free(sreq->ch.lmt_tmp_cookie.MPL_IOV_BUF);
    sreq->ch.lmt_tmp_cookie.MPL_IOV_LEN = 0;

    *complete = TRUE;

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DO_SEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME do_cookie
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int do_cookie(MPIDI_VC_t *vc, MPIR_Request *rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPL_IOV cookie;
    MPIR_Request *req = rreq->ch.lmt_req;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_DO_COOKIE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_DO_COOKIE);

    cookie = req->ch.lmt_tmp_cookie;

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    mpi_errno = vc->ch.lmt_handle_cookie(vc, req, cookie);
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

    /* free cookie buffer allocated in COOKIE handler */
    MPL_free(req->ch.lmt_tmp_cookie.MPL_IOV_BUF);
    req->ch.lmt_tmp_cookie.MPL_IOV_LEN = 0;

    *complete = TRUE;

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DO_COOKIE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
