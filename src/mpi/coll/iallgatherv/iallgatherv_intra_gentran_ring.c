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
#include "iallgatherv_tsp_ring_algos_prototypes.h"
#include "tsp_undef.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv_intra_gentran_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv_intra_gentran_ring(const void *sendbuf, int sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        const int *recvcounts, const int *displs,
                                        MPI_Datatype recvtype, MPIR_Comm * comm,
                                        MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Iallgatherv_intra_ring(sendbuf, sendcount, sendtype,
                                                    recvbuf, recvcounts, displs, recvtype,
                                                    comm, req);

    return mpi_errno;
}
