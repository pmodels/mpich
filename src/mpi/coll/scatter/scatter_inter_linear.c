/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Linear
 *
 * Linear scatter where root sends data to each other process.
 *
 * Cost: p.alpha + n.beta
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Scatter_inter_linear
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Scatter_inter_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
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
        MPIR_Datatype_get_extent_macro(sendtype, extent);
        for (i = 0; i < remote_size; i++) {
            mpi_errno =
                MPIC_Send(((char *) sendbuf + sendcount * i * extent), sendcount, sendtype, i,
                          MPIR_SCATTER_TAG, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    } else {
        mpi_errno =
            MPIC_Recv(recvbuf, recvcount, recvtype, root, MPIR_SCATTER_TAG, comm_ptr, &status,
                      errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
}
