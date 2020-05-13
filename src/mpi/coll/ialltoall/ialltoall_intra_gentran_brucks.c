/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "ialltoall_tsp_brucks_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Ialltoall_intra_gentran_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        MPIR_Comm * comm_ptr, int k, int buffer_per_phase,
                                        MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Ialltoall_intra_brucks(sendbuf, sendcount, sendtype,
                                                    recvbuf, recvcount, recvtype,
                                                    comm_ptr, request, k, buffer_per_phase);

    return mpi_errno;
}
