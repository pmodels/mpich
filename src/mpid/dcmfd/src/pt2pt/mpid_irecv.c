/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/pt2pt/mpid_irecv.c
 * \brief ADI level implemenation of MPI_Irecv()
 */
#include "mpidimpl.h"

/**
 * \brief ADI level implemenation of MPI_Irecv()
 *
 * \param[in]  buf            The buffer to receive into
 * \param[in]  count          Number of expected elements in the buffer
 * \param[in]  datatype       The datatype of each element
 * \param[in]  rank           The sending rank
 * \param[in]  tag            The message tag
 * \param[in]  comm           Pointer to the communicator
 * \param[in]  context_offset Offset from the communicator context ID
 *
 * \param[out] request        Return a pointer to the new request object
 *
 * \returns An MPI Error code
 */
int MPID_Irecv(void          * buf,
               int             count,
               MPI_Datatype    datatype,
               int             rank,
               int             tag,
               MPID_Comm     * comm,
               int             context_offset,
               MPID_Request ** request)
{
  int mpi_errno;

  /* ---------------------------------------- */
  /* NULL rank means empty request            */
  /* ---------------------------------------- */
  if (rank == MPI_PROC_NULL)
    {
      MPID_Request * rreq;
      rreq = MPID_Request_create();
      if (!rreq)
        return MPIR_Err_create_code(MPI_SUCCESS,
                                    MPIR_ERR_FATAL,
                                    __FUNCTION__,
                                    __LINE__,
                                    MPI_ERR_OTHER,
                                    "**nomem",
                                    0);
      rreq->cc               = 0;
      rreq->kind             = MPID_REQUEST_RECV;
      MPIR_Status_set_procnull(&rreq->status);
      rreq->comm             = comm;
      MPIR_Comm_add_ref(comm);
      MPID_Request_setMatch(rreq, tag, rank, comm->recvcontext_id+context_offset);
      rreq->dcmf.userbuf      = buf;
      rreq->dcmf.userbufcount = count;
      rreq->dcmf.datatype     = datatype;
      *request = rreq;
      return MPI_SUCCESS;
    }

  mpi_errno = MPIDI_Irecv(buf,
                          count,
                          datatype,
                          rank,
                          tag,
                          comm,
                          context_offset,
                          MPI_STATUS_IGNORE,
                          request,
                          (char*)__FUNCTION__);

   return mpi_errno;
}
