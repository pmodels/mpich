/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include "mpitest.h"
#include "squelch.h"

#define ITER 100

#if defined (FOP_TYPE_CHAR)
#define TYPE_C   char
#define TYPE_MPI MPI_CHAR
#define TYPE_FMT "%d"
#elif defined (FOP_TYPE_SHORT)
#define TYPE_C   short
#define TYPE_MPI MPI_SHORT
#define TYPE_FMT "%d"
#elif defined (FOP_TYPE_LONG)
#define TYPE_C   long
#define TYPE_MPI MPI_LONG
#define TYPE_FMT "%ld"
#elif defined (FOP_TYPE_LONG_LONG)
#define TYPE_C   long long
#define TYPE_MPI MPI_LONG_LONG_INT
#define TYPE_FMT "%lld"
#else
#define TYPE_C   int
#define TYPE_MPI MPI_INT
#define TYPE_FMT "%d"
#endif

int main(int argc, char **argv)
{
    int i, rank, nproc;
    int errors = 0;
    TYPE_C *val_ptr;
    MPI_Win win;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    val_ptr = malloc(sizeof(TYPE_C));

    *val_ptr = 0;

    MPI_Win_create(val_ptr, sizeof(TYPE_C), sizeof(TYPE_C), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    /* Test self communication */

    MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
    for (i = 0; i < ITER; i++) {
        TYPE_C next = (TYPE_C) (i + 1), result = (TYPE_C) - 1;
        TYPE_C val = (TYPE_C) i;
        MPI_Compare_and_swap(&next, &val, &result, TYPE_MPI, rank, 0, win);
        MPI_Win_flush(rank, win);
        if (result != val) {
            SQUELCH(printf("%d->%d -- Error: next=" TYPE_FMT " compare=" TYPE_FMT
                           " result=" TYPE_FMT " val=" TYPE_FMT "\n", rank,
                           rank, next, val, result, *val_ptr););
            errors++;
        }
    }
    MPI_Win_unlock(rank, win);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    *val_ptr = 0;
    MPI_Win_unlock(rank, win);

    MPI_Barrier(MPI_COMM_WORLD);

    /* Test neighbor communication */

    MPI_Win_lock(MPI_LOCK_SHARED, (rank + 1) % nproc, 0, win);
    for (i = 0; i < ITER; i++) {
        TYPE_C next = (TYPE_C) (i + 1), result = (TYPE_C) - 1;
        TYPE_C val = (TYPE_C) i;
        MPI_Compare_and_swap(&next, &val, &result, TYPE_MPI, (rank + 1) % nproc, 0, win);
        MPI_Win_flush((rank + 1) % nproc, win);
        if (result != val) {
            SQUELCH(printf("%d->%d -- Error: next=" TYPE_FMT " compare=" TYPE_FMT
                           " result=" TYPE_FMT " val=" TYPE_FMT "\n", rank,
                           (rank + 1) % nproc, next, val, result, *val_ptr););
            errors++;
        }
    }

    fflush(NULL);
    MPI_Win_unlock((rank + 1) % nproc, win);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    *val_ptr = 0;
    MPI_Win_unlock(rank, win);
    MPI_Barrier(MPI_COMM_WORLD);


    /* Test contention */

    if (rank != 0) {
        MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, win);
        for (i = 0; i < ITER; i++) {
            TYPE_C next = (TYPE_C) (i + 1), result = (TYPE_C) - 1;
            TYPE_C val = (TYPE_C) i;
            MPI_Compare_and_swap(&next, &val, &result, TYPE_MPI, 0, 0, win);
        }
        MPI_Win_unlock(0, win);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0 && nproc > 1) {
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
        if (*val_ptr != (TYPE_C) ITER) {
            SQUELCH(printf
                    ("%d - Error: expected=" TYPE_FMT " val=" TYPE_FMT "\n", rank, (TYPE_C) ITER,
                     *val_ptr););
            errors++;
        }
        MPI_Win_unlock(rank, win);
    }

    MPI_Win_free(&win);

    free(val_ptr);
    MTest_Finalize(errors);

    return MTestReturnValue(errors);
}
