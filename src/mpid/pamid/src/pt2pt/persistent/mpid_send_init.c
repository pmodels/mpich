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
 * \file src/pt2pt/persistent/mpid_send_init.c
 * \brief ???
 */
/* This creates and initializes a persistent send request */

#include <mpidimpl.h>

/**
 * ***************************************************************************
 *              create a persistent send template
 * ***************************************************************************
 */

static inline int
MPID_PSendRequest(const void    * buf,
                  int             count,
                  MPI_Datatype    datatype,
                  int             rank,
                  int             tag,
                  MPID_Comm     * comm,
                  int             context_offset,
                  MPID_Request ** request)
{
  MPID_Request* sreq = *request = MPIDI_Request_create2();

  sreq->kind              = MPID_PREQUEST_SEND;
  sreq->comm              = comm;
  MPIR_Comm_add_ref(comm);
  MPIDI_Request_setMatch(sreq, tag, rank, comm->context_id+context_offset);
  sreq->mpid.userbuf      = (void*)buf;
  sreq->mpid.userbufcount = count;
  sreq->mpid.datatype     = datatype;
  sreq->partner_request   = NULL;
  MPIDI_Request_complete(sreq);

  if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
    {
      MPID_Datatype_get_ptr(datatype, sreq->mpid.datatype_ptr);
      MPID_Datatype_add_ref(sreq->mpid.datatype_ptr);
    }

  return MPI_SUCCESS;
}

/**
 * ***************************************************************************
 *              simple persistent send
 * ***************************************************************************
 */

int MPID_Send_init(const void * buf,
                   int count,
                   MPI_Datatype datatype,
                   int rank,
                   int tag,
                   MPID_Comm * comm,
                   int context_offset,
                   MPID_Request ** request)
{
  int mpi_errno = MPID_PSendRequest(buf,
                                    count,
                                    datatype,
                                    rank,
                                    tag,
                                    comm,
                                    context_offset,
                                    request);
  if (mpi_errno != MPI_SUCCESS)
    return mpi_errno;
  MPIDI_Request_setPType((*request), MPIDI_REQUEST_PTYPE_SEND);
  return MPI_SUCCESS;
}

/**
 * ***************************************************************************
 *              persistent synchronous send
 * ***************************************************************************
 */

int MPID_Ssend_init(const void * buf,
                   int count,
                   MPI_Datatype datatype,
                   int rank,
                   int tag,
                   MPID_Comm * comm,
                   int context_offset,
                   MPID_Request ** request)
{
  int mpi_errno = MPID_PSendRequest(buf,
                                    count,
                                    datatype,
                                    rank,
                                    tag,
                                    comm,
                                    context_offset,
                                    request);
  if (mpi_errno != MPI_SUCCESS)
    return mpi_errno;
  MPIDI_Request_setPType((*request), MPIDI_REQUEST_PTYPE_SSEND);
  return MPI_SUCCESS;
}

/**
 * ***************************************************************************
 *              persistent buffered send
 * ***************************************************************************
 */

int MPID_Bsend_init(const void * buf,
                    int count,
                    MPI_Datatype datatype,
                    int rank,
                    int tag,
                    MPID_Comm * comm,
                    int context_offset,
                    MPID_Request ** request)
{
  int mpi_errno = MPID_PSendRequest(buf,
                                    count,
                                    datatype,
                                    rank,
                                    tag,
                                    comm,
                                    context_offset,
                                    request);
  if (mpi_errno != MPI_SUCCESS)
    return mpi_errno;
  MPIDI_Request_setPType((*request), MPIDI_REQUEST_PTYPE_BSEND);
  return MPI_SUCCESS;
}
