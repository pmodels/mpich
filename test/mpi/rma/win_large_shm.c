/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* test MPI_WIN_ALLOCATE and MPI_WIN_ALLOCATE_SHARED when allocating
   SHM memory with size of 1GB per process */

#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    int my_rank, shared_rank;
    void *mybase = NULL;
    MPI_Win win;
    MPI_Info win_info;
    MPI_Comm shared_comm;
    int i;
    int shm_win_size = 1024 * 1024 * 1024 * sizeof(char);       /* 1GB */

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    for (i = 0; i < 2; i++) {
        if (i == 0) {
            MPI_Info_create(&win_info);
            MPI_Info_set(win_info, (char *) "alloc_shm", (char *) "true");
        }
        else {
            win_info = MPI_INFO_NULL;
        }

        MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, my_rank, MPI_INFO_NULL,
                            &shared_comm);

        MPI_Comm_rank(shared_comm, &shared_rank);

        /* every processes allocate 1GB window memory */
        MPI_Win_allocate(shm_win_size, sizeof(char), win_info, MPI_COMM_WORLD, &mybase, &win);

        MPI_Win_free(&win);

        MPI_Win_allocate_shared(shm_win_size, sizeof(char), win_info, shared_comm, &mybase, &win);

        MPI_Win_free(&win);

        /* some processes allocate 1GB and some processes allocate zero bytes */
        if (my_rank % 2 == 0)
            MPI_Win_allocate(shm_win_size, sizeof(char), win_info, MPI_COMM_WORLD, &mybase, &win);
        else
            MPI_Win_allocate(0, sizeof(char), win_info, MPI_COMM_WORLD, &mybase, &win);

        MPI_Win_free(&win);

        if (shared_rank % 2 == 0)
            MPI_Win_allocate_shared(shm_win_size, sizeof(char), win_info, shared_comm, &mybase,
                                    &win);
        else
            MPI_Win_allocate_shared(0, sizeof(char), win_info, shared_comm, &mybase, &win);

        MPI_Win_free(&win);

        /* some processes allocate 1GB and some processes allocate smaller bytes */
        if (my_rank % 2 == 0)
            MPI_Win_allocate(shm_win_size, sizeof(char), win_info, MPI_COMM_WORLD, &mybase, &win);
        else
            MPI_Win_allocate(shm_win_size / 2, sizeof(char), win_info, MPI_COMM_WORLD, &mybase,
                             &win);

        MPI_Win_free(&win);

        /* some processes allocate 1GB and some processes allocate smaller bytes */
        if (shared_rank % 2 == 0)
            MPI_Win_allocate_shared(shm_win_size, sizeof(char), win_info, shared_comm, &mybase,
                                    &win);
        else
            MPI_Win_allocate_shared(shm_win_size / 2, sizeof(char), win_info, shared_comm, &mybase,
                                    &win);

        MPI_Win_free(&win);

        MPI_Comm_free(&shared_comm);

        if (i == 0)
            MPI_Info_free(&win_info);
    }

    if (my_rank == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
