/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/pt2pt/mpidi_callback_rzv.c
 * \brief The callback for a new RZV RTS
 */
#include "mpidimpl.h"

/**
 * \brief The callback for a new RZV RTS
 * \note  Because this is a short message, the data is already received
 * \param[in]  clientdata   Unused
 * \param[in]  envelope     The 16-byte msginfo struct
 * \param[in]  count        The number of msginfo quads (1)
 * \param[in]  senderrank   The sender's rank
 * \param[in]  sndlen       The length of the incoming data
 * \param[in]  sndbuf       Where the data is stored
 */
void MPIDI_BG2S_RecvRzvCB(void                         * clientdata,
                          const DCQuad                 * msgquad,
                          unsigned                       count,
                          size_t                         senderrank,
                          const char                   * sndbuf,
                          size_t                         sndlen)
{
  MPID_Request * rreq = NULL;
  const MPIDI_DCMF_MsgEnvelope * envelope = (const MPIDI_DCMF_MsgEnvelope *)msgquad;
  MPIDI_DCMF_MsgInfo * msginfo = (MPIDI_DCMF_MsgInfo *)&envelope->envelope.msginfo;
  int found;


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
  MPID_Request_setRzv(rreq, 1);

  /* ----------------------------------------------------- */
  /* Save the rendezvous information for when the target   */
  /* node calls a receive function and the data is         */
  /* retreived from the origin node.                       */
  /* ----------------------------------------------------- */
  rreq->status.count = envelope->envelope.length;
  rreq->dcmf.envelope.envelope.length = envelope->envelope.length;
  rreq->dcmf.envelope.envelope.offset = envelope->envelope.offset;
  memcpy(&rreq->dcmf.envelope.envelope.memregion,
	 &envelope->envelope.memregion,
	 sizeof(DCMF_Memregion_t));

  /* ----------------------------------------- */
  /* figure out target buffer for request data */
  /* ----------------------------------------- */
  if (found)
    {
      MPIDI_DCMF_RendezvousTransfer (rreq);
    }

  /* ------------------------------------------------------------- */
  /* Request was not posted. */
  /* ------------------------------------------------------------- */
  else
    {
      rreq->dcmf.uebuf = NULL;
      rreq->dcmf.uebuflen = 0;
    }
}
