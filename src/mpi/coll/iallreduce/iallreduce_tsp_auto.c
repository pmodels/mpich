/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "treealgo.h"

/* Routine to schedule a pipelined tree based allreduce */
int MPIR_TSP_Iallreduce_sched_intra_tsp_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                             MPI_Datatype datatype, MPI_Op op,
                                             MPIR_Comm * comm, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(comm->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    mpi_errno =
        MPIR_TSP_Iallreduce_sched_intra_recexch(sendbuf, recvbuf, count,
                                                datatype, op, comm,
                                                MPIR_IALLREDUCE_RECEXCH_TYPE_MULTIPLE_BUFFER,
                                                MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL, sched);
    return mpi_errno;
}
