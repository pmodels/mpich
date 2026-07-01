/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

/* This simple test reads stdin and copies to stdout */
#include <stdio.h>
#include "mpi.h"

/* style: allow:puts:1 sig:0 */
/* style: allow:fgets:1 sig:0 */

int main(int argc, char *argv[])
{
    int rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        char buf[128];
        while (fgets(buf, sizeof(buf), stdin)) {
            puts(buf);
        }
    }
    MPI_Finalize();

    return 0;
}
