/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* implements the naive intracomm allreduce, that is, reduce followed by bcast */
int MPIR_Iallreduce_intra_sched_naive(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                      int coll_group, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, comm_size;

    MPIR_COLL_RANK_SIZE(comm_ptr, coll_group, rank, comm_size);
    if (comm_size == 1)
        goto fn_exit;

    if ((sendbuf == MPI_IN_PLACE) && (rank != 0)) {
        mpi_errno =
            MPIR_Ireduce_intra_sched_auto(recvbuf, NULL, count, datatype, op, 0, comm_ptr,
                                          coll_group, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno =
            MPIR_Ireduce_intra_sched_auto(sendbuf, recvbuf, count, datatype, op, 0, comm_ptr,
                                          coll_group, s);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIR_SCHED_BARRIER(s);

    mpi_errno = MPIR_Ibcast_intra_sched_auto(recvbuf, count, datatype, 0, comm_ptr, coll_group, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
