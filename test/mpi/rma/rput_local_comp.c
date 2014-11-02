/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <stdio.h>
#include <assert.h>
#include "mpitest.h"

#define ITER 100
#define MAX_SIZE 65536

int main(int argc, char *argv[])
{
    int rank, nproc, i;
    int errors = 0, all_errors = 0;
    int *buf = NULL, *winbuf = NULL;
    MPI_Win window;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    if (nproc < 2) {
        if (rank == 0)
            printf("Error: must be run with two or more processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Alloc_mem(MAX_SIZE * sizeof(int), MPI_INFO_NULL, &buf);
    MPI_Alloc_mem(MAX_SIZE * sizeof(int), MPI_INFO_NULL, &winbuf);
    MPI_Win_create(winbuf, MAX_SIZE * sizeof(int), sizeof(int), MPI_INFO_NULL,
                   MPI_COMM_WORLD, &window);

    MPI_Win_lock_all(0, window);

    /* Test Rput local completion with small data.
     * Small data is always copied to header packet as immediate data. */
    if (rank == 1) {
        for (i = 0; i < ITER; i++) {
            MPI_Request put_req;
            int val = -1;

            buf[0] = rank;
            MPI_Rput(&buf[0], 1, MPI_INT, 0, 0, 1, MPI_INT, window, &put_req);
            MPI_Wait(&put_req, MPI_STATUS_IGNORE);

            /* reset local buffer to check local completion */
            buf[0] = 0;
            MPI_Win_flush(0, window);

            MPI_Get(&val, 1, MPI_INT, 0, 0, 1, MPI_INT, window);
            MPI_Win_flush(0, window);

            if (val != rank) {
                printf("%d - Got %d in small Rput test, expected %d\n", rank, val, rank);
                errors++;
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Test Rput local completion with large data .
     * Large data is not suitable for 1-copy optimization, and always sent out
     * from user buffer. */
    if (rank == 1) {
        for (i = 0; i < ITER; i++) {
            MPI_Request put_req;
            int val0 = -1, val1 = -1, val2 = -1;
            int j;

            /* initialize data */
            for (j = 0; j < MAX_SIZE; j++) {
                buf[j] = rank + j + i;
            }

            MPI_Rput(buf, MAX_SIZE, MPI_INT, 0, 0, MAX_SIZE, MPI_INT, window, &put_req);
            MPI_Wait(&put_req, MPI_STATUS_IGNORE);

            /* reset local buffer to check local completion */
            buf[0] = 0;
            buf[MAX_SIZE - 1] = 0;
            buf[MAX_SIZE / 2] = 0;
            MPI_Win_flush(0, window);

            /* get remote values which are modified in local buffer after wait */
            MPI_Get(&val0, 1, MPI_INT, 0, 0, 1, MPI_INT, window);
            MPI_Get(&val1, 1, MPI_INT, 0, MAX_SIZE - 1, 1, MPI_INT, window);
            MPI_Get(&val2, 1, MPI_INT, 0, MAX_SIZE / 2, 1, MPI_INT, window);
            MPI_Win_flush(0, window);

            if (val0 != rank + i) {
                printf("%d - Got %d in large Rput test, expected %d\n", rank, val0, rank + i);
                errors++;
            }
            if (val1 != rank + MAX_SIZE - 1 + i) {
                printf("%d - Got %d in large Rput test, expected %d\n", rank, val1,
                       rank + MAX_SIZE - 1 + i);
                errors++;
            }
            if (val2 != rank + MAX_SIZE / 2 + i) {
                printf("%d - Got %d in large Rput test, expected %d\n", rank, val2,
                       rank + MAX_SIZE / 2 + i);
                errors++;
            }
        }
    }

    MPI_Win_unlock_all(window);
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Win_free(&window);
    if (buf)
        MPI_Free_mem(buf);
    if (winbuf)
        MPI_Free_mem(winbuf);

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
