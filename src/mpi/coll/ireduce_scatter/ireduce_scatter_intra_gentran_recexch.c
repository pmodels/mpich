/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "ireduce_scatter_tsp_recexch_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Ireduce_scatter_intra_gentran_recexch(const void *sendbuf, void *recvbuf,
                                               const int *recvcounts, MPI_Datatype datatype,
                                               MPI_Op op, MPIR_Comm * comm, int k,
                                               MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Ireduce_scatter_intra_recexch(sendbuf, recvbuf, recvcounts,
                                                           datatype, op,
                                                           comm, req, k,
                                                           IREDUCE_SCATTER_RECEXCH_TYPE_DISTANCE_DOUBLING);

    return mpi_errno;
}
