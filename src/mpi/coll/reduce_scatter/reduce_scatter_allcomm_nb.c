/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Reduce_scatter_allcomm_nb(const void *sendbuf, void *recvbuf, const MPI_Aint recvcounts[],
                                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                   MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req_ptr = NULL;

    /* just call the nonblocking version and wait on it */
    mpi_errno =
        MPIR_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr, &req_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIC_Wait(req_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Request_free(req_ptr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
