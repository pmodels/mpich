/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/pt2pt/mpid_cancel_send.c
 * \brief Device interface for canceling an MPI Send
 */
#include "mpidimpl.h"

static inline int
MPID_Cancel_send_rsm(MPID_Request * sreq)
{
  int flag;
  MPID_assert(sreq != NULL);

  /* ------------------------------------------------- */
  /* Check if we already have a cancel request pending */
  /* ------------------------------------------------- */
  MPIDI_DCMF_Request_cancel_pending(sreq, &flag);
  if (flag)
    return MPI_SUCCESS;

  /* ------------------------------------ */
  /* Try to cancel a send request to self */
  /* ------------------------------------ */
  if (MPID_Request_isSelf(sreq))
    {
      int source     = MPID_Request_getMatchRank(sreq);
      int tag        = MPID_Request_getMatchTag (sreq);
      int context_id = MPID_Request_getMatchCtxt(sreq);
      MPID_Request * rreq = MPIDI_Recvq_FDUR(sreq, source, tag, context_id);
      if (rreq)
        {
          MPID_assert(rreq->partner_request == sreq);
          MPID_Request_release(rreq);
          sreq->status.cancelled = TRUE;
          sreq->cc = 0;
        }
      return MPI_SUCCESS;
    }
  else
    {
      if(!sreq->comm)
        return MPI_SUCCESS;

      MPID_Request_increment_cc(sreq);
      MPIDI_DCMF_postCancelReq(sreq);

      return MPI_SUCCESS;
    }
}


static inline int
MPID_Cancel_send_ssm(MPID_Request * sreq)
{
  return SSM_ABORT();
}


int
MPID_Cancel_send(MPID_Request * sreq)
{
  if (MPIDI_Process.use_ssm)
    return MPID_Cancel_send_ssm(sreq);
  else
    return MPID_Cancel_send_rsm(sreq);
}
