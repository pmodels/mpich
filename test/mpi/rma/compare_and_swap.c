/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>
#include "squelch.h"

#ifdef MULTI_TESTS
#define run rma_compare_and_swap
int run(const char *arg);
#endif

#define ITER 100

int run(const char *arg)
{
    int i, rank, nproc;
    int errors = 0;
    int *val_ptr;
    MPI_Win win;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    val_ptr = malloc(sizeof(int));

    *val_ptr = 0;

    MPI_Win_create(val_ptr, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    /* Test self communication */

    for (i = 0; i < ITER; i++) {
        int next = i + 1, result = -1;
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
        MPI_Compare_and_swap(&next, &i, &result, MPI_INT, rank, 0, win);
        MPI_Win_unlock(rank, win);
        if (result != i) {
            SQUELCH(printf("%d->%d -- Error: next=%d compare=%d result=%d val=%d\n", rank,
                           rank, next, i, result, *val_ptr););
            errors++;
        }
    }

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    *val_ptr = 0;
    MPI_Win_unlock(rank, win);

    MPI_Barrier(MPI_COMM_WORLD);

    /* Test neighbor communication */

    for (i = 0; i < ITER; i++) {
        int next = i + 1, result = -1;
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, (rank + 1) % nproc, 0, win);
        MPI_Compare_and_swap(&next, &i, &result, MPI_INT, (rank + 1) % nproc, 0, win);
        MPI_Win_unlock((rank + 1) % nproc, win);
        if (result != i) {
            SQUELCH(printf("%d->%d -- Error: next=%d compare=%d result=%d val=%d\n", rank,
                           (rank + 1) % nproc, next, i, result, *val_ptr););
            errors++;
        }
    }

    fflush(NULL);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    *val_ptr = 0;
    MPI_Win_unlock(rank, win);
    MPI_Barrier(MPI_COMM_WORLD);


    /* Test contention */

    if (rank != 0) {
        for (i = 0; i < ITER; i++) {
            int next = i + 1, result = -1;
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
            MPI_Compare_and_swap(&next, &i, &result, MPI_INT, 0, 0, win);
            MPI_Win_unlock(0, win);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0 && nproc > 1) {
        if (*val_ptr != ITER) {
            SQUELCH(printf("%d - Error: expected=%d val=%d\n", rank, ITER, *val_ptr););
            errors++;
        }
    }

    MPI_Win_free(&win);

    free(val_ptr);

    return errors;
}
