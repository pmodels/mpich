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
      MPID_Request * rreq;
      rreq = MPIDI_Recvq_FDUR(sreq);
      if (rreq)
        {
          MPID_assert(rreq->partner_request == sreq);
          MPIU_Object_set_ref(rreq, 0);
          MPID_Request_destroy(rreq);
          sreq->status.cancelled = TRUE;
          sreq->cc = 0;
          MPIU_Object_set_ref(sreq, 1);
        }
      return MPI_SUCCESS;
    }
  else
    {
      if (sreq->dcmf.state == MPIDI_DCMF_ACKNOWLEGED)
        {
          MPID_assert(0 == *sreq->cc_ptr);
          MPIU_Object_add_ref(sreq);
          MPID_Request_increment_cc(sreq);
        }

      if(!sreq->comm)
        return MPI_SUCCESS;

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
