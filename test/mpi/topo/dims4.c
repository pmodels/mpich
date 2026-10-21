/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run topo_dims4
int run(const char *arg);
#endif

int run(const char *arg)
{
    int nproc = (1000 * 1000 * 1000);
    int ret[3] = { 0, 0, 0 };
    int errs = 0, i, rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Dims_create(nproc, 3, ret);

    for (i = 0; i < 3; i++)
        if (ret[i] != 1000)
            errs++;

    return errs;
}
