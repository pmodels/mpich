/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* sched version of CVAR and json based collective selection. Meant only for gentran scheduler */
int MPIR_TSP_Ireduce_sched_intra_tsp_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                          MPI_Datatype datatype, MPI_Op op, int root,
                                          MPIR_Comm * comm_ptr, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_TSP_Ireduce_sched_intra_tree(sendbuf, recvbuf, count, datatype, op, root,
                                          comm_ptr, MPIR_TREE_TYPE_KARY, 1,
                                          MPIR_CVAR_IREDUCE_RING_CHUNK_SIZE,
                                          MPIR_CVAR_IREDUCE_TREE_BUFFER_PER_CHILD, sched);


    return mpi_errno;
}
