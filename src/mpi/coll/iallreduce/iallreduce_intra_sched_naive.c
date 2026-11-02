/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* implements the naive intracomm allreduce, that is, reduce followed by bcast */
int MPIR_Iallreduce_intra_sched_naive(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    const void *sendbuf = coll_sig->u.allreduce.sendbuf;
    void *recvbuf = coll_sig->u.allreduce.recvbuf;
    MPI_Aint count = coll_sig->u.allreduce.count;
    MPI_Datatype datatype = coll_sig->u.allreduce.datatype;
    MPI_Op op = coll_sig->u.allreduce.op;
    MPIR_Sched_t s = coll_sig->sched;
    int mpi_errno = MPI_SUCCESS;
    int rank;
    int size ATTRIBUTE((unused));

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, size);

    if ((sendbuf == MPI_IN_PLACE) && (rank != 0)) {
        MPIR_COLL_SCHED_INNER(Ireduce, coll_sig, recvbuf, NULL, count, datatype, op, 0, comm_ptr);
    } else {
        MPIR_COLL_SCHED_INNER(Ireduce, coll_sig, sendbuf, recvbuf, count, datatype, op, 0,
                              comm_ptr);
    }

    MPIR_SCHED_BARRIER(s);

    MPIR_COLL_SCHED_INNER(Ibcast, coll_sig, recvbuf, count, datatype, 0, comm_ptr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
