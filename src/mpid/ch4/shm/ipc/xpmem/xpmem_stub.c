/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "xpmem_post.h"

int MPIDI_XPMEM_init_local(void)
{
    return MPI_SUCCESS;
}

int MPIDI_XPMEM_init_world(void)
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
