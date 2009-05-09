/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/pt2pt/mpidi_rendezvous.c
 * \brief Provide for a flow-control rendezvous-based send
 */
#include "mpidimpl.h"

inline void MPIDI_DCMF_RendezvousTransfer (MPID_Request * rreq)
{
  char *rcvbuf;
  unsigned rcvlen;

  /* --------------------------- */
  /* if synchronized, post ack.  */
  /* --------------------------- */
  if (MPID_Request_isSync(rreq))
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
  if (rreq->dcmf.envelope.envelope.length > dt_size)
    {
      rcvlen = dt_size;
      rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
      rreq->status.count = rcvlen;
    }
  else
    {
      rcvlen = rreq->dcmf.envelope.envelope.length;
    }

  /* -------------------------------------- */
  /* if buffer is contiguous ...            */
  /* -------------------------------------- */
  if (dt_contig)
    {
      MPID_Request_setCA(rreq, MPIDI_DCMF_CA_COMPLETE);
      rreq->dcmf.uebuf = NULL;
      rreq->dcmf.uebuflen = 0;
      rcvbuf = (char *)rreq->dcmf.userbuf + dt_true_lb;
    }

  /* --------------------------------------------- */
  /* buffer is non-contiguous. we need to allocate */
  /* a temporary buffer, and unpack later.         */
  /* --------------------------------------------- */
  else
    {
      MPID_Request_setCA(rreq, MPIDI_DCMF_CA_UNPACK_UEBUF_AND_COMPLETE);
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

      rcvbuf = rreq->dcmf.uebuf;
    }

  /* ---------------------------------------------------------------- */
  /* Get the data from the origin node.                               */
  /* ---------------------------------------------------------------- */
  DCMF_Callback_t cb = { MPIDI_DCMF_RecvRzvDoneCB, (void *)rreq };

  size_t bytes;
  DCMF_Memregion_create(&rreq->dcmf.memregion, &bytes, rcvlen, rcvbuf, 0);

  DCMF_Get (&MPIDI_Protocols.get,
            (DCMF_Request_t *) &rreq->dcmf.msg,
            cb,
            DCMF_MATCH_CONSISTENCY,
            MPID_Request_getPeerRank(rreq),
            rcvlen,
            &rreq->dcmf.envelope.envelope.memregion, // data src memregion (from rzv send node)
            &rreq->dcmf.memregion,                   // data dst memregion (on   rzv recv node)
            rreq->dcmf.envelope.envelope.offset,
            0);
}
