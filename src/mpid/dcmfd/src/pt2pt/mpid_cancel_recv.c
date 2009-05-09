/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/pt2pt/mpid_cancel_recv.c
 * \brief Device interface for canceling an MPI Recv
 */
#include "mpidimpl.h"

static inline int
MPID_Cancel_recv_rsm(MPID_Request * rreq)
{
  MPID_assert(rreq->kind == MPID_REQUEST_RECV);
  if (MPIDI_Recvq_FDPR(rreq))
    {
      rreq->status.cancelled = TRUE;
      rreq->status.count = 0;
      MPID_Request_set_completed(rreq);
    }
  /* This is successful, even if the recv isn't cancelled */
  return MPI_SUCCESS;
}


static inline int
MPID_Cancel_recv_ssm(MPID_Request * rreq)
{
  return SSM_ABORT();
}


int
MPID_Cancel_recv(MPID_Request * rreq)
{
  if (MPIDI_Process.use_ssm)
    return MPID_Cancel_recv_ssm(rreq);
  else
    return MPID_Cancel_recv_rsm(rreq);
}
