/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int i;
    double dStart, dFinish, dDuration;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &i);

    dStart = MPI_Wtime();
    MTestSleep(1);
    dFinish = MPI_Wtime();
    dDuration = dFinish - dStart;

    printf("start:%g\nfinish:%g\nduration:%g\n", dStart, dFinish, dDuration);

    MPI_Finalize();
    return 0;
}
