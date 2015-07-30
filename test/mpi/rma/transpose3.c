/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "stdio.h"
#include "mpitest.h"
#include "squelch.h"

/* transposes a matrix using post/start/complete/wait and derived
   datatypes. Uses  vector and hvector (Example 3.32 from MPI 1.1
   Standard). Run on 2 processes */

#define NROWS 100
#define NCOLS 100

int main(int argc, char *argv[])
{
    int rank, nprocs, i, j, destrank;
    MPI_Comm CommDeuce;
    MPI_Win win;
    MPI_Datatype column, xpose;
    MPI_Group comm_group, group;
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
        MPI_Comm_group(CommDeuce, &comm_group);

        if (rank == 0) {
            int A[NROWS][NCOLS];

            for (i = 0; i < NROWS; i++)
                for (j = 0; j < NCOLS; j++)
                    A[i][j] = i * NCOLS + j;

            /* create datatype for one column */
            MPI_Type_vector(NROWS, 1, NCOLS, MPI_INT, &column);
            /* create datatype for matrix in column-major order */
            MPI_Type_hvector(NCOLS, 1, sizeof(int), column, &xpose);
            MPI_Type_commit(&xpose);

#ifdef USE_WIN_ALLOCATE
            int *base_ptr = NULL;
            MPI_Win_allocate(0, 1, MPI_INFO_NULL, CommDeuce, &base_ptr, &win);
#else
            MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, CommDeuce, &win);
#endif

            destrank = 1;
            MPI_Group_incl(comm_group, 1, &destrank, &group);
            MPI_Win_start(group, 0, win);

            MPI_Put(A, NROWS * NCOLS, MPI_INT, 1, 0, 1, xpose, win);

            MPI_Type_free(&column);
            MPI_Type_free(&xpose);

            MPI_Win_complete(win);
        }
        else {  /* rank=1 */
            int *A;
#ifdef USE_WIN_ALLOCATE
            MPI_Win_allocate(NROWS * NCOLS * sizeof(int), sizeof(int), MPI_INFO_NULL, CommDeuce, &A,
                             &win);
#else
            MPI_Alloc_mem(NROWS * NCOLS * sizeof(int), MPI_INFO_NULL, &A);
            MPI_Win_create(A, NROWS * NCOLS * sizeof(int), sizeof(int), MPI_INFO_NULL, CommDeuce,
                           &win);
#endif
            MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
            for (i = 0; i < NROWS; i++)
                for (j = 0; j < NCOLS; j++)
                    A[i * NCOLS + j] = -1;
            MPI_Win_unlock(rank, win);

            destrank = 0;
            MPI_Group_incl(comm_group, 1, &destrank, &group);
            MPI_Win_post(group, 0, win);
            MPI_Win_wait(win);

            for (j = 0; j < NCOLS; j++) {
                for (i = 0; i < NROWS; i++) {
                    if (A[j * NROWS + i] != i * NCOLS + j) {
                        if (errs < 50) {
                            SQUELCH(printf("Error: A[%d][%d]=%d should be %d\n", j, i,
                                           A[j * NROWS + i], i * NCOLS + j););
                        }
                        errs++;
                    }
                }
            }
            if (errs >= 50) {
                printf("Total number of errors: %d\n", errs);
            }
#ifndef USE_WIN_ALLOCATE
            MPI_Free_mem(A);
#endif
        }

        MPI_Group_free(&group);
        MPI_Group_free(&comm_group);
        MPI_Win_free(&win);
    }
    MPI_Comm_free(&CommDeuce);
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
