/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "mpitest.h"

int errors = 0;
const int NITER = 1000;
const int acc_val = 3;

int main(int argc, char **argv)
{
    int rank, nproc;
    int out_val, i, counter = 0;
    MPI_Win win;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    MPI_Win_create(&counter, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    for (i = 0; i < NITER; i++) {
        MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
        MPI_Get_accumulate(&acc_val, 1, MPI_INT, &out_val, 1, MPI_INT,
                           rank, 0, 1, MPI_INT, MPI_SUM, win);
        MPI_Win_unlock(rank, win);

        if (out_val != acc_val * i) {
            errors++;
            printf("Error: got %d, expected %d at iter %d\n", out_val, acc_val * i, i);
            break;
        }
    }

    MPI_Win_free(&win);

    if (errors == 0 && rank == 0)
        printf(" No errors\n");

    MPI_Finalize();

    return 0;
}
