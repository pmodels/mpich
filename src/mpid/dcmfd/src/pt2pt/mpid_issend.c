/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/pt2pt/mpid_issend.c
 * \brief ADI level implemenation of MPI_Issend()
 */
#include "mpidimpl.h"

/**
 * \brief ADI level implemenation of MPI_Issend()
 *
 * \param[in]  buf            The buffer to send
 * \param[in]  count          Number of elements in the buffer
 * \param[in]  datatype       The datatype of each element
 * \param[in]  rank           The destination rank
 * \param[in]  tag            The message tag
 * \param[in]  comm           Pointer to the communicator
 * \param[in]  context_offset Offset from the communicator context ID
 * \param[out] request        Return a pointer to the new request object
 *
 * \returns An MPI Error code
 *
 * This is a slight variation on mpid_ssend.c - basically, we *always*
 * want to return a send request even if the request is already
 * complete (as is in the case of sending to a NULL rank).
 */
int MPID_Issend(const void    * buf,
                int             count,
                MPI_Datatype    datatype,
                int             rank,
                int             tag,
                MPID_Comm     * comm,
                int             context_offset,
                MPID_Request ** request)
{
  MPID_Request * sreq = NULL;

  /* --------------------------- */
  /* special case: send-to-self  */
  /* --------------------------- */

  if (rank == comm->rank && comm->comm_kind != MPID_INTERCOMM)
    {
      /* I'm sending to myself! */
      int mpi_errno = MPIDI_Isend_self(buf,
                                       count,
                                       datatype,
                                       rank,
                                       tag,
                                       comm,
                                       context_offset,
                                       MPIDI_DCMF_REQUEST_TYPE_SEND,
                                       request);
      return mpi_errno;
    }

  /* --------------------- */
  /* create a send request */
  /* --------------------- */

  if (!(sreq = MPID_Request_create ()))
    {
      *request = NULL;
      int mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_FATAL,
                                           "mpid_send",
                                           __LINE__,
                                           MPI_ERR_OTHER,
                                           "**nomem", 0);
      return mpi_errno;
    }

  /* match info */
  MPID_Request_setMatch(sreq, tag,comm->rank,comm->context_id+context_offset);

  /* data buffer info */
  sreq->dcmf.userbuf          = (char *)buf;
  sreq->dcmf.userbufcount     = count;
  sreq->dcmf.datatype         = datatype;

  /* communicator & destination info */
  sreq->comm                 = comm;  MPIR_Comm_add_ref(comm);
  if (rank != MPI_PROC_NULL)
    MPID_Request_setPeerRank(sreq, comm->vcr[rank]);
  MPID_Request_setPeerRequest(sreq, sreq);

  /* message type info */
  sreq->kind = MPID_REQUEST_SEND;
  MPID_Request_setType (sreq, MPIDI_DCMF_REQUEST_TYPE_SSEND);
  MPID_Request_setSync (sreq, 1);

  /* ------------------------------ */
  /* special case: NULL destination */
  /* ------------------------------ */
  if (rank == MPI_PROC_NULL)
    {
      sreq->cc = 0;
      *request = sreq;
      return MPI_SUCCESS;
    }

  /* ----------------------------------------- */
  /*      start the message                    */
  /* ----------------------------------------- */

  MPIDI_DCMF_StartMsg (sreq);
  *request = sreq;
  return MPI_SUCCESS;
}
