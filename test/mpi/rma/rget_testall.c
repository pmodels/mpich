/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run rma_rget_testall
int run(const char *arg);
#endif

/* This test checks request-based get with MPI_Testall. Both the return value of
 * MPI_Testall and status.MPI_ERROR should be correctly set.
 *
 * Thanks for Joseph Schuchart reporting this bug in MPICH and contributing
 * the prototype of this test program. */

int run(const char *arg)
{
    int rank, size;
    MPI_Win win = MPI_WIN_NULL;
    int *baseptr = NULL;
    int errs = 0, mpi_errno = MPI_SUCCESS;
    int val1 = 0, val2 = 0, flag = 0;
    MPI_Request reqs[2];
    MPI_Status stats[2];

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Win_allocate(2 * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &baseptr, &win);

    /* Initialize window buffer */
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    baseptr[0] = 1;
    baseptr[1] = 2;
    MPI_Win_unlock(rank, win);

    /* Synchronize the processes before starting the second access epoch */
    MPI_Barrier(MPI_COMM_WORLD);

    /* Issue request-based get with testall. */
    MPI_Win_lock_all(0, win);
    MPI_Rget(&val1, 1, MPI_INT, 0, 0, 1, MPI_INT, win, &reqs[0]);
    MPI_Rget(&val2, 1, MPI_INT, 0, 1, 1, MPI_INT, win, &reqs[1]);

    do {
        mpi_errno = MPI_Testall(2, reqs, &flag, stats);
    } while (flag == 0);

    /* Check get value. */
    if (val1 != 1 || val2 != 2) {
        printf("%d - Got val1 = %d, val2 = %d, expected 1, 2\n", rank, val1, val2);
        fflush(stdout);
        errs++;
    }

    /* Check return error code. */
    if (mpi_errno != MPI_SUCCESS) {
        printf("%d - Got return errno %d, expected MPI_SUCCESS(%d)\n",
               rank, mpi_errno, MPI_SUCCESS);
        fflush(stdout);
        errs++;
    }

    MPI_Win_unlock_all(win);
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Win_free(&win);

    return errs;
}
