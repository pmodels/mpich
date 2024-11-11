/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
 * Pairwise Exchange
 *
 * We use a pairwise exchange algorithm similar to the one used in
 * intracommunicator alltoallw. Since the local and remote groups can
 * be of different sizes, we first compute the max of
 * local_group_size, remote_group_size. At step i, 0 <= i < max_size,
 * each process receives from src = (rank - i + max_size) % max_size
 * if src < remote_size, and sends to dst = (rank + i) % max_size if
 * dst < remote_size.
 *
 * FIXME: change algorithm to match intracommunicator alltoallv
 */

int MPIR_Alltoallw_inter_pairwise_exchange(const void *sendbuf, const MPI_Aint sendcounts[],
                                           const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                           void *recvbuf, const MPI_Aint recvcounts[],
                                           const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                           MPIR_Comm * comm_ptr, int coll_group,
                                           MPIR_Errflag_t errflag)
{
    int local_size, remote_size, max_size, i;
    int mpi_errno = MPI_SUCCESS;
    MPI_Status status;
    int src, dst, rank;
    char *sendaddr, *recvaddr;
    MPI_Datatype sendtype, recvtype;

    local_size = comm_ptr->local_size;
    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    /* Use pairwise exchange algorithm. */
    max_size = MPL_MAX(local_size, remote_size);
    for (i = 0; i < max_size; i++) {
        MPI_Aint sendcount, recvcount;
        src = (rank - i + max_size) % max_size;
        dst = (rank + i) % max_size;
        if (src >= remote_size) {
            src = MPI_PROC_NULL;
            recvaddr = NULL;
            recvcount = 0;
            recvtype = MPI_DATATYPE_NULL;
        } else {
            recvaddr = (char *) recvbuf + rdispls[src];
            recvcount = recvcounts[src];
            recvtype = recvtypes[src];
        }
        if (dst >= remote_size) {
            dst = MPI_PROC_NULL;
            sendaddr = NULL;
            sendcount = 0;
            sendtype = MPI_DATATYPE_NULL;
        } else {
            sendaddr = (char *) sendbuf + sdispls[dst];
            sendcount = sendcounts[dst];
            sendtype = sendtypes[dst];
        }

        mpi_errno = MPIC_Sendrecv(sendaddr, sendcount, sendtype,
                                  dst, MPIR_ALLTOALLW_TAG, recvaddr,
                                  recvcount, recvtype, src,
                                  MPIR_ALLTOALLW_TAG, comm_ptr, coll_group, &status, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
