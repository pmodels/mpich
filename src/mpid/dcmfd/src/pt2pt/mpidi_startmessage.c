/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/pt2pt/mpidi_startmessage.c
 * \brief Funnel point for starting all MPI messages
 */
#include "mpidimpl.h"

/* ----------------------------------------------------------------------- */
/*          Helper function: gets the message underway once the request    */
/* and the buffers have been allocated.                                    */
/* ----------------------------------------------------------------------- */

static inline int
MPIDI_DCMF_Send_rsm(MPID_Request  * sreq,
                    char          * sndbuf,
                    unsigned        sndlen)
{
  int rc = DCMF_ERROR;
  DCMF_Callback_t cb_done;
  MPIDI_DCMF_MsgInfo * msginfo = &sreq->dcmf.envelope.envelope.msginfo;

  /* Always use the short protocol when sndlen is zero.
   * Use the short/eager protocol when sndlen is less than the eager limit.
   */
  if ( sndlen==0 || sndlen<MPIDI_Process.eager_limit)
    {
      cb_done.function   = MPIDI_DCMF_SendDoneCB;
      cb_done.clientdata = sreq;

      rc = DCMF_Send (&MPIDI_Protocols.send,
                      &sreq->dcmf.msg,
                      cb_done,
                      DCMF_MATCH_CONSISTENCY,
                      MPID_Request_getPeerRank(sreq),
                      sndlen,
                      sndbuf,
                      msginfo->quad,
                      DCQuad_sizeof(MPIDI_DCMF_MsgInfo));
    }
  /* Use the optimized rendezvous protocol when sndlen is equal to or
   * larger than the eager_limit, but less than the optrzv_limit.
   */
  else if ( sndlen < MPIDI_Process.optrzv_limit )
    {
      cb_done.function   = MPIDI_DCMF_SendDoneCB;
      cb_done.clientdata = sreq;

      rc = DCMF_Send (&MPIDI_Protocols.mrzv,
                      &sreq->dcmf.msg,
                      cb_done,
                      DCMF_MATCH_CONSISTENCY,
                      MPID_Request_getPeerRank(sreq),
                      sndlen,
                      sndbuf,
                      msginfo->quad,
                      DCQuad_sizeof(MPIDI_DCMF_MsgInfo));
    }
  /* Otherwise, use the default rendezvous protocol (glue implementation
   * that guarantees no unexpected data.
   */
  else
    {
      /* Set the isRzv bit in the SEND request. This is important for      */
      /* canceling requests.                                               */
      MPID_Request_setRzv(sreq, 1);

      /* The rendezvous information, such as the origin/local/sender       */
      /* node's send buffer and the number of bytes the origin node wishes */
      /* to send, is sent as the payload of the request-to-send (RTS)      */
      /* message.                                                          */
      size_t bytes;
      if (DCMF_SUCCESS == DCMF_Memregion_create(&sreq->dcmf.envelope.envelope.memregion, &bytes, sndlen, sndbuf, 0))
      {
        sreq->dcmf.envelope.envelope.offset = 0;
        sreq->dcmf.envelope.envelope.length = sndlen;

        /* Do not specify a callback function to be invoked when the RTS     */
        /* message has been sent. The MPI_Send is completed only when the    */
        /* target/remote/receiver node has completed a DCMF_Get from the     */
        /* origin node and has then sent a rendezvous acknowledgement (ACK)  */
        /* to the origin node to signify the end of the transfer.  When the  */
        /* ACK message is received by the origin node the same callback      */
        /* function is used to complete the MPI_Send as the non-rendezvous   */
        /* case below.                                                       */
        cb_done.function   = NULL;
        cb_done.clientdata = NULL;

        rc = DCMF_Send (&MPIDI_Protocols.rzv,
                        &sreq->dcmf.msg,
                        cb_done,
                        DCMF_MATCH_CONSISTENCY,
                        MPID_Request_getPeerRank(sreq),
                        0,
                        NULL,
                        sreq->dcmf.envelope.quad,
                        DCQuad_sizeof(MPIDI_DCMF_MsgEnvelope));
      }
    }

  MPID_assert(rc == DCMF_SUCCESS);
  return rc;
}

static inline int
MPIDI_DCMF_Send_ssm(MPID_Request  * sreq,
                    char          * sndbuf,
                    unsigned        sndlen)
{
  return SSM_ABORT();
}


static inline int
MPIDI_DCMF_Send(MPID_Request  * sreq,
                char          * sndbuf,
                unsigned        sndlen)
{
  if (MPIDI_Process.use_ssm)
    return MPIDI_DCMF_Send_ssm(sreq, sndbuf, sndlen);
  else
    return MPIDI_DCMF_Send_rsm(sreq, sndbuf, sndlen);
}


/*
 * \brief Central function for all sends.
 * \param [in,out] sreq Structure containing all relevant info about the message.
 */

void
MPIDI_DCMF_StartMsg (MPID_Request  * sreq)
{
  int data_sz, dt_contig;
  MPID_Datatype *dt_ptr;
  MPI_Aint dt_true_lb;

  /* ----------------------------------------- */
  /* prerequisites: not sending to a NULL rank */
  /* request already allocated                 */
  /* not sending to self                       */
  /* ----------------------------------------- */
  MPID_assert(sreq != NULL);

  /* ----------------------------------------- */
  /*   get the datatype info                   */
  /* ----------------------------------------- */
  MPIDI_Datatype_get_info (sreq->dcmf.userbufcount,
                           sreq->dcmf.datatype,
                           dt_contig, data_sz, dt_ptr, dt_true_lb);

  /* ----------------------------------------- */
  /* contiguous data type                      */
  /* ----------------------------------------- */
  if (dt_contig)
    {
      MPID_assert(sreq->dcmf.uebuf == NULL);
      MPIDI_DCMF_Send (sreq, (char *)sreq->dcmf.userbuf + dt_true_lb, data_sz);
      return;
    }

  /* ------------------------------------------- */
  /* allocate and populate temporary send buffer */
  /* ------------------------------------------- */
  if (sreq->dcmf.uebuf == NULL)
    {
      MPID_Segment              segment;

      sreq->dcmf.uebuf = MPIU_Malloc(data_sz);
      if (sreq->dcmf.uebuf == NULL)
        {
          sreq->status.MPI_ERROR = MPI_ERR_NO_SPACE;
          sreq->status.count = 0;
          MPID_Abort(NULL, MPI_ERR_NO_SPACE, -1,
                     "Unable to allocate non-contiguous buffer");
        }

      DLOOP_Offset last = data_sz;
      MPID_Segment_init (sreq->dcmf.userbuf,
                         sreq->dcmf.userbufcount,
                         sreq->dcmf.datatype,
                         &segment,0);
      MPID_Segment_pack (&segment, 0, &last, sreq->dcmf.uebuf);
      MPID_assert(last == data_sz);
    }

  MPIDI_DCMF_Send (sreq, sreq->dcmf.uebuf, data_sz);
}
