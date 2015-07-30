/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/** Contended RMA put test -- James Dinan <dinan@mcs.anl.gov>
  *
  * Each process issues COUNT put operations to non-overlapping locations on
  * every other processs.
  */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mpi.h"
#include "mpitest.h"

#define MAXELEMS      6400
#define COUNT         1000

static int me, nproc;
static const int verbose = 0;

int test_put(void);

int test_put(void)
{
    MPI_Win dst_win;
    double *dst_buf;
    double src_buf[MAXELEMS];
    int i, j;
    int errs = 0;

    MPI_Alloc_mem(sizeof(double) * nproc * MAXELEMS, MPI_INFO_NULL, &dst_buf);
    MPI_Win_create(dst_buf, sizeof(double) * nproc * MAXELEMS, 1, MPI_INFO_NULL,
                   MPI_COMM_WORLD, &dst_win);

    for (i = 0; i < MAXELEMS; i++)
        src_buf[i] = me + 1.0;

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, me, 0, dst_win);

    for (i = 0; i < nproc * MAXELEMS; i++)
        dst_buf[i] = 0.0;

    MPI_Win_unlock(me, dst_win);

    MPI_Barrier(MPI_COMM_WORLD);

    for (i = 0; i < nproc; i++) {
        /* int target = (me + i) % nproc; */
        int target = i;
        for (j = 0; j < COUNT; j++) {
            if (verbose)
                printf("%2d -> %2d [%2d]\n", me, target, j);
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, dst_win);
            MPI_Put(&src_buf[j], sizeof(double), MPI_BYTE, target,
                    (me * MAXELEMS + j) * sizeof(double), sizeof(double), MPI_BYTE, dst_win);
            MPI_Win_unlock(target, dst_win);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Check that the correct data was returned.  This assumes that the
     * systems have the same data representations */
    for (i = 0; i < nproc; i++) {
        for (j = 0; j < COUNT; j++) {
            if (dst_buf[i * MAXELEMS + j] != 1.0 + i) {
                errs++;
                printf("dst_buf[%d] = %e, expected %e\n",
                       i * MAXELEMS + j, dst_buf[i * MAXELEMS + j], 1.0 + i);
            }
        }
    }

    MPI_Win_free(&dst_win);
    MPI_Free_mem(dst_buf);

    return errs;
}


int main(int argc, char *argv[])
{
    int errs = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);

    assert(COUNT <= MAXELEMS);

    if (me == 0 && verbose) {
        printf("Test starting on %d processes\n", nproc);
        fflush(stdout);
    }

    errs = test_put();

    MPI_Barrier(MPI_COMM_WORLD);

    MTest_Finalize(errs);
    MPI_Finalize();
    return MTestReturnValue(errs);
}
