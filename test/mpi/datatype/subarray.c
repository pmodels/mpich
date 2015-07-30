/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <string.h>
#include "mpi.h"

#define X 64
#define Y 8
#define Z 512

double array[X][Y][Z];

int main(int argc, char *argv[])
{
    int myrank;
    MPI_Datatype subarray;
    int array_size[] = { X, Y, Z };
    int array_subsize[] = { X / 2, Y / 2, Z };
    int array_start[] = { 0, 0, 0 };
    int i, j, k;
    int errs = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    for (i = 0; i < X; ++i) {
        for (j = 0; j < Y; ++j) {
            for (k = 0; k < Z; ++k) {
                if (myrank == 0)
                    array[i][j][k] = 2.0;
                else
                    array[i][j][k] = -2.0;
            }
        }
    }

    MPI_Type_create_subarray(3, array_size, array_subsize, array_start, MPI_ORDER_C,
                             MPI_DOUBLE, &subarray);
    MPI_Type_commit(&subarray);

    if (myrank == 0)
        MPI_Send(array, 1, subarray, 1, 0, MPI_COMM_WORLD);
    else {
        MPI_Recv(array, 1, subarray, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (i = array_start[0]; i < array_subsize[0]; ++i) {
            for (j = array_start[1]; j < array_subsize[1]; ++j) {
                for (k = array_start[2]; k < array_subsize[2]; ++k) {
                    if (array[i][j][k] != 2.0)
                        ++errs;
                }
            }
        }
    }

    MPI_Type_free(&subarray);

    MPI_Allreduce(MPI_IN_PLACE, &errs, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    if (myrank == 0) {
        if (errs)
            printf("Found %d errors\n", errs);
        else
            printf(" No Errors\n");
    }
    MPI_Finalize();

    return 0;
}
