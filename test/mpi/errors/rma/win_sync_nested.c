/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "win_sync.h"

int main(int argc, char *argv[])
{
    int rank, nproc, i;
    int errors = 0, all_errors = 0;
    int buf, *my_buf;
    MPI_Win win;
    MPI_Group world_group;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    MPI_Win_create(&buf, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);

    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    MPI_Win_post(world_group, 0, win);
    MPI_Win_start(world_group, 0, win);

    my_buf = malloc(nproc * sizeof(int));

    for (i = 0; i < nproc; i++) {
        MPI_Get(&my_buf[i], 1, MPI_INT, i, 0, 1, MPI_INT, win);
    }

    /* This should fail, because the window is in an active target access epoch. */
    CHECK_ERR(MPI_Win_start(world_group, 0, win));

    MPI_Win_complete(win);

    /* This should fail, because the window is not in an active target access epoch. */
    CHECK_ERR(MPI_Win_complete(win));

    MPI_Win_wait(win);
    MPI_Win_free(&win);

    free(my_buf);
    MPI_Group_free(&world_group);

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");
    MPI_Finalize();

    return 0;
}
