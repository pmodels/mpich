/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include <string.h>

/*
static char MTEST_Descrip[] = "Mix synchronization types";
*/

void delay(double time);
void delay(double time)
{
    double t1;
    t1 = MPI_Wtime();
    while (MPI_Wtime() - t1 < time);
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int crank, csize, source, dest, loop;
    int *buf0, *buf1, *buf2, *inbuf2, count0, count1, count2, count, i;
    MPI_Comm comm;
    MPI_Win win;
    int *winbuf;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;

    count0 = 1000;
    count1 = 1;
    count2 = 100;

    count = count0 + count1 + count2 + 2;

    /* Allocate and initialize the local buffers */
    buf0 = (int *) malloc(count0 * sizeof(int));
    buf1 = (int *) malloc(count1 * sizeof(int));
    buf2 = (int *) malloc(count2 * sizeof(int));
    inbuf2 = (int *) malloc(count2 * sizeof(int));
    if (!buf0 || !buf1 || !buf2 || !inbuf2) {
        fprintf(stderr, "Unable to allocated buf0-2\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (i = 0; i < count0; i++)
        buf0[i] = i;
    for (i = 0; i < count1; i++)
        buf1[i] = i + count0;
    for (i = 0; i < count2; i++)
        buf2[i] = i + count0 + count1;

    /* Allocate the window buffer and create the memory window. */
    MPI_Alloc_mem(count * sizeof(int), MPI_INFO_NULL, &winbuf);
    if (!winbuf) {
        fprintf(stderr, "Unable to allocate %d words\n", count);
        MPI_Abort(MPI_COMM_WORLD, 0);
    }
    MPI_Win_create(winbuf, count * sizeof(int), sizeof(int), MPI_INFO_NULL, comm, &win);

    MPI_Comm_size(comm, &csize);
    MPI_Comm_rank(comm, &crank);
    dest = 0;
    source = 1;

    for (loop = 0; loop < 2; loop++) {
        /* Perform several communication operations, mixing synchronization
         * types.  Use multiple communication to avoid the single-operation
         * optimization that may be present. */
        MTestPrintfMsg(3, "Beginning loop %d of mixed sync put operations\n", loop);
        MPI_Barrier(comm);
        if (crank == source) {
            MTestPrintfMsg(3, "About to perform exclusive lock\n");
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, dest, 0, win);
            MPI_Put(buf0, count0, MPI_INT, dest, 0, count0, MPI_INT, win);
            MPI_Put(buf1, count1, MPI_INT, dest, count0, count1, MPI_INT, win);
            MPI_Put(buf2, count2, MPI_INT, dest, count0 + count1, count2, MPI_INT, win);
            MPI_Win_unlock(dest, win);
            MTestPrintfMsg(3, "Released exclusive lock\n");
        }
        else if (crank == dest) {
            /* Just delay a bit */
            delay(0.0001);
        }

        /* The synchronization mode can only be changed when the process
         * memory and public copy are guaranteed to have the same values
         * (See 11.7, Semantics and Correctness). This barrier ensures that
         * the lock/unlock completes before the fence call.  */
        MPI_Barrier(comm);

        MTestPrintfMsg(3, "About to start fence\n");
        MPI_Win_fence(0, win);
        if (crank == source) {
            MPI_Put(buf0, count0, MPI_INT, dest, 1, count0, MPI_INT, win);
            MPI_Put(buf1, count1, MPI_INT, dest, 1 + count0, count1, MPI_INT, win);
            MPI_Put(buf2, count2, MPI_INT, dest, 1 + count0 + count1, count2, MPI_INT, win);
        }
        MPI_Win_fence(0, win);
        MTestPrintfMsg(3, "Finished with fence sync\n");

        /* Check results */
        if (crank == dest) {
            for (i = 0; i < count0 + count1 + count2; i++) {
                if (winbuf[1 + i] != i) {
                    errs++;
                    if (errs < 10) {
                        fprintf(stderr, "winbuf[%d] = %d, expected %d\n", 1 + i, winbuf[1 + i], i);
                        fflush(stderr);
                    }
                }
            }
        }

        /* End of test loop */
    }

    /* Use mixed put and accumulate */
    for (loop = 0; loop < 2; loop++) {
        /* Perform several communication operations, mixing synchronization
         * types.  Use multiple communication to avoid the single-operation
         * optimization that may be present. */
        MTestPrintfMsg(3, "Begining loop %d of mixed sync put/acc operations\n", loop);
        memset(winbuf, 0, count * sizeof(int));
        MPI_Barrier(comm);
        if (crank == source) {
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, dest, 0, win);
            MPI_Accumulate(buf0, count0, MPI_INT, dest, 0, count0, MPI_INT, MPI_SUM, win);
            MPI_Accumulate(buf1, count1, MPI_INT, dest, count0, count1, MPI_INT, MPI_SUM, win);
            MPI_Put(buf2, count2, MPI_INT, dest, count0 + count1, count2, MPI_INT, win);
            MPI_Win_unlock(dest, win);
        }
        else if (crank == dest) {
            /* Just delay a bit */
            delay(0.0001);
        }
        /* See above - the fence should not start until the unlock completes */
        MPI_Barrier(comm);
        MPI_Win_fence(0, win);
        if (crank == source) {
            MPI_Accumulate(buf0, count0, MPI_INT, dest, 1, count0, MPI_INT, MPI_REPLACE, win);
            MPI_Accumulate(buf1, count1, MPI_INT, dest, 1 + count0, count1,
                           MPI_INT, MPI_REPLACE, win);
            MPI_Put(buf2, count2, MPI_INT, dest, 1 + count0 + count1, count2, MPI_INT, win);
        }
        MPI_Win_fence(0, win);

        /* Check results */
        if (crank == dest) {
            for (i = 0; i < count0 + count1 + count2; i++) {
                if (winbuf[1 + i] != i) {
                    errs++;
                    if (errs < 10) {
                        fprintf(stderr, "winbuf[%d] = %d, expected %d\n", 1 + i, winbuf[1 + i], i);
                        fflush(stderr);
                    }
                }
            }
        }

        /* End of test loop */
    }

    /* Use mixed accumulate and get */
    for (loop = 0; loop < 2; loop++) {
        /* Perform several communication operations, mixing synchronization
         * types.  Use multiple communication to avoid the single-operation
         * optimization that may be present. */
        MTestPrintfMsg(3, "Begining loop %d of mixed sync put/get/acc operations\n", loop);
        MPI_Barrier(comm);
        if (crank == source) {
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, dest, 0, win);
            MPI_Accumulate(buf0, count0, MPI_INT, dest, 0, count0, MPI_INT, MPI_REPLACE, win);
            MPI_Put(buf1, count1, MPI_INT, dest, count0, count1, MPI_INT, win);
            MPI_Get(inbuf2, count2, MPI_INT, dest, count0 + count1, count2, MPI_INT, win);
            MPI_Win_unlock(dest, win);
        }
        else if (crank == dest) {
            /* Just delay a bit */
            delay(0.0001);
        }
        /* See above - the fence should not start until the unlock completes */
        MPI_Barrier(comm);
        MPI_Win_fence(0, win);
        if (crank == source) {
            MPI_Accumulate(buf0, count0, MPI_INT, dest, 1, count0, MPI_INT, MPI_REPLACE, win);
            MPI_Put(buf1, count1, MPI_INT, dest, 1 + count0, count1, MPI_INT, win);
            MPI_Get(inbuf2, count2, MPI_INT, dest, 1 + count0 + count1, count2, MPI_INT, win);
        }
        MPI_Win_fence(0, win);

        /* Check results */
        if (crank == dest) {
            /* Do the put/accumulate parts */
            for (i = 0; i < count0 + count1; i++) {
                if (winbuf[1 + i] != i) {
                    errs++;
                    if (errs < 10) {
                        fprintf(stderr, "winbuf[%d] = %d, expected %d\n", 1 + i, winbuf[1 + i], i);
                        fflush(stderr);
                    }
                }
            }
        }

        /* End of test loop */
    }

    MTestPrintfMsg(3, "Freeing the window\n");
    MPI_Barrier(comm);
    MPI_Win_free(&win);
    MPI_Free_mem(winbuf);
    free(buf0);
    free(buf1);
    free(buf2);
    free(inbuf2);

    MTest_Finalize(errs);

    MPI_Finalize();
    return 0;
}
