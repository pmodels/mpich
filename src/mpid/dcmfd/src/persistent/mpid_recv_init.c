/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/persistent/mpid_recv_init.c
 * \brief ???
 */

/* This creates and initializes a persistent recv request */

#include "mpidimpl.h"

int MPID_Recv_init(void * buf,
                   int count,
                   MPI_Datatype datatype,
                   int rank,
                   int tag,
                   MPID_Comm * comm,
                   int context_offset,
                   MPID_Request ** request)
{
  MPID_Request * rreq = MPID_Request_create();
  if (rreq == NULL) {
    *request = NULL;
    return MPIR_ERR_MEMALLOCFAILED;
  };

  rreq->kind = MPID_PREQUEST_RECV;
  rreq->comm = comm;
  MPIR_Comm_add_ref(comm);
  MPID_Request_setMatch(rreq,tag,rank,comm->recvcontext_id + context_offset);
  rreq->dcmf.userbuf = (char *) buf;
  rreq->dcmf.userbufcount = count;
  rreq->dcmf.datatype = datatype;
  rreq->partner_request = NULL;
  rreq->cc = 0;

  MPID_Request_setType(rreq, MPIDI_DCMF_REQUEST_TYPE_RECV);
  if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
    {
      MPID_Datatype_get_ptr(datatype, rreq->dcmf.datatype_ptr);
      MPID_Datatype_add_ref(rreq->dcmf.datatype_ptr);
    }

  *request = rreq;
  return MPI_SUCCESS;
}
