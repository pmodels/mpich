/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "ibcast.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "ibcast_tsp_scatterv_allgatherv_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Ibcast_intra_gentran_scatterv_recexch_allgatherv(void *buffer, int count,
                                                          MPI_Datatype datatype, int root,
                                                          MPIR_Comm * comm_ptr, int scatterv_k,
                                                          int allgatherv_k, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Ibcast_intra_scatterv_allgatherv(buffer, count, datatype, root,
                                                              comm_ptr, scatterv_k, allgatherv_k,
                                                              request);

    return mpi_errno;
}
