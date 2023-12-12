/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/** Contended RMA put/get test -- James Dinan <dinan@mcs.anl.gov>
  *
  * Each process issues COUNT put and get operations to non-overlapping
  * locations on every other process.
  */

#include <assert.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run rma_contention_putget
int run(const char *arg);
#endif

#define MAXELEMS      6400
#define COUNT         1000

static int me, nproc;
static const int verbose = 0;

static void test_put(void)
{
    MPI_Win dst_win;
    double *dst_buf;
    double src_buf[MAXELEMS];
    int i, j;

    MPI_Alloc_mem(sizeof(double) * nproc * MAXELEMS, MPI_INFO_NULL, &dst_buf);
    MPI_Win_create(dst_buf, sizeof(double) * nproc * MAXELEMS, 1, MPI_INFO_NULL, MPI_COMM_WORLD,
                   &dst_win);

    for (i = 0; i < MAXELEMS; i++)
        src_buf[i] = me + 1.0;

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, me, 0, dst_win);

    for (i = 0; i < nproc * MAXELEMS; i++)
        dst_buf[i] = 0.0;

    MPI_Win_unlock(me, dst_win);

    MPI_Barrier(MPI_COMM_WORLD);

    for (i = 0; i < nproc; i++) {
        int target = i;

        for (j = 0; j < COUNT; j++) {
            if (verbose)
                printf("%2d -> %2d [%2d]\n", me, target, j);
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, dst_win);
            MPI_Put(&src_buf[j], sizeof(double), MPI_BYTE, target,
                    (me * MAXELEMS + j) * sizeof(double), sizeof(double), MPI_BYTE, dst_win);
            MPI_Win_unlock(target, dst_win);
        }

        for (j = 0; j < COUNT; j++) {
            if (verbose)
                printf("%2d <- %2d [%2d]\n", me, target, j);
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, dst_win);
            MPI_Get(&src_buf[j], sizeof(double), MPI_BYTE, target,
                    (me * MAXELEMS + j) * sizeof(double), sizeof(double), MPI_BYTE, dst_win);
            MPI_Win_unlock(target, dst_win);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Win_free(&dst_win);
    MPI_Free_mem(dst_buf);
}


int run(const char *arg)
{
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);

    assert(COUNT <= MAXELEMS);

    if (me == 0 && verbose) {
        printf("Test starting on %d processes\n", nproc);
        fflush(stdout);
    }

    test_put();

    MPI_Barrier(MPI_COMM_WORLD);

    if (me == 0 && verbose) {
        printf("Test completed.\n");
        fflush(stdout);
    }

    return 0;
}
