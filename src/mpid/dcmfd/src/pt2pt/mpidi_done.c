/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/pt2pt/mpidi_done.c
 * \brief "Done" call-backs provided to the message layer for signaling completion
 */
#include "mpidimpl.h"


/**
 * \brief Message layer callback which is invoked on the origin node
 * when the send of the message is done
 *
 * \param[in,out] sreq MPI receive request object
 */
void MPIDI_DCMF_SendDoneCB (void *clientdata, DCMF_Error_t *err)
{
  MPID_Request * sreq = (MPID_Request *)clientdata;
  MPID_assert(sreq != NULL);

  if (sreq->dcmf.uebuf)
    MPIU_Free(sreq->dcmf.uebuf);
  sreq->dcmf.uebuf = NULL;

  if(sreq->status.cancelled == FALSE)
    {
      if(MPID_Request_getType(sreq) != MPIDI_DCMF_REQUEST_TYPE_SSEND)
        {
          sreq->dcmf.state = MPIDI_DCMF_ACKNOWLEGED;
          MPID_Request_complete(sreq);
        }
      else
        {
          if(sreq->dcmf.state == MPIDI_DCMF_ACKNOWLEGED)
            MPID_Request_complete(sreq);
          else
            sreq->dcmf.state = MPIDI_DCMF_SEND_COMPLETE;
        }
    }
  else
    MPID_Request_complete(sreq);
}


/**
 * \brief Message layer callback which is invoked on the target node
 * when the incoming message is complete.
 *
 * \param[in,out] rreq MPI receive request object
 */
void MPIDI_DCMF_RecvDoneCB (void *clientdata, DCMF_Error_t *err)
{
  MPID_Request * rreq = (MPID_Request *)clientdata;
  MPID_assert(rreq != NULL);
  switch (MPID_Request_getCA(rreq))
    {
    case MPIDI_DCMF_CA_UNPACK_UEBUF_AND_COMPLETE:
      {
        int smpi_errno;
        MPID_assert(rreq->dcmf.uebuf != NULL);
        // It is unsafe to check the user buffer against NULL.
        // Believe it or not, an IRECV can legally be posted with a NULL buffer.
        // MPID_assert(rreq->dcmf.userbuf != NULL);
        MPIDI_DCMF_Buffer_copy (rreq->dcmf.uebuf, /* source buffer */
                               rreq->dcmf.uebuflen,
                               MPI_CHAR,
                               &smpi_errno,
                               rreq->dcmf.userbuf, /* dest buffer */
                               rreq->dcmf.userbufcount, /* dest count */
                               rreq->dcmf.datatype, /* dest type */
                               (MPIDI_msg_sz_t*)&rreq->status.count,
                               &rreq->status.MPI_ERROR);
        /* free the unexpected data buffer */
        MPIU_Free(rreq->dcmf.uebuf); rreq->dcmf.uebuf = NULL;
        MPID_Request_complete(rreq);
        break;
      }
    case MPIDI_DCMF_CA_UNPACK_UEBUF_AND_COMPLETE_NOFREE:
      {
        int smpi_errno;
        MPID_assert(rreq->dcmf.uebuf != NULL);
        // It is unsafe to check the user buffer against NULL.
        // Believe it or not, an IRECV can legally be posted with a NULL buffer.
        // MPID_assert(rreq->dcmf.userbuf != NULL);
        MPIDI_DCMF_Buffer_copy (rreq->dcmf.uebuf, /* source buffer */
                               rreq->dcmf.uebuflen,
                               MPI_CHAR,
                               &smpi_errno,
                               rreq->dcmf.userbuf, /* dest buffer */
                               rreq->dcmf.userbufcount, /* dest count */
                               rreq->dcmf.datatype, /* dest type */
                               (MPIDI_msg_sz_t*)&rreq->status.count,
                               &rreq->status.MPI_ERROR);
        MPID_Request_complete(rreq);
        break;
      }
    case MPIDI_DCMF_CA_COMPLETE:
      {
        MPID_Request_complete(rreq);
        break;
      }
    default:
      {
        MPID_Abort(NULL, MPI_ERR_OTHER, -1, "Internal: unknown CA");
        break;
      }
    }
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
void MPIDI_DCMF_RecvRzvDoneCB (void *clientdata, DCMF_Error_t *err)
{
  MPID_Request * rreq = (MPID_Request *)clientdata;
  MPID_assert(rreq != NULL);

  /* Is it neccesary to save the original value of the 'type' field ?? */
  unsigned original_value = MPID_Request_getType(rreq);
  MPID_Request_setType(rreq, MPIDI_DCMF_REQUEST_TYPE_RENDEZVOUS_ACKNOWLEDGE);
  DCMF_Control (&MPIDI_Protocols.control,
                DCMF_MATCH_CONSISTENCY,
                rreq->dcmf.peerrank,
                &rreq->dcmf.envelope.envelope.msginfo.quad);
  MPID_Request_setType(rreq, original_value);

  DCMF_Memregion_destroy(&rreq->dcmf.memregion);

  MPIDI_DCMF_RecvDoneCB (rreq, NULL);
}
