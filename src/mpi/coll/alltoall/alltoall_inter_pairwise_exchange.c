/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

int MPIR_Alltoall_inter_pairwise_exchange(const void *sendbuf, MPI_Aint sendcount,
                                          MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                          MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                          MPIR_Errflag_t errflag)
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
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
    }

    return mpi_errno_ret;
}
