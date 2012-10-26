/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <stdio.h>
#include "mpitest.h"

/* MPI-3 is not yet standardized -- allow MPI-3 routines to be switched off.
 */

#if !defined(USE_STRICT_MPI) && defined(MPICH2)
#  define TEST_MPI3_ROUTINES 1
#endif

#define ITER 100

int main( int argc, char *argv[] )
{
    int rank, nproc, i;
    int errors = 0, all_errors = 0;
    int *buf;
    MPI_Win window;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    if (nproc < 2) {
        if (rank == 0) printf("Error: must be run with two or more processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /** Create using MPI_Win_create() **/

#ifdef TEST_MPI3_ROUTINES

    if (rank == 0) {
      MPI_Alloc_mem(sizeof(int), MPI_INFO_NULL, &buf);
      *buf = nproc-1;
    } else
      buf = NULL;

    MPI_Win_create(buf, sizeof(int)*(rank == 0), 1, MPI_INFO_NULL, MPI_COMM_WORLD, &window);

    MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, window);

    /* GET-ACC: Test third-party communication, through rank 0. */
    for (i = 0; i < ITER; i++) {
        MPI_Request gacc_req;
        int val = -1, exp = -1;

        /* Processes form a ring.  Process 0 starts first, then passes a token
         * to the right.  Each process, in turn, performs third-party
         * communication via process 0's window. */
        if (rank > 0) {
            MPI_Recv(NULL, 0, MPI_BYTE, rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        MPI_Rget_accumulate(&rank, 1, MPI_INT, &val, 1, MPI_INT, 0, 0, 1, MPI_INT, MPI_REPLACE, window, &gacc_req);
        MPI_Wait(&gacc_req, MPI_STATUS_IGNORE);

        exp = (rank + nproc-1) % nproc;

        if (val != exp) {
            printf("%d - Got %d, expected %d\n", rank, val, exp);
            errors++;
        }

        if (rank < nproc-1) {
            MPI_Send(NULL, 0, MPI_BYTE, rank+1, 0, MPI_COMM_WORLD);
        }

        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) *buf = nproc-1;
    MPI_Win_sync(window);

    /* GET+PUT: Test third-party communication, through rank 0. */
    for (i = 0; i < ITER; i++) {
        MPI_Request req;
        int val = -1, exp = -1;

        /* Processes form a ring.  Process 0 starts first, then passes a token
         * to the right.  Each process, in turn, performs third-party
         * communication via process 0's window. */
        if (rank > 0) {
            MPI_Recv(NULL, 0, MPI_BYTE, rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        MPI_Rget(&val, 1, MPI_INT, 0, 0, 1, MPI_INT, window, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        MPI_Rput(&rank, 1, MPI_INT, 0, 0, 1, MPI_INT, window, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        exp = (rank + nproc-1) % nproc;

        if (val != exp) {
            printf("%d - Got %d, expected %d\n", rank, val, exp);
            errors++;
        }

        if (rank < nproc-1) {
            MPI_Send(NULL, 0, MPI_BYTE, rank+1, 0, MPI_COMM_WORLD);
        }

        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) *buf = nproc-1;
    MPI_Win_sync(window);

    /* GET+ACC: Test third-party communication, through rank 0. */
    for (i = 0; i < ITER; i++) {
        MPI_Request req;
        int val = -1, exp = -1;

        /* Processes form a ring.  Process 0 starts first, then passes a token
         * to the right.  Each process, in turn, performs third-party
         * communication via process 0's window. */
        if (rank > 0) {
            MPI_Recv(NULL, 0, MPI_BYTE, rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        MPI_Rget(&val, 1, MPI_INT, 0, 0, 1, MPI_INT, window, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        MPI_Raccumulate(&rank, 1, MPI_INT, 0, 0, 1, MPI_INT, MPI_REPLACE, window, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        exp = (rank + nproc-1) % nproc;

        if (val != exp) {
            printf("%d - Got %d, expected %d\n", rank, val, exp);
            errors++;
        }

        if (rank < nproc-1) {
            MPI_Send(NULL, 0, MPI_BYTE, rank+1, 0, MPI_COMM_WORLD);
        }

        MPI_Barrier(MPI_COMM_WORLD);
    }
    MPI_Win_unlock(0, window);

    MPI_Win_free(&window);
    if (buf) MPI_Free_mem(buf);

#endif /* TEST_MPI3_ROUTINES */

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
