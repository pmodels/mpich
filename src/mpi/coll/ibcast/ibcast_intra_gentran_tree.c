/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "ibcast.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "ibcast_tsp_tree_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Ibcast_intra_gentran_tree(void *buffer, int count,
                                   MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr,
                                   int tree_type, int k, int chunk_size, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Ibcast_intra_tree(buffer, count, datatype, root,
                                               comm_ptr, request, tree_type, k, chunk_size);

    return mpi_errno;
}
