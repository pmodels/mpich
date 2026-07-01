/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"

void *MPIDI_SHM_mpi_alloc_mem(MPI_Aint size, MPIR_Info * info_ptr)
{
    void *ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_alloc_mem(size, info_ptr);

    MPIR_FUNC_EXIT;
    return ret;
}

int MPIDI_SHM_mpi_free_mem(void *ptr)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_free_mem(ptr);

    MPIR_FUNC_EXIT;
    return ret;
}
