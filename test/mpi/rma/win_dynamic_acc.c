/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include "mpitest.h"

#define ITER_PER_RANK 25

const int verbose = 0;

int main(int argc, char **argv)
{
    int i, rank, nproc;
    int errors = 0, all_errors = 0;
    int val = 0, one = 1;
    int iter;
    MPI_Aint *val_ptrs;
    MPI_Win dyn_win;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    iter = ITER_PER_RANK * nproc;

    val_ptrs = malloc(nproc * sizeof(MPI_Aint));
    MPI_Get_address(&val, &val_ptrs[rank]);

    MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, val_ptrs, 1, MPI_AINT, MPI_COMM_WORLD);

    MPI_Win_create_dynamic(MPI_INFO_NULL, MPI_COMM_WORLD, &dyn_win);
    MPI_Win_attach(dyn_win, &val, sizeof(int));

    for (i = 0; i < iter; i++) {
        MPI_Win_fence(MPI_MODE_NOPRECEDE, dyn_win);
        MPI_Accumulate(&one, 1, MPI_INT, i % nproc, val_ptrs[i % nproc], 1, MPI_INT, MPI_SUM,
                       dyn_win);
        MPI_Win_fence(MPI_MODE_NOSUCCEED, dyn_win);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Read and verify my data */
    if (val != iter) {
        errors++;
        printf("%d -- Got %d, expected %d\n", rank, val, iter);
    }

    MPI_Win_detach(dyn_win, &val);
    MPI_Win_free(&dyn_win);

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    free(val_ptrs);
    MPI_Finalize();

    return 0;
}
