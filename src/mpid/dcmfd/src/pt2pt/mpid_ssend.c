/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/pt2pt/mpid_ssend.c
 * \brief ADI level implemenation of MPI_Ssend()
 */
#include "mpidimpl.h"

/**
 * \brief ADI level implemenation of MPI_Ssend()
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
 */
int MPID_Ssend(const void    * buf,
               int             count,
               MPI_Datatype    datatype,
               int             rank,
               int             tag,
               MPID_Comm     * comm,
               int             context_offset,
               MPID_Request ** request)
{
  MPID_Request * sreq = NULL;

  /* ------------------------------ */
  /* special case: NULL destination */
  /* ------------------------------ */

  if (rank == MPI_PROC_NULL)
    {
      *request = NULL;
      return MPI_SUCCESS;
    }

  /* --------------------------- */
  /* special case: send-to-self  */
  /* --------------------------- */

  else if (rank == comm->rank && comm->comm_kind != MPID_INTERCOMM)
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
                                       &sreq);
      if (MPIR_ThreadInfo.thread_provided <= MPI_THREAD_FUNNELED && sreq != NULL && sreq->cc != 0)
        {
          *request = NULL;
          mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_FATAL,
                                           __PRETTY_FUNCTION__,
                                           __LINE__,
                                           MPI_ERR_OTHER,
                                           "**dev|selfsenddeadlock", 0);
          return mpi_errno;
        }
      *request = sreq;
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
  MPID_Request_setPeerRank(sreq, comm->vcr[rank]);
  MPID_Request_setPeerRequest(sreq, sreq);

  /* message type info */
  sreq->kind = MPID_REQUEST_SEND;
  MPID_Request_setType (sreq, MPIDI_DCMF_REQUEST_TYPE_SSEND);
  MPID_Request_setSync (sreq, 1);

  /* ----------------------------------------- */
  /*      start the message                    */
  /* ----------------------------------------- */

  MPIDI_DCMF_StartMsg (sreq);
  *request = sreq;
  return MPI_SUCCESS;
}
