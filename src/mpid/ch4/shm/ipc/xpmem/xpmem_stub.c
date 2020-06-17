/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "xpmem_post.h"

int MPIDI_XPMEM_mpi_init_hook(int rank, int size, int *tag_bits)
{
    return MPI_SUCCESS;
}

int MPIDI_XPMEM_mpi_finalize_hook(void)
{
    return MPI_SUCCESS;
}

int MPIDI_XPMEM_ipc_handle_map(MPIDI_XPMEM_ipc_handle_t handle, void **vaddr)
{
    return MPI_SUCCESS;
}
