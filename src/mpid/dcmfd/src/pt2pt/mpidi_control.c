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
static inline int MPIDI_DCMF_CtrlSend(MPIDI_DCMF_MsgInfo * control, unsigned peerrank)
{
  int rc;
  MPID_assert_debug(sizeof(DCMF_Control_t) == sizeof(MPIDI_DCMF_MsgInfo));
  rc = DCMF_Control(&MPIDI_Protocols.control,
                    DCMF_MATCH_CONSISTENCY,
                    peerrank,
                    control->quad);
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
  MPIDI_DCMF_MsgInfo * info = &req->dcmf.msginfo;
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

  MPIDI_DCMF_MsgInfo control;
  control.msginfo.MPItag  = req->dcmf.msginfo.msginfo.MPItag;
  control.msginfo.MPIrank = req->dcmf.msginfo.msginfo.MPIrank;
  control.msginfo.MPIctxt = req->dcmf.msginfo.msginfo.MPIctxt;
  control.msginfo.type    = MPIDI_DCMF_REQUEST_TYPE_CANCEL_REQUEST;
  control.msginfo.req     = req;

  return MPIDI_DCMF_CtrlSend(&control, MPID_Request_getPeerRank(req));
}

/**
 * \brief Process an incoming MPI_Send() cancelation
 *
 * \param[in] info The contents of the control message as a MPIDI_DCMF_MsgInfo struct
 * \param[in] peer The rank of the node sending the data
 */
static inline void MPIDI_DCMF_procCancelReq(MPIDI_DCMF_MsgInfo *info, unsigned peer)
{
  MPIDI_DCMF_REQUEST_TYPE  type;
  MPIDI_DCMF_MsgInfo       ackinfo;
  MPID_Request            *sreq       = NULL;
  MPID_Comm               *comm_world = NULL;

  assert(info != NULL);
  assert(info->msginfo.req != NULL);

  MPID_Comm_get_ptr(MPI_COMM_WORLD, comm_world );
  sreq=MPIDI_Recvq_FDURSTC(info->msginfo.req,
                           info->msginfo.MPIrank,
                           info->msginfo.MPItag,
                           info->msginfo.MPIctxt);
  if(sreq)
    {
      sreq->status.cancelled = TRUE;
      sreq->dcmf.ca = MPIDI_DCMF_CA_DISCARD_UEBUF_AND_COMPLETE;
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
static inline void MPIDI_DCMF_procCanelAck(MPIDI_DCMF_MsgInfo *info, unsigned peer)
{
  MPID_Request *infoRequest = (MPID_Request *)info->msginfo.req;
  MPID_assert(infoRequest != NULL);

  if(info->msginfo.type == MPIDI_DCMF_REQUEST_TYPE_CANCEL_ACKNOWLEDGE)
    infoRequest->status.cancelled = TRUE;
  else if(info->msginfo.type == MPIDI_DCMF_REQUEST_TYPE_CANCEL_NOT_ACKNOWLEDGE)
    ;
  else
    MPID_abort();

  MPID_assert(infoRequest->dcmf.cancel_pending == TRUE);
  if(
     (infoRequest->dcmf.state==MPIDI_DCMF_REQUEST_DONE_CANCELLED) ||
     (infoRequest->dcmf.state==MPIDI_DCMF_SEND_COMPLETE)          ||
     (infoRequest->dcmf.state==MPIDI_DCMF_ACKNOWLEGED)
    )
    {
      infoRequest->dcmf.state=MPIDI_DCMF_REQUEST_DONE_CANCELLED;
      MPID_Request_complete(infoRequest);
    }
  else if (info->msginfo.type == MPIDI_DCMF_REQUEST_TYPE_CANCEL_ACKNOWLEDGE)
    {
      infoRequest->dcmf.state=MPIDI_DCMF_REQUEST_DONE_CANCELLED;
      if (infoRequest->dcmf.msginfo.msginfo.isRzv)
        {
          /*
           * Rendezvous Sends wait until a rzv ack is received to complete the
           * send. Since this request was canceled, no rzv ack will be sent
           * from the target node, and the send done callback must be
           * explicitly called here.
           */
          MPIDI_DCMF_SendDoneCB(infoRequest);
        }
    }

  return;
}


/**
 * \brief Process an incoming rendezvous acknowledgment from the
 * target (remote) node and complete the MPI_Send() on the origin
 * (local) node.
 *
 * \param[in] info The contents of the control message as a MPIDI_DCMF_MsgInfo struct
 * \param[in] peer The rank of the node sending the data
 */
static inline void MPIDI_DCMF_procRzvAck(MPIDI_DCMF_MsgInfo *info, unsigned peer)
{
  MPID_assert(info->msginfo.req != NULL);
  MPIDI_DCMF_SendDoneCB((MPID_Request *)info->msginfo.req);
}


/**
 * \brief This is the general PT2PT control message call-back
 *
 * \param[in] clientdata Opaque client data
 * \param[in] p    The contents of the control message as a DCMF_Control_t
 * \param[in] peer The rank of the node sending the data
 */
void MPIDI_BG2S_ControlCB(void *clientdata, const DCMF_Control_t * p, unsigned peer)
{
  MPID_assert_debug(sizeof(DCMF_Control_t) == sizeof(MPIDI_DCMF_MsgInfo));
  MPIDI_DCMF_MsgInfo * info = (MPIDI_DCMF_MsgInfo *) p;

  switch (info->msginfo.type)
    {
    case MPIDI_DCMF_REQUEST_TYPE_SSEND_ACKNOWLEDGE:
      MPIDI_DCMF_procSyncAck(info, peer);
      break;
    case MPIDI_DCMF_REQUEST_TYPE_CANCEL_REQUEST:
      MPIDI_DCMF_procCancelReq(info, peer);
      break;
    case MPIDI_DCMF_REQUEST_TYPE_CANCEL_ACKNOWLEDGE:
    case MPIDI_DCMF_REQUEST_TYPE_CANCEL_NOT_ACKNOWLEDGE:
      MPIDI_DCMF_procCanelAck(info, peer);
      break;
    case MPIDI_DCMF_REQUEST_TYPE_RENDEZVOUS_ACKNOWLEDGE:
      MPIDI_DCMF_procRzvAck(info, peer);
      break;
    default:
      MPID_abort();
    }
  MPID_Progress_signal();
}
