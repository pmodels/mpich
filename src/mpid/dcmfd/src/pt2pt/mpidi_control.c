/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/pt2pt/mpidi_control.c
 * \brief Interface to the control protocols used by MPID pt2pt
 */
#include "mpidimpl.h"


/**
 * \brief Send a high-priority msginfo struct (control data)
 *
 * \param[in] control  The pointer to the msginfo structure
 * \param[in] peerrank The node to whom the control message is to be sent
 *
 * \return The same as DCMF_Control()
 */
static inline int MPIDI_DCMF_CtrlSend(MPIDI_DCMF_MsgInfo * control, size_t peerrank)
{
  int rc;
  MPID_assert_debug(sizeof(DCMF_Control_t) == sizeof(MPIDI_DCMF_MsgInfo));
  rc = DCMF_Control(&MPIDI_Protocols.control,
                    DCMF_MATCH_CONSISTENCY,
                    peerrank,
                    &control->quad);
  return rc;
}




/**
 * \brief Acknowledge an MPI_Ssend()
 *
 * \param[in] req The request element to acknowledge
 *
 * \return The same as MPIDI_DCMF_CtrlSend()
 */
int MPIDI_DCMF_postSyncAck(MPID_Request * req)
{
  unsigned peerrank         =  req->dcmf.peerrank;
  MPIDI_DCMF_MsgInfo * info = &req->dcmf.envelope.envelope.msginfo;
  info->msginfo.type = MPIDI_DCMF_REQUEST_TYPE_SSEND_ACKNOWLEDGE;
  return MPIDI_DCMF_CtrlSend(info, peerrank);
}

/**
 * \brief Process an incoming MPI_Ssend() acknowledgment
 *
 * \param[in] info The contents of the control message as a MPIDI_DCMF_MsgInfo struct
 * \param[in] peer The rank of the node sending the data
 */
static inline void MPIDI_DCMF_procSyncAck(MPIDI_DCMF_MsgInfo *info, unsigned peer)
{
  MPID_Request *infoRequest = (MPID_Request *)info->msginfo.req;
  MPID_assert(infoRequest != NULL);

  if(infoRequest->dcmf.state ==  MPIDI_DCMF_SEND_COMPLETE)
    MPID_Request_complete(infoRequest);
  else
    infoRequest->dcmf.state = MPIDI_DCMF_ACKNOWLEGED;
}


void MPIDI_DCMF_postCancelReq_free(void* p, DCMF_Error_t *err)
{
  MPID_assert_debug(p != NULL);
  MPIU_Free(p);
}


/**
 * \brief Cancel an MPI_Send()
 *
 * \param[in] req The request element to cancel
 *
 * \return The same as MPIDI_DCMF_CtrlSend()
 */
int MPIDI_DCMF_postCancelReq(MPID_Request * req)
{
  MPID_assert(req != NULL);
  struct cancel_t {
    MPIDI_DCMF_MsgInfo msginfo;
    DCMF_Request_t     msg;
  };

  struct cancel_t * cancel = MPIU_Malloc(sizeof(struct cancel_t));
  MPID_assert(cancel != NULL);
  DCMF_Callback_t cb_done = {
    function   : MPIDI_DCMF_postCancelReq_free,
    clientdata : cancel,
  };

  cancel->msginfo.msginfo.MPItag  = MPID_Request_getMatchTag(req);
  cancel->msginfo.msginfo.MPIrank = MPID_Request_getMatchRank(req);
  cancel->msginfo.msginfo.MPIctxt = MPID_Request_getMatchCtxt(req);
  cancel->msginfo.msginfo.type    = MPIDI_DCMF_REQUEST_TYPE_CANCEL_REQUEST;
  cancel->msginfo.msginfo.req     = req;

  return DCMF_Send(&MPIDI_Protocols.send,
                   &cancel->msg,
                   cb_done,
                   DCMF_MATCH_CONSISTENCY,
                   MPID_Request_getPeerRank(req),
                   0,
                   NULL,
                   cancel->msginfo.quad,
                   DCQuad_sizeof(MPIDI_DCMF_MsgInfo));
}

/**
 * \brief Process an incoming MPI_Send() cancelation
 *
 * \param[in] info The contents of the control message as a MPIDI_DCMF_MsgInfo struct
 * \param[in] peer The rank of the node sending the data
 */
void MPIDI_DCMF_procCancelReq(const MPIDI_DCMF_MsgInfo *info, size_t peer)
{
  MPIDI_DCMF_REQUEST_TYPE  type;
  MPIDI_DCMF_MsgInfo       ackinfo;
  MPID_Request            *sreq       = NULL;

  assert(info != NULL);
  assert(info->msginfo.req != NULL);

  sreq=MPIDI_Recvq_FDUR(info->msginfo.req,
                           info->msginfo.MPIrank,
                           info->msginfo.MPItag,
                           info->msginfo.MPIctxt);
  if(sreq)
    {
      if (sreq->dcmf.uebuf) {MPIU_Free(sreq->dcmf.uebuf); sreq->dcmf.uebuf = NULL;}
      MPID_Request_release(sreq);
      type = MPIDI_DCMF_REQUEST_TYPE_CANCEL_ACKNOWLEDGE;
    }
  else
    type = MPIDI_DCMF_REQUEST_TYPE_CANCEL_NOT_ACKNOWLEDGE;

  ackinfo.msginfo.type = type;
  ackinfo.msginfo.req  = info->msginfo.req;
  MPIDI_DCMF_CtrlSend(&ackinfo, peer);
}



