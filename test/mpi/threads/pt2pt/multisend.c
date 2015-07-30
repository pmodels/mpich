/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "mpithreadtest.h"

int rank;

MTEST_THREAD_RETURN_TYPE run_test(void *arg);
MTEST_THREAD_RETURN_TYPE run_test(void *arg)
{
    MPI_Status reqstat;
    int i, j;
    int peer = rank ? 0 : 1;

    for (j = 0; j < 10; j++) {
        if (rank % 2) {
            for (i = 0; i < 16; i++)
                MPI_Send(NULL, 0, MPI_CHAR, peer, j, MPI_COMM_WORLD);
            for (i = 0; i < 16; i++)
                MPI_Recv(NULL, 0, MPI_CHAR, peer, j, MPI_COMM_WORLD, &reqstat);
        }
        else {
            for (i = 0; i < 16; i++)
                MPI_Recv(NULL, 0, MPI_CHAR, peer, j, MPI_COMM_WORLD, &reqstat);
            for (i = 0; i < 16; i++)
                MPI_Send(NULL, 0, MPI_CHAR, peer, j, MPI_COMM_WORLD);
        }
    }
    return (MTEST_THREAD_RETURN_TYPE) NULL;
}

int main(int argc, char **argv)
{
    int zero = 0, pmode, nprocs;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &pmode);
    if (pmode != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "Thread Multiple not supported by the MPI implementation\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (nprocs != 2) {
        fprintf(stderr, "Need two processes\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    MTest_Start_thread(run_test, NULL);
    run_test(&zero);
    MTest_Join_threads();

    MPI_Finalize();

    /* This program works if it gets here */
    if (rank == 0) {
        printf(" No Errors\n");
    }

    return 0;
}
