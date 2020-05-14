/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "../iallreduce/iallreduce_tsp_recexch_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Ibarrier_intra_gentran_recexch(MPIR_Comm * comm, int k, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    void *recvbuf = NULL;

    mpi_errno =
        MPII_Gentran_Iallreduce_intra_recexch(MPI_IN_PLACE, recvbuf, 0, MPI_BYTE, MPI_SUM, comm,
                                              req, MPIR_IALLREDUCE_RECEXCH_TYPE_MULTIPLE_BUFFER, k);

    return mpi_errno;
}
