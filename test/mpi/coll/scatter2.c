/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "mpitest.h"
#include <stdlib.h>
#include <stdio.h>

/* This example sends a vector and receives individual elements, but the
   root process does not receive any data */

int main(int argc, char **argv)
{
    MPI_Datatype vec;
    double *vecin, *vecout, ivalue;
    int root, i, n, stride, err = 0;
    int rank, size;
    MPI_Aint vextent;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    n = 12;
    stride = 10;
    vecin = (double *) malloc(n * stride * size * sizeof(double));
    vecout = (double *) malloc(n * sizeof(double));

    MPI_Type_vector(n, 1, stride, MPI_DOUBLE, &vec);
    MPI_Type_commit(&vec);
    MPI_Type_extent(vec, &vextent);
    if (vextent != ((n - 1) * (MPI_Aint) stride + 1) * sizeof(double)) {
        err++;
        printf("Vector extent is %ld, should be %ld\n",
               (long) vextent, (long) (((n - 1) * stride + 1) * sizeof(double)));
    }
    /* Note that the exted of type vector is from the first to the
     * last element, not n*stride.
     * E.g., with n=1, the extent is a single double */

    for (i = 0; i < n * stride * size; i++)
        vecin[i] = (double) i;
    for (root = 0; root < size; root++) {
        for (i = 0; i < n; i++)
            vecout[i] = -1.0;
        if (rank == root) {
            MPI_Scatter(vecin, 1, vec, MPI_IN_PLACE, -1, MPI_DATATYPE_NULL, root, MPI_COMM_WORLD);
        }
        else {
            MPI_Scatter(NULL, -1, MPI_DATATYPE_NULL, vecout, n, MPI_DOUBLE, root, MPI_COMM_WORLD);
            ivalue = rank * ((n - 1) * stride + 1);
            for (i = 0; i < n; i++) {
                if (vecout[i] != ivalue) {
                    printf("[%d] Expected %f but found %f for vecout[%d]\n",
                           rank, ivalue, vecout[i], i);
                    err++;
                }
                ivalue += stride;
            }
        }
    }

    free(vecin);
    free(vecout);
    MTest_Finalize(err);
    MPI_Type_free(&vec);
    MPI_Finalize();
    return 0;
}
