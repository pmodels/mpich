/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "ibcast.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "ibcast_tsp_scatter_recexch_allgather_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Ibcast_intra_gentran_scatter_recexch_allgather(void *buffer, int count,
                                                        MPI_Datatype datatype, int root,
                                                        MPIR_Comm * comm_ptr,
                                                        MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Ibcast_intra_scatter_recexch_allgather(buffer, count, datatype, root,
                                                                    comm_ptr,
                                                                    MPIR_CVAR_IBCAST_SCATTER_KVAL,
                                                                    MPIR_CVAR_IBCAST_ALLGATHER_RECEXCH_KVAL,
                                                                    request);

    return mpi_errno;
}
