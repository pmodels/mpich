/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <assert.h>
#include "mpitest.h"

/* This test is adapted from the reproducer by Jeff Hammond via Github issue #7890 */

/* Target expose a contiguous buffer of 2N size. Origin rput or rget using a datatype
 * of 2 noncontig block each of size N. Depends on the size of N, this triggers either
 * the nopack iov path or packed chunk read/write paths.
 */

#define T int
#define DT MPI_INT

enum op {
    PUT,
    GET,
    RPUT,
    RGET,
};

int test_rputget(enum op op_type, int N, int rank)
{
    int errs = 0;

    T *buf;
    MPI_Win win;

    MPI_Aint buf_size = (MPI_Aint) ((rank == 0) ? N * 2 * sizeof(T) : 0);
    MPI_Win_allocate(buf_size, sizeof(T), MPI_INFO_NULL, MPI_COMM_WORLD, &buf, &win);

    MPI_Win_lock_all(0, win);
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 1) {
        /* origin */
        T *origin = malloc((N * 2 + 1) * sizeof(T));
        for (int i = 0; i < N; ++i) {
            origin[i] = i;
            origin[N + 1 + i] = N + i;
        }

        const int origin_disps[2] = { 0, N + 1 };
        const int target_disps[2] = { 0, N };
        MPI_Datatype origin_type;
        MPI_Datatype target_type;
        MPI_Type_create_indexed_block(2, N, origin_disps, DT, &origin_type);
        MPI_Type_create_indexed_block(2, N, target_disps, DT, &target_type);
        MPI_Type_commit(&origin_type);
        MPI_Type_commit(&target_type);

        MPI_Request request;
        switch (op_type) {
            case PUT:
                MPI_Put(origin, 1, origin_type, 0, 0, 1, target_type, win);
                break;
            case GET:
                MPI_Get(origin, 1, origin_type, 0, 0, 1, target_type, win);
                break;
            case RPUT:
                MPI_Rput(origin, 1, origin_type, 0, 0, 1, target_type, win, &request);
                MPI_Wait(&request, MPI_STATUS_IGNORE);
                break;
            case RGET:
                MPI_Rget(origin, 1, origin_type, 0, 0, 1, target_type, win, &request);
                MPI_Wait(&request, MPI_STATUS_IGNORE);
                break;
        }
        MPI_Win_flush(0, win);

        /* TODO: verify results */

        free(origin);
        MPI_Type_free(&target_type);
        MPI_Type_free(&origin_type);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_unlock_all(win);
    MPI_Win_free(&win);

    return errs;
}

int main(int argc, char *argv[])
{
    int errs = 0;

    int rank, nproc;
    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    if (nproc != 2) {
        if (rank == 0)
            printf("Error: must be run with two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int counts[] = { 20, 2500 };
    enum op ops[] = { PUT, GET, RPUT, RGET };
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 4; j++) {
            errs += test_rputget(ops[j], counts[i], rank);
        }
    }

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
