/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/pt2pt/mpidi_callback_short.c
 * \brief The standard callback for a new short message
 */
#include "mpidimpl.h"

/**
 * \brief The standard callback for a new short message
 * \note  Because this is a short message, the data is already received
 * \param[in]  clientdata Unused
 * \param[in]  msginfo    The 16-byte msginfo struct
 * \param[in]  count      The number of msginfo quads (1)
 * \param[in]  senderrank The sender's rank
 * \param[in]  sndlen     The length of the incoming data
 * \param[in]  sndbuf     Where the data is stored
 */
void MPIDI_BG2S_RecvShortCB(void                     * clientdata,
                            const DCQuad             * msgquad,
                            unsigned                   count,
                            size_t                     senderrank,
                            const char               * sndbuf,
                            size_t                     sndlen)
{
  const MPIDI_DCMF_MsgInfo *msginfo = (const MPIDI_DCMF_MsgInfo *)msgquad;
  MPID_Request * rreq = NULL;
  int found;
  int rcvlen = sndlen;

  /* Handle cancel requests */
  if (msginfo->msginfo.type == MPIDI_DCMF_REQUEST_TYPE_CANCEL_REQUEST)
    {
      MPIDI_DCMF_procCancelReq(msginfo, senderrank);
      return;
    }

  /* -------------------------- */
  /*      match request         */
  /* -------------------------- */
  MPIDI_Message_match match;
  match.rank              = msginfo->msginfo.MPIrank;
  match.tag               = msginfo->msginfo.MPItag;
  match.context_id        = msginfo->msginfo.MPIctxt;

  rreq = MPIDI_Recvq_FDP_or_AEU(match.rank, match.tag, match.context_id, &found);

  if (rreq == NULL)
    {
      /* ------------------------------------------------- */
      /* we have failed to match the request.              */
      /* allocate and initialize a request object instead. */
      /* ------------------------------------------------- */

      int mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_FATAL,
                                           "mpid_recv",
                                           __LINE__,
                                           MPI_ERR_OTHER,
                                           "**nomem", 0);
      rreq->status.MPI_ERROR = mpi_errno;
      rreq->status.count     = 0;
      MPID_Abort(NULL, mpi_errno, -1, "Cannot allocate message");
    }

  /* -------------------------------------- */
  /* Signal that the recv has been started. */
  /* -------------------------------------- */
  MPID_Progress_signal ();

  /* ------------------------ */
  /* copy in information      */
  /* ------------------------ */
  rreq->status.MPI_SOURCE = match.rank;
  rreq->status.MPI_TAG    = match.tag;
  MPID_Request_setPeerRank(rreq,senderrank);
  MPID_Request_setPeerRequest(rreq,msginfo->msginfo.req);
  MPID_Request_setSync(rreq, msginfo->msginfo.isSync);
  MPID_Request_setRzv(rreq, 0);

  /*
   *
   * Whitespace to sync lines of code with mpidi_callback.c
   *
   */

  /* ----------------------------------------- */
  /* figure out target buffer for request data */
  /* ----------------------------------------- */
  MPID_Request_setCA(rreq, MPIDI_DCMF_CA_COMPLETE);
  rreq->status.count = rcvlen;
  if (found)
    {
      /* --------------------------- */
      /* request was already posted. */
      /* if synchronized, post ack.  */
      /* --------------------------- */
      if (msginfo->msginfo.isSync)
        MPIDI_DCMF_postSyncAck(rreq);

      /* -------------------------------------- */
      /* calculate message length for reception */
      /* calculate receive message "count"      */
      /* -------------------------------------- */
      unsigned dt_contig, dt_size;
      MPID_Datatype *dt_ptr;
      MPI_Aint dt_true_lb;
      MPIDI_Datatype_get_info (rreq->dcmf.userbufcount,
                               rreq->dcmf.datatype,
                               dt_contig,
                               dt_size,
                               dt_ptr,
                               dt_true_lb);

      /* -------------------------------------- */
      /* test for truncated message.            */
      /* -------------------------------------- */
      if (rcvlen > dt_size)
        {
          rcvlen = dt_size;
          rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
          rreq->status.count = rcvlen;
        }

      /* -------------------------------------- */
      /* if buffer is contiguous ...            */
      /* -------------------------------------- */
      if (dt_contig)
        {
          char *rcvbuf;
          rreq->dcmf.uebuf = NULL;
          rreq->dcmf.uebuflen = 0;
          rcvbuf = (char *)rreq->dcmf.userbuf + dt_true_lb;

          memcpy(rcvbuf, sndbuf, rcvlen);
          MPIDI_DCMF_RecvDoneCB(rreq, NULL);

          return;
        }

      /* --------------------------------------------- */
      /* buffer is non-contiguous. we need to specify  */
      /* the send buffer as temporary and unpack.      */
      /* --------------------------------------------- */
      else
        {
          MPID_Request_setCA(rreq, MPIDI_DCMF_CA_UNPACK_UEBUF_AND_COMPLETE_NOFREE);

          rreq->dcmf.uebuflen   = rcvlen ;
          rreq->dcmf.uebuf      = (char *) sndbuf ;

          MPIDI_DCMF_RecvDoneCB(rreq, NULL);
          return;
        }
    }

  /* ------------------------------------------------------------- */
  /* Request was not posted. We must allocate enough space to hold */
  /* the message temporarily and copy the data into the temporary  */
  /* buffer. The temporary buffer will be unpacked later.          */
  /* ------------------------------------------------------------- */
  rreq->dcmf.uebuflen   = rcvlen ;
  if ((rreq->dcmf.uebuf = MPIU_Malloc (rcvlen)) == NULL)
    {
      /* ------------------------------------ */
      /* creation of temporary buffer failed. */
      /* we are in trouble and must bail out. */
      /* ------------------------------------ */

      int mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_FATAL,
                                           "mpid_recv",
                                           __LINE__,
                                           MPI_ERR_OTHER,
                                           "**nomem", 0);
      rreq->status.MPI_ERROR = mpi_errno;
      rreq->status.count     = 0;
      MPID_Abort(NULL, mpi_errno, -1, "Cannot allocate unexpected buffer");
    }

  /* ------------------------------------------------ */
  /* Copy the data into the unexpected buffer.        */
  /* ------------------------------------------------ */
  memcpy(rreq->dcmf.uebuf, sndbuf, rreq->dcmf.uebuflen);
  MPIDI_DCMF_RecvDoneCB(rreq, NULL);
  return;
}
