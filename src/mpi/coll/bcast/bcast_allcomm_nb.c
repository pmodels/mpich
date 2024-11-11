/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Bcast_allcomm_nb(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                          MPIR_Comm * comm_ptr, int coll_group, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req_ptr = NULL;

    /* just call the nonblocking version and wait on it */
    mpi_errno = MPIR_Ibcast(buffer, count, datatype, root, comm_ptr, coll_group, &req_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIC_Wait(req_ptr);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Request_free(req_ptr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
