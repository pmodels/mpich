/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Alltoallv_allcomm_nb(const void *sendbuf, const MPI_Aint * sendcounts,
                              const MPI_Aint * sdispls, MPI_Datatype sendtype, void *recvbuf,
                              const MPI_Aint * recvcounts, const MPI_Aint * rdispls,
                              MPI_Datatype recvtype, MPIR_Comm * comm_ptr, int collattr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req_ptr = NULL;

    /* just call the nonblocking version and wait on it */
    mpi_errno =
        MPIR_Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls,
                        recvtype, comm_ptr, collattr, &req_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIC_Wait(req_ptr);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Request_free(req_ptr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
