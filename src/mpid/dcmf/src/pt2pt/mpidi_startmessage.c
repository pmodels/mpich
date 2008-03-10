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
MPIDI_DCMF_Send(MPID_Request  * sreq,
                char          * sndbuf,
                unsigned        sndlen)
{
  int rc;
  DCMF_Callback_t cb_done;
  MPIDI_DCMF_MsgInfo * msginfo = &sreq->dcmf.msginfo;
  MPID_assert((((unsigned) msginfo )&0x0F)==0);
  MPID_assert((((unsigned) sreq    )&0x0f)==0);

  if ( sndlen==0 || sndlen<MPIDI_Process.eager_limit)
    {
      cb_done.function   = (void (*)(void *))MPIDI_DCMF_SendDoneCB;
      cb_done.clientdata = sreq;

      rc = DCMF_Send (&MPIDI_Protocols.send,
                      &sreq->dcmf.msg,
                      cb_done,
                      DCMF_MATCH_CONSISTENCY,
                      MPID_Request_getPeerRank(sreq),
                      sndlen,
                      sndbuf,
                      msginfo->quad,
                      1);
    }
  else
    {
      MPIDI_DCMF_RzvEnvelope rzv_envelope;

      /* Set the isRzv bit in the SEND request. This is important for      */
      /* canceling requests.                                               */
      sreq->dcmf.msginfo.msginfo.isRzv = 1;

      /* The rendezvous information, such as the origin/local/sender       */
      /* node's send buffer and the number of bytes the origin node wishes */
      /* to send, is sent as the payload of the request-to-send (RTS)      */
      /* message.                                                          */
      MPIDI_DCMF_RzvInfo * rzvinfo = &sreq->dcmf.rzvinfo;
      rzvinfo->sndbuf        = sndbuf;
      rzvinfo->sndlen        = sndlen;

      MPID_assert_debug(sizeof(MPIDI_DCMF_RzvEnvelope) == 32);
      memcpy(&rzv_envelope.msginfo, &sreq->dcmf.msginfo, sizeof(MPIDI_DCMF_MsgInfo));
      memcpy(&rzv_envelope.rzvinfo, &sreq->dcmf.rzvinfo, sizeof(MPIDI_DCMF_RzvInfo));

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
                      rzv_envelope.quad,
                      2);
    }

  MPID_assert(rc == DCMF_SUCCESS);
  return rc;
}

/* ----------------------------------------------------------------------- */
/*   Start a message send.                                                 */
/* ----------------------------------------------------------------------- */

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
