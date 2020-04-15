/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "ireduce_tsp_tree_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Ireduce_intra_gentran_tree(const void *sendbuf, void *recvbuf, int count,
                                    MPI_Datatype datatype, MPI_Op op, int root,
                                    MPIR_Comm * comm_ptr, int tree_type, int k, int chunk_size,
                                    int buffer_per_child, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Ireduce_intra_tree(sendbuf, recvbuf, count, datatype, op, root,
                                                comm_ptr, request, tree_type, k, chunk_size,
                                                buffer_per_child);

    return mpi_errno;
}
