/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "stdio.h"
#include "mpitest.h"

/* transposes a matrix using passive target RMA and derived
   datatypes. Uses  vector and hvector (Example 3.32 from MPI 1.1
   Standard). Run on 2 processes. */

#define NROWS 100
#define NCOLS 100

int main(int argc, char *argv[])
{
    int rank, nprocs, A[NROWS][NCOLS], i, j;
    MPI_Comm CommDeuce;
    MPI_Win win;
    MPI_Datatype column, xpose;
    int errs = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (nprocs < 2) {
        printf("Run this program with 2 or more processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_split(MPI_COMM_WORLD, (rank < 2), rank, &CommDeuce);

    if (rank < 2) {
        if (rank == 0) {
            for (i = 0; i < NROWS; i++)
                for (j = 0; j < NCOLS; j++)
                    A[i][j] = i * NCOLS + j;

            /* create datatype for one column */
            MPI_Type_vector(NROWS, 1, NCOLS, MPI_INT, &column);
            /* create datatype for matrix in column-major order */
            MPI_Type_hvector(NCOLS, 1, sizeof(int), column, &xpose);
            MPI_Type_commit(&xpose);

            MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, CommDeuce, &win);

            MPI_Win_lock(MPI_LOCK_SHARED, 1, 0, win);

            MPI_Put(A, NROWS * NCOLS, MPI_INT, 1, 0, 1, xpose, win);

            MPI_Type_free(&column);
            MPI_Type_free(&xpose);

            MPI_Win_unlock(1, win);
            MPI_Win_free(&win);
        }
        else {  /* rank=1 */
            for (i = 0; i < NROWS; i++)
                for (j = 0; j < NCOLS; j++)
                    A[i][j] = -1;
            MPI_Win_create(A, NROWS * NCOLS * sizeof(int), sizeof(int), MPI_INFO_NULL, CommDeuce,
                           &win);

            MPI_Win_free(&win);

            for (j = 0; j < NCOLS; j++)
                for (i = 0; i < NROWS; i++)
                    if (A[j][i] != i * NCOLS + j) {
                        printf("Error: A[%d][%d]=%d should be %d\n", j, i, A[j][i], i * NCOLS + j);
                        errs++;
                    }
        }
    }

    MPI_Comm_free(&CommDeuce);
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
