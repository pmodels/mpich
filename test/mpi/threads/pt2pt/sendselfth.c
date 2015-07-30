/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "mpithreadtest.h"

/*
static char MTEST_Descrip[] = "Send to self in a threaded program";
*/

MTEST_THREAD_RETURN_TYPE send_thread(void *p);
MTEST_THREAD_RETURN_TYPE send_thread(void *p)
{
    int rank;
    char buffer[100];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Send(buffer, sizeof(buffer), MPI_CHAR, rank, 0, MPI_COMM_WORLD);
    return MTEST_THREAD_RETVAL_IGN;
}

int main(int argc, char *argv[])
{
    int rank, size;
    int provided;
    char buffer[100];
    MPI_Status status;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (provided != MPI_THREAD_MULTIPLE) {
        if (rank == 0) {
            printf
                ("MPI_Init_thread must return MPI_THREAD_MULTIPLE in order for this test to run.\n");
            fflush(stdout);
        }
        MPI_Finalize();
        return -1;
    }

    MTest_Start_thread(send_thread, NULL);

    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, rank, 0, MPI_COMM_WORLD, &status);

    MTest_Join_threads();

    MTest_Finalize(0);
    MPI_Finalize();
    return 0;
}
