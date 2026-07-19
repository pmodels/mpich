/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stdio.h"
#include "mpi.h"
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int provided, rank;

    MTest_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* The test here is a simple one that Finalize exits, so the only action
     * is to write no errors */
    MTest_Finalize(0);

    return 0;
}
