/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"
#include "ucx_noinline.h"

int MPIDI_UCX_mpi_psend_init_hook(void *buf, int partitions, MPI_Aint count,
                                  MPI_Datatype datatype, int dest, int tag,
                                  MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    return MPI_SUCCESS;
}

int MPIDI_UCX_mpi_precv_init_hook(void *buf, int partitions, MPI_Aint count,
                                  MPI_Datatype datatype, int source, int tag,
                                  MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    return MPI_SUCCESS;
}

int MPIDI_UCX_precv_matched_hook(MPIR_Request * part_req)
{
    int mpi_errno = MPI_SUCCESS;

    /* If MPI_START has been called, notify sender CTS */
    if (MPIR_Part_request_is_active(part_req)) {
        mpi_errno = MPIDIG_part_issue_cts(part_req);
    }

    return mpi_errno;
}
