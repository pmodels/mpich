/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpi.h"

/* This is a program that tests the ability of mpiexec to timeout a process
   after no more than 3 minutes.  By default, it will run for 5 minutes */
int main(int argc, char *argv[])
{
    double t1;
    double deltaTime = 300;

    MPI_Init(0, 0);
    t1 = MPI_Wtime();
    while (MPI_Wtime() - t1 < deltaTime);
    MPI_Finalize();
    return 0;
}
