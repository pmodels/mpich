/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <stdio.h>
#include "mpitest.h"

#define ITER 100

int main(int argc, char *argv[])
{
    int rank, nproc, i;
    int errors = 0, all_errors = 0;
    int *buf;
    MPI_Win window;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    if (nproc < 2) {
        if (rank == 0)
            printf("Error: must be run with two or more processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /** Create using MPI_Win_create() **/

    if (rank == 0) {
        MPI_Alloc_mem(sizeof(int), MPI_INFO_NULL, &buf);
        *buf = nproc - 1;
    }
    else
        buf = NULL;

    MPI_Win_create(buf, sizeof(int) * (rank == 0), 1, MPI_INFO_NULL, MPI_COMM_WORLD, &window);

    /* Test flush of an empty epoch */
    MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, window);
    MPI_Win_flush_all(window);
    MPI_Win_unlock(0, window);

    MPI_Barrier(MPI_COMM_WORLD);

    /* Test third-party communication, through rank 0. */
    MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, window);

    for (i = 0; i < ITER; i++) {
        int val = -1, exp = -1;

        /* Processes form a ring.  Process 0 starts first, then passes a token
         * to the right.  Each process, in turn, performs third-party
         * communication via process 0's window. */
        if (rank > 0) {
            MPI_Recv(NULL, 0, MPI_BYTE, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        MPI_Get_accumulate(&rank, 1, MPI_INT, &val, 1, MPI_INT, 0, 0, 1, MPI_INT, MPI_REPLACE,
                           window);
        MPI_Win_flush(0, window);

        exp = (rank + nproc - 1) % nproc;

        if (val != exp) {
            printf("%d - Got %d, expected %d\n", rank, val, exp);
            errors++;
        }

        if (rank < nproc - 1) {
            MPI_Send(NULL, 0, MPI_BYTE, rank + 1, 0, MPI_COMM_WORLD);
        }

        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Win_unlock(0, window);

    MPI_Win_free(&window);
    if (buf)
        MPI_Free_mem(buf);

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
