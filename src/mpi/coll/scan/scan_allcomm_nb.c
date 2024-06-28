/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Scan_allcomm_nb(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                         MPI_Op op, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req_ptr = NULL;

    /* just call the nonblocking version and wait on it */
    mpi_errno = MPIR_Iscan(sendbuf, recvbuf, count, datatype, op, comm_ptr, &req_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIC_Wait(req_ptr);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Request_free(req_ptr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
