/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidu_init_shm.h"
#include "build_nodemap.h"

int MPIDU_Init_shm_init(int rank, int size, int *nodemap)
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}

int MPIDU_Init_shm_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}

int MPIDU_Init_shm_barrier(void)
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}

int MPIDU_Init_shm_put(void *orig, size_t len)
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}

int MPIDU_Init_shm_get(int local_rank, void **target)
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}
