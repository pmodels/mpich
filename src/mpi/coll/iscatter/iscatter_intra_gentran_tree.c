/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "iscatter_tsp_tree_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Iscatter_intra_gentran_tree(const void *sendbuf, int sendcount,
                                     MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                     MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr, int k,
                                     MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Iscatter_intra_tree(sendbuf, sendcount, sendtype,
                                                 recvbuf, recvcount, recvtype,
                                                 root, comm_ptr, request, k);

    return mpi_errno;
}
