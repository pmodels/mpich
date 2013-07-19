/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* test MPI_WIN_ALLOCATE_SHARED when size of total shared memory region is 0. */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    MPI_Win win;
    void *win_buf = NULL;
    int world_rank, shm_rank;
    MPI_Comm shm_comm;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, world_rank, MPI_INFO_NULL, &shm_comm);

    MPI_Comm_rank(shm_comm, &shm_rank);

    /* Some ranks allocate zero bytes */
    if (shm_rank % 2 == 0)
        MPI_Win_allocate_shared(0, sizeof(char), MPI_INFO_NULL, shm_comm, &win_buf, &win);
    else
        MPI_Win_allocate_shared(1, sizeof(char), MPI_INFO_NULL, shm_comm, &win_buf, &win);
    MPI_Win_free(&win);

    if (world_rank % 2 == 0)
        MPI_Win_allocate(0, sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &win_buf, &win);
    else
        MPI_Win_allocate(1, sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &win_buf, &win);
    MPI_Win_free(&win);

    win_buf = NULL;
    if (world_rank % 2 == 0)
        MPI_Win_create(NULL, 0, sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    else {
        win_buf = (void *) malloc(sizeof(char));
        MPI_Win_create(win_buf, 1, sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    }
    MPI_Win_free(&win);
    if (win_buf)
        free(win_buf);

    /* All ranks allocate zero bytes */
    MPI_Win_allocate_shared(0, sizeof(char), MPI_INFO_NULL, shm_comm, &win_buf, &win);
    MPI_Win_free(&win);
    MPI_Win_allocate(0, sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &win_buf, &win);
    MPI_Win_free(&win);
    MPI_Win_create(NULL, 0, sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    MPI_Win_free(&win);

    MPI_Comm_free(&shm_comm);

    if (world_rank == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;

}
