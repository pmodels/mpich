/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "stubnm_impl.h"

int MPIDI_STUBNM_mpi_init_hook(int rank, int size, int appnum, int *tag_bits,
                               MPIR_Comm * comm_world)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);
    return mpi_errno;
}

int MPIDI_STUBNM_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}

int MPIDI_STUBNM_post_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}

int MPIDI_STUBNM_get_vci_attr(int vci)
{
    MPIR_Assert(0);
    return 0;
}

int MPIDI_STUBNM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size, char **local_upids)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_upids_to_lupids(int size, size_t * remote_upid_size, char *remote_upids,
                                 int **remote_lupids)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr, int size, const int lpids[])
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_mpi_free_mem(void *ptr)
{
    return MPIDIG_mpi_free_mem(ptr);
}

void *MPIDI_STUBNM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    return MPIDIG_mpi_alloc_mem(size, info_ptr);
}
