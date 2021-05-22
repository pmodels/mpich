/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "gpu_post.h"

int MPIDI_GPU_init_local(void)
{
    return MPI_SUCCESS;
}

int MPIDI_GPU_init_world(void)
{
    return MPI_SUCCESS;
}

int MPIDI_GPU_mpi_finalize_hook(void)
{
    return MPI_SUCCESS;
}
