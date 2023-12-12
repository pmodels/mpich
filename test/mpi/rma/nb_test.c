/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpitest.h>

#ifdef MULTI_TESTS
#define run rma_nb_test
int run(const char *arg);
#endif

int run(const char *arg)
{
    MPI_Win win;
    int flag, tmp, rank;
    int base[1024], errs = 0;
    MPI_Request req;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Win_create(base, 1024 * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    if (rank == 0) {
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_unlock(0, win);
    } else {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
        MPI_Rput(&tmp, 1, MPI_INT, 0, 0, 1, MPI_INT, win, &req);
        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_unlock(0, win);
    }

    MPI_Win_free(&win);

    return errs;
}
