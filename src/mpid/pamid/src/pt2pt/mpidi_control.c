/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/pt2pt/mpidi_control.c
 * \brief Interface to the control protocols used by MPID pt2pt
 */
#include <mpidimpl.h>


/**
 * \brief Send a high-priority msginfo struct (control data)
 *
 * \param[in] control  The pointer to the msginfo structure
 * \param[in] peerrank The node to whom the control message is to be sent
 */
static inline void
MPIDI_CtrlSend(pami_context_t  context,
               MPIDI_MsgInfo * msginfo,
               pami_task_t     peerrank)
{
  pami_endpoint_t dest;
  PAMI_Endpoint_create(MPIDI_Client, peerrank, 0, &dest);

  TRACE_ERR("CtrlSend:  type=%d  local=%u  remote=%u\n", msginfo->control, MPIR_Process.comm_world->rank, peerrank);
  pami_send_immediate_t params = {
    .dispatch = MPIDI_Protocols_Control,
    .dest     = dest,
    .header   = {
      .iov_base = msginfo,
      .iov_len  = sizeof(MPIDI_MsgInfo),
    },
    .data     = {
      .iov_base = NULL,
      .iov_len  = 0,
    },
  };

  pami_result_t rc;
  rc = PAMI_Send_immediate(context, &params);
  MPID_assert(rc == PAMI_SUCCESS);
}


/**
 * \brief Message layer callback which is invoked on the target node
 * of a flow-control rendezvous operation.
 *
 * This callback is invoked when the data buffer on the origin node
 * has been completely transfered to the target node. The target node
 * must acknowledge the completion of the transfer to the origin node
 * with a control message and then complete the receive by releasing
 * the request object.
 *
 * \param[in,out] rreq MPI receive request object
 */
void
MPIDI_RecvRzvDoneCB(pami_context_t  context,
                    void          * cookie,
                    pami_result_t   result)
{
  MPID_Request * rreq = (MPID_Request*)cookie;
  MPID_assert(rreq != NULL);

  TRACE_ERR("RZV Done for req=%p addr=%p *addr[0]=%#016llx *addr[1]=%#016llx\n",
            rreq,
            rreq->mpid.userbuf,
            *(((unsigned long long*)rreq->mpid.userbuf)+0),
            *(((unsigned long long*)rreq->mpid.userbuf)+1));

  /* Is it neccesary to save the original value of the 'type' field ?? */
  unsigned original_value = MPIDI_Request_getControl(rreq);
  MPIDI_Request_setControl(rreq, MPIDI_CONTROL_RENDEZVOUS_ACKNOWLEDGE);
  MPIDI_CtrlSend(context,
                 &rreq->mpid.envelope.msginfo,
                 MPIDI_Request_getPeerRank_pami(rreq));
  MPIDI_Request_setControl(rreq, original_value);

#ifdef USE_PAMI_RDMA
  pami_result_t rc;
  rc = PAMI_Memregion_destroy(context, &rreq->mpid.memregion);
  MPID_assert(rc == PAMI_SUCCESS);
#else
  if( (!MPIDI_Process.mp_s_use_pami_get) && (rreq->mpid.memregion_used) )
    {
      pami_result_t rc;
      rc = PAMI_Memregion_destroy(context, &rreq->mpid.memregion);
      MPID_assert(rc == PAMI_SUCCESS);
    }
#endif

  MPIDI_RecvDoneCB(context, rreq, PAMI_SUCCESS);
  MPID_Request_release(rreq);
}

/**
 * \brief Message layer callback which is invoked on the target node
 * of a 'zero-byte' flow-control rendezvous operation.
 *
 * \param[in] context Communication context
 * \param[in] cookie  Completion callback cookie - MPI receive request object
 * \param[in] result  Status
 */
void
MPIDI_RecvRzvDoneCB_zerobyte(pami_context_t  context,
                             void          * cookie,
                             pami_result_t   result)
{
  MPID_Request * rreq = (MPID_Request*)cookie;
  MPID_assert(rreq != NULL);

  /* Is it neccesary to save the original value of the 'type' field ?? */
  unsigned original_value = MPIDI_Request_getControl(rreq);
  MPIDI_Request_setControl(rreq, MPIDI_CONTROL_RENDEZVOUS_ACKNOWLEDGE);
  MPIDI_CtrlSend(context,
                 &rreq->mpid.envelope.msginfo,
                 MPIDI_Request_getPeerRank_pami(rreq));
  MPIDI_Request_setControl(rreq, original_value);

  MPIDI_RecvDoneCB(context, rreq, PAMI_SUCCESS);
  TRACE_SET_R_BIT(MPIDI_Request_getPeerRank_pami(rreq),(rreq->mpid.idx),fl.f.sync_com_in_HH);
  TRACE_SET_R_BIT(MPIDI_Request_getPeerRank_pami(rreq),(rreq->mpid.idx),fl.f.matchedInHH);
  TRACE_SET_R_VAL(MPIDI_Request_getPeerRank_pami(rreq),(rreq->mpid.idx),bufadd,rreq->mpid.userbuf);
  MPID_Request_release(rreq);
}