/**
 * \brief Process an incoming MPI_Send() cancelation result
 *
 * \param[in] info The contents of the control message as a MPIDI_DCMF_MsgInfo struct
 * \param[in] peer The rank of the node sending the data
 */
static inline void MPIDI_DCMF_procCanelAck(MPIDI_DCMF_MsgInfo *info, size_t peer)
{
  MPID_Request *req = (MPID_Request *)info->msginfo.req;
  MPID_assert(req != NULL);

  if(info->msginfo.type == MPIDI_DCMF_REQUEST_TYPE_CANCEL_NOT_ACKNOWLEDGE)
    {
      req->dcmf.cancel_pending = FALSE;
      MPID_Request_complete(req);
      return;
    }

  MPID_assert(info->msginfo.type == MPIDI_DCMF_REQUEST_TYPE_CANCEL_ACKNOWLEDGE);
  MPID_assert(req->dcmf.cancel_pending == TRUE);

  req->status.cancelled = TRUE;

  /*
   * Rendezvous Sends wait until a rzv ack is received to complete the
   * send. Since this request was canceled, no rzv ack will be sent
   * from the target node, and the send done callback must be
   * explicitly called here.
   */
  if (MPID_Request_isRzv(req))
    MPIDI_DCMF_SendDoneCB(req, NULL);
  /*
   * This checks for a Sync-Send that hasn't been ACKed (and now will
   * never be acked), but has transfered the data.  When
   * MPIDI_DCMF_SendDoneCB() was called for this one, it wouldn't have
   * called MPID_Request_complete() to decrement the CC.  Therefore,
   * we call it now to simulate an ACKed message.
   */
  if ( (MPID_Request_getType(req) == MPIDI_DCMF_REQUEST_TYPE_SSEND) &&
       (req->dcmf.state           == MPIDI_DCMF_SEND_COMPLETE) )
    MPID_Request_complete(req);

  /*
   * Finally, this request has been faux-Sync-ACKed and
   * faux-RZV-ACKed.  We just set the state and do a normal
   * completion.  This will set the completion to 0, so that the user
   * can call MPI_Wait()/MPI_Test() to finish it off (unless the
   * message is still in the MPIDI_DCMF_INITIALIZED state, in which
   * case the done callback will finish it off).
   */
  req->dcmf.state=MPIDI_DCMF_REQUEST_DONE_CANCELLED;
  MPID_Request_complete(req);
}


/**
 * \brief Process an incoming rendezvous acknowledgment from the
 * target (remote) node and complete the MPI_Send() on the origin
 * (local) node.
 *
 * \param[in] info The contents of the control message as a MPIDI_DCMF_MsgInfo struct
 * \param[in] peer The rank of the node sending the data
 */
static inline void MPIDI_DCMF_procRzvAck(MPIDI_DCMF_MsgInfo *info, size_t peer)
{
  MPID_assert(info->msginfo.req != NULL);
  MPID_Request  * sreq = ((MPID_Request *)info->msginfo.req);
  DCMF_Memregion_destroy(&sreq->dcmf.envelope.envelope.memregion);
  MPIDI_DCMF_SendDoneCB((MPID_Request *)info->msginfo.req, NULL);
}


/**
 * \brief This is the general PT2PT control message call-back
 *
 * \param[in] clientdata Opaque client data
 * \param[in] p    The contents of the control message as a DCMF_Control_t
 * \param[in] peer The rank of the node sending the data
 */
void MPIDI_BG2S_ControlCB(void *clientdata, const DCMF_Control_t * p, size_t peer)
{
  MPID_assert_debug(sizeof(DCMF_Control_t) == sizeof(MPIDI_DCMF_MsgInfo));
  MPIDI_DCMF_MsgInfo * info = (MPIDI_DCMF_MsgInfo *) p;

  switch (info->msginfo.type)
    {
    case MPIDI_DCMF_REQUEST_TYPE_SSEND_ACKNOWLEDGE:
      MPIDI_DCMF_procSyncAck(info, peer);
      break;
    case MPIDI_DCMF_REQUEST_TYPE_CANCEL_ACKNOWLEDGE:
    case MPIDI_DCMF_REQUEST_TYPE_CANCEL_NOT_ACKNOWLEDGE:
      MPIDI_DCMF_procCanelAck(info, peer);
      break;
    case MPIDI_DCMF_REQUEST_TYPE_RENDEZVOUS_ACKNOWLEDGE:
      MPIDI_DCMF_procRzvAck(info, peer);
      break;
    default:
      printf("Bad msginfo type: 0x%08x  %u  %d\n", info->msginfo.type, info->msginfo.type, info->msginfo.type);
      printf("Quad:\n");
      printf("    w0: 0x%08x  %u\n", info->quad[0].w0, info->quad[0].w0);
      printf("    w1: 0x%08x  %u\n", info->quad[0].w1, info->quad[0].w1);
      printf("    w2: 0x%08x  %u\n", info->quad[0].w2, info->quad[0].w2);
      printf("--> w3: 0x%08x  %u\n", info->quad[0].w3, info->quad[0].w3);
      MPID_abort();
    }
  MPID_Progress_signal();
}
