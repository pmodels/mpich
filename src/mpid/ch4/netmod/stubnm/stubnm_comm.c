/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "stubnm_impl.h"

int MPIDI_STUBNM_mpi_comm_commit_pre_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}

int MPIDI_STUBNM_mpi_comm_commit_post_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}

int MPIDI_STUBNM_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}
