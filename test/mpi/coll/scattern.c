/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run coll_scattern
int run(const char *arg);
#endif

/* This example sends a vector and receives individual elements */

int run(const char *arg)
{
    MPI_Datatype vec;
    double *vecin, *vecout, ivalue;
    int root, i, n, stride, errs = 0;
    int rank, size;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    n = 12;
    stride = 10;
    vecin = (double *) malloc(n * stride * size * sizeof(double));
    vecout = (double *) malloc(n * sizeof(double));

    MPI_Type_vector(n, 1, stride, MPI_DOUBLE, &vec);
    MPI_Type_commit(&vec);

    for (i = 0; i < n * stride * size; i++)
        vecin[i] = (double) i;
    for (root = 0; root < size; root++) {
        for (i = 0; i < n; i++)
            vecout[i] = -1.0;
        MPI_Scatter(vecin, 1, vec, vecout, n, MPI_DOUBLE, root, MPI_COMM_WORLD);
        ivalue = rank * ((n - 1) * stride + 1);
        for (i = 0; i < n; i++) {
            if (vecout[i] != ivalue) {
                printf("Expected %f but found %f\n", ivalue, vecout[i]);
                errs++;
            }
            ivalue += stride;
        }
    }
    free(vecin);
    free(vecout);
    MPI_Type_free(&vec);

    return errs;
}
