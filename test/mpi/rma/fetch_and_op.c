/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

/* MPI-3 is not yet standardized -- allow MPI-3 routines to be switched off.
 */

#if !defined(USE_STRICT_MPI) && defined(MPICH2)
#  define TEST_MPI3_ROUTINES 1
#endif

static const int SQ_LIMIT = 10;
static       int SQ_COUNT = 0;

#define SQUELCH(X)                      \
  do {                                  \
    if (SQ_COUNT < SQ_LIMIT) {          \
      SQ_COUNT++;                       \
      X                                 \
    }                                   \
  } while (0)

#define ITER 100

const int verbose = 0;

int main(int argc, char **argv) {
    int       i, j, rank, nproc;
    int       errors = 0, all_errors = 0;
    int      *val_ptr, *res_ptr;
    MPI_Win   win;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    val_ptr = malloc(sizeof(int)*nproc);
    res_ptr = malloc(sizeof(int)*nproc);

    val_ptr[0] = 0;

    MPI_Win_create(val_ptr, sizeof(int)*nproc, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

#ifdef TEST_MPI3_ROUTINES

    /* Test self communication */

    for (i = 0; i < ITER; i++) {
        int one = 1, result;
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
        MPIX_Fetch_and_op(&one, &result, MPI_INT, rank, 0, MPI_SUM, win);
        MPI_Win_unlock(rank, win);
    }

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    if (val_ptr[0] != ITER) {
        SQUELCH( printf("%d->%d -- SELF: expected %d, got %d\n", rank, rank, ITER, val_ptr[0]); );
        errors++;
    }
    val_ptr[0] = 0;
    MPI_Win_unlock(rank, win);

    MPI_Barrier(MPI_COMM_WORLD);

    /* Test neighbor communication */

    for (i = 0; i < ITER; i++) {
        int one = 1, result;
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, (rank+1)%nproc, 0, win);
        MPIX_Fetch_and_op(&one, &result, MPI_INT, (rank+1)%nproc, 0, MPI_SUM, win);
        MPI_Win_unlock((rank+1)%nproc, win);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    if (val_ptr[0] != ITER) {
        SQUELCH( printf("%d->%d -- NEIGHBOR: expected %d, got %d\n", (rank+1)%nproc, rank, ITER, val_ptr[0]); );
        errors++;
    }
    val_ptr[0] = 0;
    MPI_Win_unlock(rank, win);
    MPI_Barrier(MPI_COMM_WORLD);

    /* Test contention */

    if (rank != 0) {
        for (i = 0; i < ITER; i++) {
            int one = 1, result;
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
            MPIX_Fetch_and_op(&one, &result, MPI_INT, 0, 0, MPI_SUM, win);
            MPI_Win_unlock(0, win);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    if (rank == 0 && nproc > 1) {
        if (val_ptr[0] != ITER*(nproc-1)) {
            SQUELCH( printf("*->%d - CONTENTION: expected=%d val=%d\n", rank, ITER*(nproc-1), val_ptr[0]); );
            errors++;
        }
    }
    for (i = 0; i < nproc; i++)
        val_ptr[i] = 0;
    MPI_Win_unlock(rank, win);
    MPI_Barrier(MPI_COMM_WORLD);

    /* Test all-to-all communication */

    for (i = 0; i < nproc; i++)
        res_ptr[i] = 0;

    for (i = 1; i <= ITER; i++) {
        int j;

        MPI_Win_fence(MPI_MODE_NOPRECEDE, win);
        for (j = 0; j < nproc; j++) {
            MPIX_Fetch_and_op(&rank, &res_ptr[j], MPI_INT, j, rank, MPI_SUM, win);
        }
        MPI_Win_fence(MPI_MODE_NOSUCCEED, win);
        MPI_Barrier(MPI_COMM_WORLD);

        for (j = 0; j < nproc; j++) {
            if (res_ptr[j] != (i-1)*rank) {
                SQUELCH( printf("%d->%d -- ALL-TO-ALL: expected result %d, got %d\n", rank, j, (i-1)*rank, val_ptr[j]); );
                errors++;
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (i = 0; i < nproc; i++) {
        if (val_ptr[i] != ITER*i) {
            SQUELCH( printf("%d->%d -- ALL-TO-ALL: expected %d, got %d\n", i, rank, ITER*i, val_ptr[i]); );
            errors++;
        }
    }
    val_ptr[0] = 0;
    MPI_Win_unlock(rank, win);
    MPI_Barrier(MPI_COMM_WORLD);

#endif /* TEST_MPI3_ROUTINES */

    MPI_Win_free(&win);

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    free(val_ptr);
    free(res_ptr);
    MPI_Finalize();

    return 0;
}
