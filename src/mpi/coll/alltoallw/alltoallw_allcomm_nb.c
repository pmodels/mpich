/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Alltoallw_allcomm_nb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoallw_allcomm_nb(const void *sendbuf, const int sendcounts[], const int sdispls[],
                              const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                              const int rdispls[], const MPI_Datatype recvtypes[],
                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    MPIR_Request *req_ptr = NULL;

    /* just call the nonblocking version and wait on it */
    mpi_errno =
        MPIR_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls,
                        recvtypes, comm_ptr, &req_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    if (req_ptr)
        req = req_ptr->handle;

    mpi_errno = MPIR_Wait(&req, MPI_STATUS_IGNORE);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
