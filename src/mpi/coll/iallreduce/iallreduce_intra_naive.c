/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "coll_util.h"

/* implements the naive intracomm allreduce, that is, reduce followed by bcast */
#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_intra_naive_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_intra_naive_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;

    rank = comm_ptr->rank;

    if ((sendbuf == MPI_IN_PLACE) && (rank != 0)) {
        mpi_errno = MPIR_Ireduce_intra_sched(recvbuf, NULL, count, datatype, op, 0, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        mpi_errno = MPIR_Ireduce_intra_sched(sendbuf, recvbuf, count, datatype, op, 0, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_BARRIER(s);

    mpi_errno = MPIR_Ibcast_intra_sched(recvbuf, count, datatype, 0, comm_ptr, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

