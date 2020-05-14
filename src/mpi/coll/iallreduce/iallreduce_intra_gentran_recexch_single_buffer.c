/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "iallreduce_tsp_recexch_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Iallreduce_intra_gentran_recexch_single_buffer(const void *sendbuf, void *recvbuf,
                                                        int count, MPI_Datatype datatype, MPI_Op op,
                                                        MPIR_Comm * comm, int k,
                                                        MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Iallreduce_intra_recexch(sendbuf, recvbuf, count,
                                                      datatype, op,
                                                      comm, req,
                                                      MPIR_IALLREDUCE_RECEXCH_TYPE_SINGLE_BUFFER,
                                                      k);

    return mpi_errno;
}
