/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/pt2pt/mpid_recv.c
 * \brief ADI level implemenation of MPI_Recv()
 */
#include "mpidimpl.h"

/**
 * \brief ADI level implemenation of MPI_Recv()
 *
 * \param[in]  buf            The buffer to receive into
 * \param[in]  count          Number of expected elements in the buffer
 * \param[in]  datatype       The datatype of each element
 * \param[in]  rank           The sending rank
 * \param[in]  tag            The message tag
 * \param[in]  comm           Pointer to the communicator
 * \param[in]  context_offset Offset from the communicator context ID
 * \param[out] status         Update the status structure
 * \param[out] request        Return a pointer to the new request object
 *
 * \returns An MPI Error code
 */
int MPID_Recv(void          * buf,
              int             count,
              MPI_Datatype    datatype,
              int             rank,
              int             tag,
              MPID_Comm     * comm,
              int             context_offset,
              MPI_Status    * status,
              MPID_Request ** request)
{
  int mpi_errno;

  /* ---------------------------------------- */
  /* NULL rank means nothing to do.           */
  /* ---------------------------------------- */
  if (rank == MPI_PROC_NULL)
    {
      MPIR_Status_set_procnull(status);
      *request = NULL;
      return MPI_SUCCESS;
    }

  mpi_errno = MPIDI_Irecv(buf,
                          count,
                          datatype,
                          rank,
                          tag,
                          comm,
                          context_offset,
                          status,
                          request,
                          (char*)__FUNCTION__);

  if (*((*request)->cc_ptr) == 0)
    {
      MPID_Request_release(*request);
      *request = NULL;
    }

   return mpi_errno;
}
