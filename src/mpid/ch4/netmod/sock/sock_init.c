/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "sock_impl.h"

int MPIDI_SOCK_init_local(int *tag_bits)
{
    /* TODO: not implemented yet */
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_SOCK_mpi_finalize_hook(void)
{
    /* TODO: not implemented yet */
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_SOCK_mpi_free_mem(void *ptr)
{
    return MPIDIG_mpi_free_mem(ptr);
}

void *MPIDI_SOCK_mpi_alloc_mem(MPI_Aint size, MPIR_Info * info_ptr)
{
    return MPIDIG_mpi_alloc_mem(size, info_ptr);
}
