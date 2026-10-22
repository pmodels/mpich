/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run topo_dims0
int run(const char *arg);
#endif

/*
 * Test a corner case. See https://github.com/mpi-forum/mpi-issues/issues/72
 */
int run(const char *arg)
{
    int mpi_error, errs = 0;

    /* 0 dimensional test */
    mpi_error = MPI_Dims_create(1, 0, NULL);
    if (mpi_error != MPI_SUCCESS) {
        errs++;
        printf("Dims_create should succeed if nnodes = 1 and ndims = 0\n");
    }

    return errs;
}
