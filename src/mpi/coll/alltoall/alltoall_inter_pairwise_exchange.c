/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Intercommunicator alltoall
 *
 * We use a pairwise exchange algorithm similar to the one used in
 * intracommunicator alltoall for long messages.  Since the local and
 * remote groups can be of different sizes, we first compute the max
 * of local_group_size, remote_group_size.  At step i, 0 <= i <
 * max_size, each process receives from src = (rank - i + max_size) %
 * max_size if src < remote_size, and sends to dst = (rank + i) %
 * max_size if dst < remote_size.
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Alltoall_inter_pairwise_exchange
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoall_inter_pairwise_exchange(const void *sendbuf, int sendcount,
                                          MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                          MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                          MPIR_Errflag_t * errflag)
{
    int local_size, remote_size, max_size, i;
    MPI_Aint sendtype_extent, recvtype_extent;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    int src, dst, rank;
    char *sendaddr, *recvaddr;

    local_size = comm_ptr->local_size;
    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    /* Get extent of send and recv types */
    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    /* Do the pairwise exchanges */
    max_size = MPL_MAX(local_size, remote_size);
    MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                     max_size * recvcount * recvtype_extent);
    MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                                     max_size * sendcount * sendtype_extent);
    for (i = 0; i < max_size; i++) {
        src = (rank - i + max_size) % max_size;
        dst = (rank + i) % max_size;
        if (src >= remote_size) {
            src = MPI_PROC_NULL;
            recvaddr = NULL;
        } else {
            recvaddr = (char *) recvbuf + src * recvcount * recvtype_extent;
        }
        if (dst >= remote_size) {
            dst = MPI_PROC_NULL;
            sendaddr = NULL;
        } else {
            sendaddr = (char *) sendbuf + dst * sendcount * sendtype_extent;
        }

        mpi_errno = MPIC_Sendrecv(sendaddr, sendcount, sendtype, dst,
                                  MPIR_ALLTOALL_TAG, recvaddr,
                                  recvcount, recvtype, src,
                                  MPIR_ALLTOALL_TAG, comm_ptr, &status, errflag);
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
