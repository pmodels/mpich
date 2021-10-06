/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"

int MPIDI_UCX_dynamic_send(uint64_t remote_gpid, int tag, const void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**ucx_nm_notsupported");

    return mpi_errno;
}

int MPIDI_UCX_dynamic_recv(int tag, void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**ucx_nm_notsupported");

    return mpi_errno;
}

int MPIDI_UCX_get_local_upids(MPIR_Comm * comm, int **local_upid_size, char **local_upids)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**ucx_nm_notsupported");

    return mpi_errno;
}

int MPIDI_UCX_upids_to_gpids(int size, int *remote_upid_size, char *remote_upids,
                             uint64_t * remote_gpids)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**ucx_nm_notsupported");

    return mpi_errno;
}
