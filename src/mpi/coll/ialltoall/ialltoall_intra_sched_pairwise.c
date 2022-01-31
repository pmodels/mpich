/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Algorithm: Pairwise Exchange
 *
 * This pairwise exchange algorithm takes p-1 steps.
 *
 * We calculate the pairs by using an exclusive-or algorithm:
 *         for (i=1; i<comm_size; i++)
 *             dest = rank ^ i;
 * This algorithm doesn't work if the number of processes is not a power of
 * two. For a non-power-of-two number of processes, we use an
 * algorithm in which, in step i, each process  receives from (rank-i)
 * and sends to (rank+i).
 *
 * Cost = (p-1).alpha + n.beta
 *
 * where n is the total amount of data a process needs to send to all
 * other processes.
 */
int MPIR_Ialltoall_intra_sched_pairwise(const void *sendbuf, MPI_Aint sendcount,
                                        MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int src, dst, is_pof2;
    int rank, comm_size;
    MPI_Aint sendtype_extent, recvtype_extent;

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(sendbuf != MPI_IN_PLACE);       /* we do not handle in-place */
#endif

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    /* Make local copy first */
    mpi_errno = MPIR_Sched_copy(((char *) sendbuf + rank * sendcount * sendtype_extent),
                                sendcount, sendtype,
                                ((char *) recvbuf + rank * recvcount * recvtype_extent),
                                recvcount, recvtype, s);
    MPIR_ERR_CHECK(mpi_errno);

    is_pof2 = MPL_is_pof2(comm_size);

    /* Do the pairwise exchanges */
    for (i = 1; i < comm_size; i++) {
        if (is_pof2 == 1) {
            /* use exclusive-or algorithm */
            src = dst = rank ^ i;
        } else {
            src = (rank - i + comm_size) % comm_size;
            dst = (rank + i) % comm_size;
        }

        mpi_errno = MPIR_Sched_send(((char *) sendbuf + dst * sendcount * sendtype_extent),
                                    sendcount, sendtype, dst, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Sched_recv(((char *) recvbuf + src * recvcount * recvtype_extent),
                                    recvcount, recvtype, src, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
