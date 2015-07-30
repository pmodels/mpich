/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "stdio.h"
#include "mpitest.h"
#include "squelch.h"

/* tests a series of puts, gets, and accumulate on 2 processes using fence */
/* Same as test1.c but uses derived datatypes to receive data */

#define SIZE 100

int main(int argc, char *argv[])
{
    int rank, nprocs, A[SIZE], B[SIZE], i;
    MPI_Comm CommDeuce;
    MPI_Win win;
    MPI_Datatype contig_2ints;
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
            for (i = 0; i < SIZE; i++)
                A[i] = B[i] = i;
        }
        else {
            for (i = 0; i < SIZE; i++) {
                A[i] = (-3) * i;
                B[i] = (-4) * i;
            }
        }

        MPI_Type_contiguous(2, MPI_INT, &contig_2ints);
        MPI_Type_commit(&contig_2ints);

        MPI_Win_create(B, SIZE * sizeof(int), sizeof(int), MPI_INFO_NULL, CommDeuce, &win);

        MPI_Win_fence(0, win);

        if (rank == 0) {
            for (i = 0; i < SIZE - 2; i += 2)
                MPI_Put(A + i, 2, MPI_INT, 1, i, 1, contig_2ints, win);
        }
        else {
            for (i = 0; i < SIZE - 2; i += 2)
                MPI_Get(A + i, 2, MPI_INT, 0, i, 1, contig_2ints, win);

            MPI_Accumulate(A + SIZE - 2, 2, MPI_INT, 0, SIZE - 2, 1, contig_2ints, MPI_SUM, win);
        }
        MPI_Win_fence(0, win);

        if (rank == 1) {
            for (i = 0; i < SIZE - 2; i++) {
                if (A[i] != B[i]) {
                    SQUELCH(printf("Put/Get Error: A[i]=%d, B[i]=%d\n", A[i], B[i]););
                    errs++;
                }
            }
        }
        else {
            if (B[SIZE - 1] != SIZE - 1 - 3 * (SIZE - 1)) {
                SQUELCH(printf
                        ("Accumulate Error: B[SIZE-1] is %d, should be %d\n", B[SIZE - 1],
                         SIZE - 1 - 3 * (SIZE - 1)););
                errs++;
            }
        }

        MPI_Win_free(&win);
        MPI_Type_free(&contig_2ints);
    }
    MPI_Comm_free(&CommDeuce);
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
