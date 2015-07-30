/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    int nproc = (1000 * 1000 * 1000);
    int ret[3] = { 0, 0, 0 };
    int errs = 0, i, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Dims_create(nproc, 3, ret);

    for (i = 0; i < 3; i++)
        if (ret[i] != 1000)
            errs++;

    if (!errs && rank == 0)
        printf(" No Errors\n");

    MPI_Finalize();
    return 0;
}
