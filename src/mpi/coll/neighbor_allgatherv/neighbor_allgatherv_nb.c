/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Neighbor_allgatherv_nb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Neighbor_allgatherv_nb(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                void *recvbuf, const int recvcounts[], const int displs[],
                                MPI_Datatype recvtype, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req_ptr = NULL;

    /* just call the nonblocking version and wait on it */
    mpi_errno = MPIR_Ineighbor_allgatherv(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcounts, displs, recvtype,
                                          comm_ptr, &req_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIR_Request_wait_and_complete(req_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
