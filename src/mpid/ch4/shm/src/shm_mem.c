/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"

void *MPIDI_SHM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    void *ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_ALLOC_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_ALLOC_MEM);

    ret = MPIDI_POSIX_mpi_alloc_mem(size, info_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_ALLOC_MEM);
    return ret;
}

int MPIDI_SHM_mpi_free_mem(void *ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_FREE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_FREE_MEM);

    ret = MPIDI_POSIX_mpi_free_mem(ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_FREE_MEM);
    return ret;
}
