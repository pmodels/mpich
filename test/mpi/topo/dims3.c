/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run topo_dims3
int run(const char *arg);
#endif

static inline void print_err(int *dims, int ndims)
{
    int i;

    printf("[ ");
    for (i = 0; i < ndims; i++)
        printf("%d ", dims[i]);
    printf("] Suboptimal distribution!\n");
}

int run(const char *arg)
{
    int errs = 0;
    int dims[4], ndims, nnodes;


    for (ndims = 3; ndims <= 4; ndims++) {
        for (nnodes = 2; nnodes <= 4096; nnodes *= 2) {
            int i;
            for (i = 0; i < ndims; i++)
                dims[i] = 0;

            MPI_Dims_create(nnodes, ndims, dims);

            /* Checking */
            for (i = 0; i < ndims - 1; i++)
                if (dims[i] / 2 > dims[i + 1]) {
                    print_err(dims, ndims);
                    ++errs;
                    break;
                }
        }
    }

    return errs;
}