/**
 * \brief Acknowledge an MPI_Ssend()
 *
 * \param[in] req The request element to acknowledge
 *
 * \return The same as MPIDI_CtrlSend()
 */
void
MPIDI_SyncAck_post(pami_context_t   context,
                   MPID_Request   * req,
                   unsigned         peer)
{
  MPIDI_Request_setControl(req, MPIDI_CONTROL_SSEND_ACKNOWLEDGE);
  MPIDI_MsgInfo * info = &req->mpid.envelope.msginfo;
  MPIDI_CtrlSend(context, info, peer);
}


/**
 * \brief Acknowledge an MPI_Ssend()
 *
 * This is the handoff side, executing in advance().
 * The send is performed, and then the request is
 * completed.  The send is "immediate" so the payload
 * has been copied upon return from MPIDI_CtrlSend(),
 * so it is safe to complete the request.
 *
 * \param[in] context The PAMI context
 * \param[in] req The request element to acknowledge
 *
 * \returns  The PAMI return code
 */
pami_result_t
MPIDI_SyncAck_handoff(pami_context_t   context,
                      void           * inputReq)
{
  MPID_Request *req = inputReq;
  MPIDI_Request_setControl(req, MPIDI_CONTROL_SSEND_ACKNOWLEDGE);
  MPIDI_MsgInfo * info = &req->mpid.envelope.msginfo;
  unsigned peer        = MPIDI_Request_getPeerRank_pami(req);
  MPIDI_CtrlSend(context, info, peer);
  MPIDI_Request_complete(req);
  return PAMI_SUCCESS;
}


/**
 * \brief Process an incoming MPI_Ssend() acknowledgment
 *
 * \param[in] info The contents of the control message as a MPIDI_MsgInfo struct
 * \param[in] peer The rank of the node sending the data
 */
static inline void
MPIDI_SyncAck_proc(pami_context_t        context,
                   const MPIDI_MsgInfo * info,
                   unsigned              peer)
{
  MPID_assert(info != NULL);
  MPID_Request *req = MPIDI_Msginfo_getPeerRequest(info);
  MPID_assert(req != NULL);
  MPIDI_Request_complete(req);
}


/**
 * \brief
 *
 * \param[in] context
 * \param[in] req
 */
static inline void
MPIDI_RzvAck_proc_req(pami_context_t   context,
                  MPID_Request   * req)
{
#ifdef USE_PAMI_RDMA
  pami_result_t rc;
  rc = PAMI_Memregion_destroy(context, &req->mpid.envelope.memregion);
  MPID_assert(rc == PAMI_SUCCESS);
#else
  if( (!MPIDI_Process.mp_s_use_pami_get) && (req->mpid.envelope.memregion_used) )
    {
      pami_result_t rc;
      rc = PAMI_Memregion_destroy(context, &req->mpid.envelope.memregion);
      MPID_assert(rc == PAMI_SUCCESS);
    }
#endif
  TRACE_SET_S_BIT(req->mpid.partner_id,(req->mpid.idx),fl.f.recvAck);
  MPIDI_SendDoneCB(context, req, PAMI_SUCCESS);
}


/**
 * \brief Process an incoming rendezvous acknowledgment from the
 * target (remote) node and complete the MPI_Send() on the origin
 * (local) node.
 *
 * \param[in] context
 * \param[in] info The contents of the control message as a MPIDI_MsgInfo struct
 * \param[in] peer The rank of the node sending the data
 */
static inline void
MPIDI_RzvAck_proc(pami_context_t        context,
                  const MPIDI_MsgInfo * info,
                  pami_task_t           peer)
{
  MPID_assert(info != NULL);
  MPID_Request *req = MPIDI_Msginfo_getPeerRequest(info);
  MPID_assert(req != NULL);
  MPIDI_RzvAck_proc_req(context, req);
}


/**
 * \brief Process an incoming MPI_Send() cancelation
 *
 * \param[in] info The contents of the control message as a MPIDI_MsgInfo struct
 * \param[in] peer The rank of the node sending the data
 */
static inline void
MPIDI_CancelReq_proc(pami_context_t        context,
                     const MPIDI_MsgInfo * info,
                     pami_task_t           peer)
{
  MPIDI_CONTROL   type;
  MPIDI_MsgInfo   ackinfo;
  MPID_Request  * sreq;

  MPID_assert(info != NULL);

  sreq=MPIDI_Recvq_FDUR(MPIDI_Msginfo_getPeerRequestH(info),
                        info->MPIrank,
                        info->MPItag,
                        info->MPIctxt);
  if(sreq)
    {
      MPID_Request_release(sreq);
      type = MPIDI_CONTROL_CANCEL_ACKNOWLEDGE;
    }
  else
    {
      type = MPIDI_CONTROL_CANCEL_NOT_ACKNOWLEDGE;
    }

  TRACE_ERR("Cancel search: {rank=%d:tag=%d:ctxt=%d:req=%d}  my_request=%p  result=%s\n",
            info->MPIrank,
            info->MPItag,
            info->MPIctxt,
            MPIDI_Msginfo_getPeerRequestH(info),
            sreq,
            (type==MPIDI_CONTROL_CANCEL_ACKNOWLEDGE) ? "ACK" : "NAK");

  ackinfo.control = type;
  MPIDI_Msginfo_cpyPeerRequestH(&ackinfo, info);
  MPIDI_CtrlSend(context, &ackinfo, peer);
}


