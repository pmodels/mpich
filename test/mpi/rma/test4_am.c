/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"
#include "mpitest.h"
#include "squelch.h"

/* tests passive target RMA on 2 processes. tests the lock-single_op-unlock
   optimization. */

/* same as test4.c but uses alloc_mem */

#define SIZE1 100
#define SIZE2 200

int main(int argc, char *argv[])
{
    int rank, nprocs, *A, *B, i;
    MPI_Comm CommDeuce;
    MPI_Win win;
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
        i = MPI_Alloc_mem(SIZE2 * sizeof(int), MPI_INFO_NULL, &A);
        if (i) {
            printf("Can't allocate memory in test program\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        i = MPI_Alloc_mem(SIZE2 * sizeof(int), MPI_INFO_NULL, &B);
        if (i) {
            printf("Can't allocate memory in test program\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        if (rank == 0) {
            for (i = 0; i < SIZE2; i++)
                A[i] = B[i] = i;
            MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, CommDeuce, &win);

            for (i = 0; i < SIZE1; i++) {
                MPI_Win_lock(MPI_LOCK_SHARED, 1, 0, win);
                MPI_Put(A + i, 1, MPI_INT, 1, i, 1, MPI_INT, win);
                MPI_Win_unlock(1, win);
            }

            for (i = 0; i < SIZE1; i++) {
                MPI_Win_lock(MPI_LOCK_SHARED, 1, 0, win);
                MPI_Get(B + i, 1, MPI_INT, 1, SIZE1 + i, 1, MPI_INT, win);
                MPI_Win_unlock(1, win);
            }

            MPI_Win_free(&win);

            for (i = 0; i < SIZE1; i++)
                if (B[i] != (-4) * (i + SIZE1)) {
                    SQUELCH(printf
                            ("Get Error: B[%d] is %d, should be %d\n", i, B[i],
                             (-4) * (i + SIZE1)););
                    errs++;
                }
        }
        else {  /* rank=1 */
            for (i = 0; i < SIZE2; i++)
                B[i] = (-4) * i;
            MPI_Win_create(B, SIZE2 * sizeof(int), sizeof(int), MPI_INFO_NULL, CommDeuce, &win);

            MPI_Win_free(&win);

            for (i = 0; i < SIZE1; i++) {
                if (B[i] != i) {
                    SQUELCH(printf("Put Error: B[%d] is %d, should be %d\n", i, B[i], i););
                    errs++;
                }
            }
        }

        MPI_Free_mem(A);
        MPI_Free_mem(B);
    }
    MPI_Comm_free(&CommDeuce);
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
