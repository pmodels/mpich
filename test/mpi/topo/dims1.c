/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run topo_dims1
int run(const char *arg);
#endif

static int prodof(int ndims, const int dims[]);
static int increasing(int ndims, const int dims[]);

static int prodof(int ndims, const int dims[])
{
    int i, prod = 1;
    for (i = 0; i < ndims; i++)
        prod *= dims[i];
    return prod;
}

static int increasing(int ndims, const int dims[])
{
    int i, err = 0;
    for (i = 1; i < ndims; i++) {
        if (dims[i] > dims[i - 1]) {
            printf("%d = dims[%d] > dims[%d] = %d\n", dims[i], i, i - 1, dims[i - 1]);
            err = 1;
        }
    }
    return err;
}

int run(const char *arg)
{
    int errs = 0;
    int dims[4], nnodes, ndims;


    /* Test multiple dims create values.  For each, make sure that the
     * product of dims is the number of input nodes */
    nnodes = 2 * 3 * 5 * 7 * 11;
    ndims = 2;
    dims[0] = dims[1] = 0;
    MPI_Dims_create(nnodes, ndims, dims);
    if (prodof(ndims, dims) != nnodes) {
        errs++;
        printf("dims create returned the wrong decomposition for %d in %d dims\n", nnodes, ndims);
    }
    if (increasing(ndims, dims)) {
        errs++;
        printf
            ("dims create returned a decomposition with increasing dimensions (see MPI-1 standard section 6.5)\n");
        printf("dims create returned the wrong decomposition for %d in %d dims\n", nnodes, ndims);
    }

    /* Test multiple dims create values.  For each, make sure that the
     * product of dims is the number of input nodes */
    nnodes = 2 * 7;
    ndims = 2;
    dims[0] = dims[1] = 0;
    MPI_Dims_create(nnodes, ndims, dims);
    if (prodof(ndims, dims) != nnodes) {
        errs++;
        printf("dims create returned the wrong decomposition for %d in %d dims\n", nnodes, ndims);
    }
    if (increasing(ndims, dims)) {
        errs++;
        printf
            ("dims create returned a decomposition with increasing dimensions (see MPI-1 standard section 6.5)\n");
        printf("dims create returned the wrong decomposition for %d in %d dims\n", nnodes, ndims);
    }

    nnodes = 2 * 2 * 3 * 3 * 5 * 7 * 11;
    ndims = 2;
    dims[0] = dims[1] = 0;
    MPI_Dims_create(nnodes, ndims, dims);
    if (prodof(ndims, dims) != nnodes) {
        errs++;
        printf("dims create returned the wrong decomposition for %d in %d dims\n", nnodes, ndims);
    }
    if (increasing(ndims, dims)) {
        errs++;
        printf
            ("dims create returned a decomposition with increasing dimensions (see MPI-1 standard section 6.5)\n");
        printf("dims create returned the wrong decomposition for %d in %d dims\n", nnodes, ndims);
    }

    nnodes = 11;
    ndims = 2;
    dims[0] = dims[1] = 0;
    MPI_Dims_create(nnodes, ndims, dims);
    if (prodof(ndims, dims) != nnodes) {
        errs++;
        printf("dims create returned the wrong decomposition for %d in %d dims\n", nnodes, ndims);
    }
    if (increasing(ndims, dims)) {
        errs++;
        printf
            ("dims create returned a decomposition with increasing dimensions (see MPI-1 standard section 6.5)\n");
        printf("dims create returned the wrong decomposition for %d in %d dims\n", nnodes, ndims);
    }

    nnodes = 5 * 7 * 11;
    ndims = 4;
    dims[0] = dims[1] = dims[2] = dims[3] = 0;
    MPI_Dims_create(nnodes, ndims, dims);
    if (prodof(ndims, dims) != nnodes) {
        errs++;
        printf("dims create returned the wrong decomposition for %d in %d dims\n", nnodes, ndims);
    }
    if (increasing(ndims, dims)) {
        errs++;
        printf
            ("dims create returned a decomposition with increasing dimensions (see MPI-1 standard section 6.5)\n");
        printf("dims create returned the wrong decomposition for %d in %d dims\n", nnodes, ndims);
    }

    nnodes = 64;
    ndims = 4;
    dims[0] = dims[1] = dims[2] = dims[3] = 0;
    MPI_Dims_create(nnodes, ndims, dims);
    if (prodof(ndims, dims) != nnodes) {
        errs++;
        printf("dims create returned the wrong decomposition for %d in %d dims\n", nnodes, ndims);
    }
    if (increasing(ndims, dims)) {
        errs++;
        printf
            ("dims create returned a decomposition with increasing dimensions (see MPI-1 standard section 6.5)\n");
        printf("dims create returned the wrong decomposition for %d in %d dims\n", nnodes, ndims);
    }

    return errs;
}
