/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>

#ifdef MULTI_TESTS
#define run rma_req_example
int run(const char *arg);
#endif

#define NSTEPS 100
#define N 1000
#define M 10

static int do_alloc_shm = 0;

/* This is example 11.21 from the MPI 3.0 spec:
 *
 * The following example shows how request-based operations can be used to
 * overlap communication with computation. Each process fetches, processes,
 * and writes the result for NSTEPS chunks of data. Instead of a single
 * buffer, M local buffers are used to allow up to M communication operations
 * to overlap with computation.
 */

/* Use a global variable to inhibit compiler optimizations in the compute
 * function. */
static double junk = 0.0;

static void compute(int step, double *data)
{
    int i;

    for (i = 0; i < N; i++)
        junk += data[i] * (double) step;
}

int run(const char *arg)
{
    int i, rank, nproc;
    int errors = 0;
    MPI_Win win;
    MPI_Request put_req[M] = { MPI_REQUEST_NULL };
    MPI_Request get_req;
    double *baseptr;
    double data[M][N];          /* M buffers of length N */
    MPI_Info win_info;

    MTestArgList *head = MTestArgListCreate_arg(arg);
    do_alloc_shm = MTestArgListGetInt_with_default(head, "alloc-shm", 0);
    MTestArgListDestroy(head);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    assert(M < NSTEPS);

    MPI_Info_create(&win_info);

    if (do_alloc_shm) {
        MPI_Info_set(win_info, "alloc_shm", "true");
    } else {
        MPI_Info_set(win_info, "alloc_shm", "false");
    }

    MPI_Win_allocate(NSTEPS * N * sizeof(double), sizeof(double), win_info,
                     MPI_COMM_WORLD, &baseptr, &win);

    MPI_Win_lock_all(0, win);

    for (i = 0; i < NSTEPS; i++) {
        int target = (rank + 1) % nproc;
        int j;

        /* Find a free put request */
        if (i < M) {
            j = i;
        } else {
            MPI_Waitany(M, put_req, &j, MPI_STATUS_IGNORE);
        }

        MPI_Rget(data[j], N, MPI_DOUBLE, target, i * N, N, MPI_DOUBLE, win, &get_req);
        MPI_Wait(&get_req, MPI_STATUS_IGNORE);

        compute(i, data[j]);
        MPI_Rput(data[j], N, MPI_DOUBLE, target, i * N, N, MPI_DOUBLE, win, &put_req[j]);
    }

    MPI_Waitall(M, put_req, MPI_STATUSES_IGNORE);
    MPI_Win_unlock_all(win);

    MPI_Win_free(&win);

    MPI_Info_free(&win_info);

    return errors;
}
