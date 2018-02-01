/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* implements the naive intracomm allreduce, that is, reduce followed by bcast */
#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_sched_intra_naive
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_sched_intra_naive(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                      MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;

    rank = comm_ptr->rank;

    if ((sendbuf == MPI_IN_PLACE) && (rank != 0)) {
        mpi_errno =
            MPIR_Ireduce_sched_intra_auto(recvbuf, NULL, count, datatype, op, 0, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        mpi_errno =
            MPIR_Ireduce_sched_intra_auto(sendbuf, recvbuf, count, datatype, op, 0, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_BARRIER(s);

    mpi_errno = MPIR_Ibcast_sched_intra_auto(recvbuf, count, datatype, 0, comm_ptr, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
