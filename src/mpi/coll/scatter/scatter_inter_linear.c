/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Linear
 *
 * Linear scatter where root sends data to each other process.
 *
 * Cost: p.alpha + n.beta
 */

int MPIR_Scatter_inter_linear(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                              void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                              MPIR_Comm * comm_ptr, int coll_group, MPIR_Errflag_t errflag)
{
    int remote_size, mpi_errno = MPI_SUCCESS;
    int i;
    MPI_Status status;
    MPI_Aint extent;

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        return MPI_SUCCESS;
    }

    remote_size = comm_ptr->remote_size;

    if (root == MPI_ROOT) {
        MPIR_Datatype_get_extent_macro(sendtype, extent);
        for (i = 0; i < remote_size; i++) {
            mpi_errno =
                MPIC_Send(((char *) sendbuf + sendcount * i * extent), sendcount, sendtype, i,
                          MPIR_SCATTER_TAG, comm_ptr, errflag);
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else {
        mpi_errno =
            MPIC_Recv(recvbuf, recvcount, recvtype, root, MPIR_SCATTER_TAG, comm_ptr, &status);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
