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

int MPIR_Ibcast_intra_gentran_ring(void *buffer, int count,
                                   MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr,
                                   int chunk_size, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    /* Ring algorithm is equivalent to kary tree algorithm with k = 1 */
    mpi_errno = MPII_Gentran_Ibcast_intra_tree(buffer, count, datatype, root,
                                               comm_ptr, request, MPIR_TREE_TYPE_KARY, 1,
                                               chunk_size);

    return mpi_errno;
}
