/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    int rank;
    double v1[2], v2[2];

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* Clearly incorrect program */
    MPI_Allreduce(v1, v2, rank ? 1 : 2, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