/**
 * \brief Process an incoming MPI_Send() cancelation result
 *
 * \param[in] info The contents of the control message as a MPIDI_MsgInfo struct
 * \param[in] peer The rank of the node sending the data
 */
static inline void
MPIDI_CancelAck_proc(pami_context_t        context,
                     const MPIDI_MsgInfo * info,
                     pami_task_t           peer)
{
  MPID_assert(info != NULL);
  MPID_Request *req = MPIDI_Msginfo_getPeerRequest(info);
  MPID_assert(req != NULL);

  TRACE_ERR("Cancel result: my_request=%p  result=%s\n",
            req,
            (info->control==MPIDI_CONTROL_CANCEL_ACKNOWLEDGE) ? "ACK" : "NAK");

  if(info->control == MPIDI_CONTROL_CANCEL_NOT_ACKNOWLEDGE)
    {
      req->mpid.cancel_pending = FALSE;
    }
  else
    {
      MPID_assert(info->control == MPIDI_CONTROL_CANCEL_ACKNOWLEDGE);
      MPID_assert(req->mpid.cancel_pending == TRUE);

      MPIR_STATUS_SET_CANCEL_BIT(req->status, TRUE);

      /*
       * Rendezvous-Sends wait until a rzv ack is received to complete
       * the send. Since this request was canceled, no rzv ack will be
       * sent from the target node; fake the response here.
       */
      if (MPIDI_Request_isRzv(req))
        {
          TRACE_ERR("RZV\n");
          MPIDI_RzvAck_proc_req(context, req);
        }
      /*
       * A canceled Sync-Send hasn't been ACKed (and now will never be
       * acked).  We call complete now to simulate an ACKed message.
       * This is the entirety of the sync-ack processing, so it hasn't
       * been made into a new function.
       */
      if (MPIDI_Request_isSync(req))
        {
          TRACE_ERR("Sync\n");
          MPIDI_Request_complete(req);
        }

      /*
       * Finally, this request has been faux-Sync-ACKed and
       * faux-RZV-ACKed.  We just do a normal completion.  The user
       * can call MPI_Wait()/MPI_Test() to finish it off (unless the
       * message hasn't finished sending, in which case the done
       * callback will finish it off).
       */
    }

  TRACE_ERR("Completing request\n");
  MPIDI_Request_complete(req);
}


/**
 * \brief This is the general PT2PT control message call-back
 */
void
MPIDI_ControlCB(pami_context_t    context,
                void            * cookie,
                const void      * _msginfo,
                size_t            size,
                const void      * sndbuf,
                size_t            sndlen,
                pami_endpoint_t   sender,
                pami_recv_t     * recv)
{
  MPID_assert(recv == NULL);
  MPID_assert(sndlen == 0);
  MPID_assert(_msginfo != NULL);
  MPID_assert(size == sizeof(MPIDI_MsgInfo));
  const MPIDI_MsgInfo *msginfo = (const MPIDI_MsgInfo *)_msginfo;
  pami_task_t senderrank = PAMIX_Endpoint_query(sender);

  TRACE_ERR("CtrlRecv:  type=%d  local=%u  remote=%u\n", msginfo->control, MPIR_Process.comm_world->rank, senderrank);
  switch (msginfo->control)
    {
    case MPIDI_CONTROL_SSEND_ACKNOWLEDGE:
      MPIDI_SyncAck_proc(context, msginfo, senderrank);
      break;
    case MPIDI_CONTROL_CANCEL_REQUEST:
      MPIDI_CancelReq_proc(context, msginfo, senderrank);
      break;
    case MPIDI_CONTROL_CANCEL_ACKNOWLEDGE:
    case MPIDI_CONTROL_CANCEL_NOT_ACKNOWLEDGE:
      MPIDI_CancelAck_proc(context, msginfo, senderrank);
      break;
    case MPIDI_CONTROL_RENDEZVOUS_ACKNOWLEDGE:
      MPIDI_RzvAck_proc(context, msginfo, senderrank);
      break;
#if TOKEN_FLOW_CONTROL
    case MPIDI_CONTROL_RETURN_TOKENS:
      MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
      MPIDI_Token_cntr[sender].tokens += msginfo->alltokens;
      MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
      break;
#endif
    default:
      fprintf(stderr, "Bad msginfo type: 0x%08x  %d\n",
              msginfo->control,
              msginfo->control);
      MPID_abort();
    }
  MPIDI_Progress_signal();
}
