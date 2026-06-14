/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpi.h"

/*
 * This is a special test to check that mpiexec handles zero/non-zero
 * return status from an application
 */
int main(int argc, char *argv[])
{
    MPI_Init(0, 0);
    MPI_Finalize();
    return 1;
}
