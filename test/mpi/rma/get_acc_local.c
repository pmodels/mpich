/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */


#include "mpitest.h"

#ifdef MULTI_TESTS
#define run rma_get_acc_local
int run(const char *arg);
#endif

static int errs = 0;
static const int NITER = 1000;
static const int acc_val = 3;

int run(const char *arg)
{
    int rank, nproc;
    int out_val, i, counter = 0;
    MPI_Win win;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    MPI_Win_create(&counter, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    for (i = 0; i < NITER; i++) {
        MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
        MPI_Get_accumulate(&acc_val, 1, MPI_INT, &out_val, 1, MPI_INT,
                           rank, 0, 1, MPI_INT, MPI_SUM, win);
        MPI_Win_unlock(rank, win);

        if (out_val != acc_val * i) {
            errs++;
            printf("Error: got %d, expected %d at iter %d\n", out_val, acc_val * i, i);
            break;
        }
    }

    MPI_Win_free(&win);

    return errs;
}
