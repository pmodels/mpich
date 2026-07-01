/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int errs = 0;
    int majversion, subversion;

    MTest_Init(&argc, &argv);

    MPI_Get_version(&majversion, &subversion);
    if (majversion != MPI_VERSION) {
        errs++;
        printf("Major version is %d but is %d in the mpi.h file\n", majversion, MPI_VERSION);
    }
    if (subversion != MPI_SUBVERSION) {
        errs++;
        printf("Minor version is %d but is %d in the mpi.h file\n", subversion, MPI_SUBVERSION);
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
