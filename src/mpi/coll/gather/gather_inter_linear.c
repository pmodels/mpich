/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Linear
 *
 * Linear gather where root directly receives data from nonroot
 * processes.
 *
 * Cost: p.alpha + n.beta
 */

int MPIR_Gather_inter_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int remote_size, mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int i;
    MPI_Status status;
    MPI_Aint extent;

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        return MPI_SUCCESS;
    }

    remote_size = comm_ptr->remote_size;

    if (root == MPI_ROOT) {
        MPIR_Datatype_get_extent_macro(recvtype, extent);

        for (i = 0; i < remote_size; i++) {
            mpi_errno =
                MPIC_Recv(((char *) recvbuf + recvcount * i * extent), recvcount, recvtype, i,
                          MPIR_GATHER_TAG, comm_ptr, &status, errflag);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno);
        }
    } else {
        mpi_errno =
            MPIC_Send(sendbuf, sendcount, sendtype, root, MPIR_GATHER_TAG, comm_ptr, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno);
    }

    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
}
