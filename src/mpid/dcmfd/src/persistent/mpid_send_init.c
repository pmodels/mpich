/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/persistent/mpid_send_init.c
 * \brief ???
 */
/* This creates and initializes a persistent send request */

#include "mpidimpl.h"

/**
 * ***************************************************************************
 *              create a persistent send template
 * ***************************************************************************
 */

static inline int
MPID_PSendRequest (const void    * buf,
                   int             count,
                   MPI_Datatype    datatype,
                   int             rank,
                   int             tag,
                   MPID_Comm     * comm,
                   int             context_offset,
                   MPID_Request ** request)
{
  (*request) = MPID_Request_create();
  if ((*request) == NULL) return MPIR_ERR_MEMALLOCFAILED;
  (*request)->kind                 = MPID_PREQUEST_SEND;
  (*request)->comm                 = comm;
  MPIR_Comm_add_ref(comm);
  MPID_Request_setMatch((*request),tag,rank,comm->context_id + context_offset);
  (*request)->dcmf.userbuf          = (void *) buf;
  (*request)->dcmf.userbufcount     = count;
  (*request)->dcmf.datatype         = datatype;
  (*request)->partner_request      = NULL;
  (*request)->cc		    = 0;

  if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
    {
      MPID_Datatype_get_ptr(datatype, (*request)->dcmf.datatype_ptr);
      MPID_Datatype_add_ref((*request)->dcmf.datatype_ptr);
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
  int mpi_errno = MPID_PSendRequest (buf, count, datatype,
                                     rank, tag, comm, context_offset,
                                     request);
  if (mpi_errno != MPI_SUCCESS) return mpi_errno;
  MPID_Request_setType((*request), MPIDI_DCMF_REQUEST_TYPE_SEND);
  return MPI_SUCCESS;
}

/**
 * ***************************************************************************
 *              persistent ready-send
 * ***************************************************************************
 */

int MPID_Rsend_init(const void * buf,
                   int count,
                   MPI_Datatype datatype,
                   int rank,
                   int tag,
                   MPID_Comm * comm,
                   int context_offset,
                   MPID_Request ** request)
{
  int mpi_errno = MPID_PSendRequest (buf, count, datatype,
                                     rank, tag, comm, context_offset,
                                     request);
  if (mpi_errno != MPI_SUCCESS) return mpi_errno;
  MPID_Request_setType((*request), MPIDI_DCMF_REQUEST_TYPE_RSEND);
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
  int mpi_errno = MPID_PSendRequest (buf, count, datatype,
                                     rank, tag, comm, context_offset,
                                     request);
  if (mpi_errno != MPI_SUCCESS) return mpi_errno;
  MPID_Request_setType((*request), MPIDI_DCMF_REQUEST_TYPE_SSEND);
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
  int mpi_errno = MPID_PSendRequest (buf, count, datatype,
                                     rank, tag, comm, context_offset,
                                     request);
  if (mpi_errno != MPI_SUCCESS) return mpi_errno;
  MPID_Request_setType((*request), MPIDI_DCMF_REQUEST_TYPE_BSEND);
  return MPI_SUCCESS;
}
