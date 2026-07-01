/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpi.h"

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Finalize();

    return 0;
}
