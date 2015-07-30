/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"
#include "mpitest.h"

/* This is a modified version of test4.c. Sent by Liwei Peng, Microsoft. */

/* tests passive target RMA on 3 processes. tests the lock-single_op-unlock
   optimization. */


#define SIZE1 100
#define SIZE2 200

int main(int argc, char *argv[])
{
    int rank, nprocs, A[SIZE2], B[SIZE2], i;
    MPI_Comm CommThree;
    MPI_Win win;
    int errs = 0;
    int trank = 1;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (nprocs < 3) {
        fprintf(stderr, "Run this program with 3 or more processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_split(MPI_COMM_WORLD, (rank < 3), rank, &CommThree);

    if (rank < 3) {
        if (rank == 0) {
            for (i = 0; i < SIZE2; i++) {
                A[i] = B[i] = i;
            }
        }
        else if (rank == 2) {
            for (i = 0; i < SIZE2; i++) {
                A[i] = B[i] = -1;
            }
        }
        else if (rank == 1) {
            for (i = 0; i < SIZE2; i++) {
                B[i] = (-4) * i;
            }
        }

        MPI_Win_create(B, SIZE2 * sizeof(int), sizeof(int), MPI_INFO_NULL, CommThree, &win);

        if (rank == 0) {
            for (i = 0; i < SIZE1; i++) {
                MPI_Win_lock(MPI_LOCK_EXCLUSIVE, trank, 0, win);
                MPI_Put(A + i, 1, MPI_INT, trank, i, 1, MPI_INT, win);
                /*  MPI_Put(A+i, 1, MPI_INT, trank, i, 1, MPI_INT, win);
                 * MPI_Put(A+i, 1, MPI_INT, trank, i, 1, MPI_INT, win); */
                MPI_Win_unlock(trank, win);
            }

            MPI_Win_free(&win);
        }
        else if (rank == 2) {
            for (i = 0; i < SIZE1; i++) {
                MPI_Win_lock(MPI_LOCK_EXCLUSIVE, trank, 0, win);
                MPI_Get(A + i, 1, MPI_INT, trank, SIZE1 + i, 1, MPI_INT, win);
                MPI_Win_unlock(trank, win);
            }

            MPI_Win_free(&win);

            for (i = 0; i < SIZE1; i++)
                if (A[i] != (-4) * (i + SIZE1)) {
                    printf("Get Error: A[%d] is %d, should be %d\n", i, A[i], (-4) * (i + SIZE1));
                    errs++;
                }
        }

        else if (rank == 1) {   /*target */
            MPI_Win_free(&win);

            for (i = 0; i < SIZE1; i++) {
                if (B[i] != i) {
                    printf("Put Error: B[%d] is %d, should be %d\n", i, B[i], i);
                    errs++;
                }
            }
        }
    }
    MPI_Comm_free(&CommThree);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
