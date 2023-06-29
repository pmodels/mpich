/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "cma_post.h"
#include "mpidu_init_shm.h"

int MPIDI_CMA_init_world(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

#ifndef MPIDI_CH4_SHM_ENABLE_CMA
    MPIR_CVAR_CH4_CMA_ENABLE = 0;
#endif

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_CMA_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_FUNC_EXIT;
    return mpi_errno;
}
