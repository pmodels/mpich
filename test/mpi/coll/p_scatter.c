/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run coll_p_scatter
int run(const char *arg);
#endif

/* This example sends a vector and receives individual elements */

int run(const char *arg)
{
    MPI_Datatype vec;
    double *vecin, *vecout, ivalue;
    int root, i, n, stride, errs = 0;
    int rank, size;

    MPI_Info info;
    MPI_Request *reqs;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Info_create(&info);

    n = 12;
    stride = 10;
    vecin = (double *) malloc(n * stride * size * sizeof(double));
    vecout = (double *) malloc(n * sizeof(double));
    reqs = (MPI_Request *) malloc(size * sizeof(MPI_Request));

    MPI_Type_vector(n, 1, stride, MPI_DOUBLE, &vec);
    MPI_Type_commit(&vec);

    for (i = 0; i < n * stride * size; i++)
        vecin[i] = (double) i;
    for (root = 0; root < size; root++) {
        MPI_Scatter_init(vecin, 1, vec, vecout, n, MPI_DOUBLE, root, MPI_COMM_WORLD, info,
                         &(reqs[root]));
    }
    for (root = 0; root < size; root++) {
        for (i = 0; i < 10; ++i) {
            for (i = 0; i < n; i++)
                vecout[i] = -1.0;

            MPI_Start(&reqs[root]);
            MPI_Wait(&reqs[root], MPI_STATUS_IGNORE);

            ivalue = rank * ((n - 1) * stride + 1);
            for (i = 0; i < n; i++) {
                if (vecout[i] != ivalue) {
                    printf("Expected %f but found %f\n", ivalue, vecout[i]);
                    errs++;
                }
                ivalue += stride;
            }
        }
        MPI_Request_free(&reqs[root]);
    }
    free(vecin);
    free(vecout);
    free(reqs);

    MPI_Info_free(&info);
    MPI_Type_free(&vec);

    return errs;
}
