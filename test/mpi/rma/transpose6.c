/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "stdio.h"
#include "mpitest.h"

/* This does a local transpose-cum-accumulate operation. Uses
   vector and hvector datatypes (Example 3.32 from MPI 1.1
   Standard). Run on 1 process. */

#define NROWS 100
#define NCOLS 100

int main(int argc, char *argv[])
{
    int rank, nprocs, A[NROWS][NCOLS], B[NROWS][NCOLS], i, j;
    MPI_Win win;
    MPI_Datatype column, xpose;
    int errs = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (rank == 0) {
        for (i = 0; i < NROWS; i++)
            for (j = 0; j < NCOLS; j++)
                A[i][j] = B[i][j] = i * NCOLS + j;

        /* create datatype for one column */
        MPI_Type_vector(NROWS, 1, NCOLS, MPI_INT, &column);
        /* create datatype for matrix in column-major order */
        MPI_Type_hvector(NCOLS, 1, sizeof(int), column, &xpose);
        MPI_Type_commit(&xpose);

        MPI_Win_create(B, NROWS * NCOLS * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_SELF,
                       &win);

        MPI_Win_fence(0, win);

        MPI_Accumulate(A, NROWS * NCOLS, MPI_INT, 0, 0, 1, xpose, MPI_SUM, win);

        MPI_Type_free(&column);
        MPI_Type_free(&xpose);

        MPI_Win_fence(0, win);

        for (j = 0; j < NCOLS; j++) {
            for (i = 0; i < NROWS; i++) {
                if (B[j][i] != i * NCOLS + j + j * NCOLS + i) {
                    if (errs < 20) {
                        printf("Error: B[%d][%d]=%d should be %d\n", j, i,
                               B[j][i], i * NCOLS + j + j * NCOLS + i);
                    }
                    errs++;
                }
            }
        }
        if (errs >= 20) {
            printf("Total number of errors: %d\n", errs);
        }

        MPI_Win_free(&win);
    }
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
