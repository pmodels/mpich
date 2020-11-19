/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
 * Test a corner case. See https://github.com/mpi-forum/mpi-issues/issues/72
 */
int main(int argc, char *argv[])
{
    int mpi_error, errs = 0;
    MTest_Init(&argc, &argv);

    /* 0 dimensional test */
    mpi_error = MPI_Dims_create(1, 0, NULL);
    if (mpi_error != MPI_SUCCESS) {
        errs++;
        printf("Dims_create should succeed if nnodes = 1 and ndims = 0\n");
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
