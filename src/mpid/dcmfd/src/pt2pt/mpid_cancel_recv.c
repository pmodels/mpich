/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/pt2pt/mpid_cancel_recv.c
 * \brief Device interface for canceling an MPI Recv
 */
#include "mpidimpl.h"

int MPID_Cancel_recv(MPID_Request * rreq)
{
  MPID_assert(rreq->kind == MPID_REQUEST_RECV);
  if (MPIDI_Recvq_FDPR(rreq))
    {
      rreq->status.cancelled = TRUE;
      rreq->status.count = 0;
      MPID_Request_set_completed(rreq);
      MPID_Request_release(rreq);
    }
  /* This is successful, even if the recv isn't cancelled */
  return MPI_SUCCESS;
}
