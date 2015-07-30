/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <mpi.h>
#include <mpitest.h>

int main(int argc, char *argv[])
{
    MPI_Win win;
    int flag, tmp, rank;
    int base[1024], errs = 0;
    MPI_Request req;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Win_create(base, 1024 * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    if (rank == 0) {
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_unlock(0, win);
    }
    else {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
        MPI_Rput(&tmp, 1, MPI_INT, 0, 0, 1, MPI_INT, win, &req);
        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_unlock(0, win);
    }

    MPI_Win_free(&win);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
