/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/pt2pt/persistent/mpid_recv_init.c
 * \brief ???
 */

/* This creates and initializes a persistent recv request */

#include <mpidimpl.h>

int MPID_Recv_init(void * buf,
                   int count,
                   MPI_Datatype datatype,
                   int rank,
                   int tag,
                   MPID_Comm * comm,
                   int context_offset,
                   MPID_Request ** request)
{
  MPID_Request * rreq = *request = MPIDI_Request_create2();

  rreq->kind = MPID_PREQUEST_RECV;
  rreq->comm = comm;
  MPIR_Comm_add_ref(comm);
  MPIDI_Request_setMatch(rreq, tag, rank, comm->recvcontext_id+context_offset);
  rreq->mpid.userbuf = buf;
  rreq->mpid.userbufcount = count;
  rreq->mpid.datatype = datatype;
  rreq->partner_request = NULL;
  MPIDI_Request_complete(rreq);

  MPIDI_Request_setPType(rreq, MPIDI_REQUEST_PTYPE_RECV);
  if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
    {
      MPID_Datatype_get_ptr(datatype, rreq->mpid.datatype_ptr);
      MPID_Datatype_add_ref(rreq->mpid.datatype_ptr);
    }

  return MPI_SUCCESS;
}
