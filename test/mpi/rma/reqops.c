/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <stdio.h>
#include <assert.h>
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
        MPI_Alloc_mem(4 * sizeof(int), MPI_INFO_NULL, &buf);
        *buf = nproc - 1;
    }
    else
        buf = NULL;

    MPI_Win_create(buf, 4 * sizeof(int) * (rank == 0), 1, MPI_INFO_NULL, MPI_COMM_WORLD, &window);

    /* PROC_NULL Communication */
    {
        MPI_Request pn_req[4];
        int val[4], res;

        MPI_Win_lock_all(0, window);

        MPI_Rget_accumulate(&val[0], 1, MPI_INT, &res, 1, MPI_INT, MPI_PROC_NULL, 0, 1, MPI_INT,
                            MPI_REPLACE, window, &pn_req[0]);
        MPI_Rget(&val[1], 1, MPI_INT, MPI_PROC_NULL, 1, 1, MPI_INT, window, &pn_req[1]);
        MPI_Rput(&val[2], 1, MPI_INT, MPI_PROC_NULL, 2, 1, MPI_INT, window, &pn_req[2]);
        MPI_Raccumulate(&val[3], 1, MPI_INT, MPI_PROC_NULL, 3, 1, MPI_INT, MPI_REPLACE, window,
                        &pn_req[3]);

        assert(pn_req[0] != MPI_REQUEST_NULL);
        assert(pn_req[1] != MPI_REQUEST_NULL);
        assert(pn_req[2] != MPI_REQUEST_NULL);
        assert(pn_req[3] != MPI_REQUEST_NULL);

        MPI_Win_unlock_all(window);

        MPI_Waitall(4, pn_req, MPI_STATUSES_IGNORE);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, window);

    /* GET-ACC: Test third-party communication, through rank 0. */
    for (i = 0; i < ITER; i++) {
        MPI_Request gacc_req;
        int val = -1, exp = -1;

        /* Processes form a ring.  Process 0 starts first, then passes a token
         * to the right.  Each process, in turn, performs third-party
         * communication via process 0's window. */
        if (rank > 0) {
            MPI_Recv(NULL, 0, MPI_BYTE, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        MPI_Rget_accumulate(&rank, 1, MPI_INT, &val, 1, MPI_INT, 0, 0, 1, MPI_INT, MPI_REPLACE,
                            window, &gacc_req);
        assert(gacc_req != MPI_REQUEST_NULL);
        MPI_Wait(&gacc_req, MPI_STATUS_IGNORE);

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

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0)
        *buf = nproc - 1;
    MPI_Win_sync(window);

    /* GET+PUT: Test third-party communication, through rank 0. */
    for (i = 0; i < ITER; i++) {
        MPI_Request req;
        int val = -1, exp = -1;

        /* Processes form a ring.  Process 0 starts first, then passes a token
         * to the right.  Each process, in turn, performs third-party
         * communication via process 0's window. */
        if (rank > 0) {
            MPI_Recv(NULL, 0, MPI_BYTE, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        MPI_Rget(&val, 1, MPI_INT, 0, 0, 1, MPI_INT, window, &req);
        assert(req != MPI_REQUEST_NULL);
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        /* Use flush to guarantee remote completion */
        MPI_Put(&rank, 1, MPI_INT, 0, 0, 1, MPI_INT, window);
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

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0)
        *buf = nproc - 1;
    MPI_Win_sync(window);

    /* GET+ACC: Test third-party communication, through rank 0. */
    for (i = 0; i < ITER; i++) {
        MPI_Request req;
        int val = -1, exp = -1;

        /* Processes form a ring.  Process 0 starts first, then passes a token
         * to the right.  Each process, in turn, performs third-party
         * communication via process 0's window. */
        if (rank > 0) {
            MPI_Recv(NULL, 0, MPI_BYTE, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        MPI_Rget(&val, 1, MPI_INT, 0, 0, 1, MPI_INT, window, &req);
        assert(req != MPI_REQUEST_NULL);
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        /* Use flush to guarantee remote completion */
        MPI_Accumulate(&rank, 1, MPI_INT, 0, 0, 1, MPI_INT, MPI_REPLACE, window);
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

    MPI_Barrier(MPI_COMM_WORLD);

    /* Wait inside of an epoch */
    {
        MPI_Request pn_req[4];
        int val[4], res;
        const int target = 0;

        MPI_Win_lock_all(0, window);

        MPI_Rget_accumulate(&val[0], 1, MPI_INT, &res, 1, MPI_INT, target, 0, 1, MPI_INT,
                            MPI_REPLACE, window, &pn_req[0]);
        MPI_Rget(&val[1], 1, MPI_INT, target, 1, 1, MPI_INT, window, &pn_req[1]);
        MPI_Rput(&val[2], 1, MPI_INT, target, 2, 1, MPI_INT, window, &pn_req[2]);
        MPI_Raccumulate(&val[3], 1, MPI_INT, target, 3, 1, MPI_INT, MPI_REPLACE, window,
                        &pn_req[3]);

        assert(pn_req[0] != MPI_REQUEST_NULL);
        assert(pn_req[1] != MPI_REQUEST_NULL);
        assert(pn_req[2] != MPI_REQUEST_NULL);
        assert(pn_req[3] != MPI_REQUEST_NULL);

        MPI_Waitall(4, pn_req, MPI_STATUSES_IGNORE);

        MPI_Win_unlock_all(window);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Wait outside of an epoch */
    {
        MPI_Request pn_req[4];
        int val[4], res;
        const int target = 0;

        MPI_Win_lock_all(0, window);

        MPI_Rget_accumulate(&val[0], 1, MPI_INT, &res, 1, MPI_INT, target, 0, 1, MPI_INT,
                            MPI_REPLACE, window, &pn_req[0]);
        MPI_Rget(&val[1], 1, MPI_INT, target, 1, 1, MPI_INT, window, &pn_req[1]);
        MPI_Rput(&val[2], 1, MPI_INT, target, 2, 1, MPI_INT, window, &pn_req[2]);
        MPI_Raccumulate(&val[3], 1, MPI_INT, target, 3, 1, MPI_INT, MPI_REPLACE, window,
                        &pn_req[3]);

        assert(pn_req[0] != MPI_REQUEST_NULL);
        assert(pn_req[1] != MPI_REQUEST_NULL);
        assert(pn_req[2] != MPI_REQUEST_NULL);
        assert(pn_req[3] != MPI_REQUEST_NULL);

        MPI_Win_unlock_all(window);

        MPI_Waitall(4, pn_req, MPI_STATUSES_IGNORE);
    }

    /* Wait in a different epoch */
    {
        MPI_Request pn_req[4];
        int val[4], res;
        const int target = 0;

        MPI_Win_lock_all(0, window);

        MPI_Rget_accumulate(&val[0], 1, MPI_INT, &res, 1, MPI_INT, target, 0, 1, MPI_INT,
                            MPI_REPLACE, window, &pn_req[0]);
        MPI_Rget(&val[1], 1, MPI_INT, target, 1, 1, MPI_INT, window, &pn_req[1]);
        MPI_Rput(&val[2], 1, MPI_INT, target, 2, 1, MPI_INT, window, &pn_req[2]);
        MPI_Raccumulate(&val[3], 1, MPI_INT, target, 3, 1, MPI_INT, MPI_REPLACE, window,
                        &pn_req[3]);

        assert(pn_req[0] != MPI_REQUEST_NULL);
        assert(pn_req[1] != MPI_REQUEST_NULL);
        assert(pn_req[2] != MPI_REQUEST_NULL);
        assert(pn_req[3] != MPI_REQUEST_NULL);

        MPI_Win_unlock_all(window);

        MPI_Win_lock_all(0, window);
        MPI_Waitall(4, pn_req, MPI_STATUSES_IGNORE);
        MPI_Win_unlock_all(window);
    }

    /* Wait in a fence epoch */
    {
        MPI_Request pn_req[4];
        int val[4], res;
        const int target = 0;

        MPI_Win_lock_all(0, window);

        MPI_Rget_accumulate(&val[0], 1, MPI_INT, &res, 1, MPI_INT, target, 0, 1, MPI_INT,
                            MPI_REPLACE, window, &pn_req[0]);
        MPI_Rget(&val[1], 1, MPI_INT, target, 1, 1, MPI_INT, window, &pn_req[1]);
        MPI_Rput(&val[2], 1, MPI_INT, target, 2, 1, MPI_INT, window, &pn_req[2]);
        MPI_Raccumulate(&val[3], 1, MPI_INT, target, 3, 1, MPI_INT, MPI_REPLACE, window,
                        &pn_req[3]);

        assert(pn_req[0] != MPI_REQUEST_NULL);
        assert(pn_req[1] != MPI_REQUEST_NULL);
        assert(pn_req[2] != MPI_REQUEST_NULL);
        assert(pn_req[3] != MPI_REQUEST_NULL);

        MPI_Win_unlock_all(window);

        MPI_Win_fence(0, window);
        MPI_Waitall(4, pn_req, MPI_STATUSES_IGNORE);
        MPI_Win_fence(0, window);
    }

    MPI_Win_free(&window);
    if (buf)
        MPI_Free_mem(buf);

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
