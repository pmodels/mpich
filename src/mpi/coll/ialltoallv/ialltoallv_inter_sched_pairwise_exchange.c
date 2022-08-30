/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Ialltoallv_inter_sched_pairwise_exchange(const void *sendbuf, const MPI_Aint sendcounts[],
                                                  const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                                  void *recvbuf, const MPI_Aint recvcounts[],
                                                  const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    /* Intercommunicator alltoallv. We use a pairwise exchange algorithm
     * similar to the one used in intracommunicator alltoallv. Since the
     * local and remote groups can be of different
     * sizes, we first compute the max of local_group_size,
     * remote_group_size. At step i, 0 <= i < max_size, each process
     * receives from src = (rank - i + max_size) % max_size if src <
     * remote_size, and sends to dst = (rank + i) % max_size if dst <
     * remote_size.
     */
    int mpi_errno = MPI_SUCCESS;
    int local_size, remote_size, max_size, i;
    MPI_Aint send_extent, recv_extent, sendtype_size, recvtype_size;
    int src, dst, rank;
    char *sendaddr, *recvaddr;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM);

    local_size = comm_ptr->local_size;
    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    /* Get extent of send and recv types */
    MPIR_Datatype_get_extent_macro(sendtype, send_extent);
    MPIR_Datatype_get_extent_macro(recvtype, recv_extent);
    MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);

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
        } else {
            recvaddr = (char *) recvbuf + rdispls[src] * recv_extent;
            recvcount = recvcounts[src];
        }
        if (dst >= remote_size) {
            dst = MPI_PROC_NULL;
            sendaddr = NULL;
            sendcount = 0;
        } else {
            sendaddr = (char *) sendbuf + sdispls[dst] * send_extent;
            sendcount = sendcounts[dst];
        }

        if (sendcount * sendtype_size == 0)
            dst = MPI_PROC_NULL;
        if (recvcount * recvtype_size == 0)
            src = MPI_PROC_NULL;

        mpi_errno = MPIR_Sched_send(sendaddr, sendcount, sendtype, dst, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Sched_recv(recvaddr, recvcount, recvtype, src, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Sched_barrier(s);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
