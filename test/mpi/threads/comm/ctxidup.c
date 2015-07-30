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
 * static char MTEST_Descrip[] = "Creating communications concurrently in different threads";
 */

MTEST_THREAD_RETURN_TYPE dup_thread(void *);

MTEST_THREAD_RETURN_TYPE dup_thread(void *p)
{
    int rank;
    int buffer[1];
    MPI_Comm *comm2_ptr = (MPI_Comm *) p;
    MPI_Comm comm3;
    MPI_Request req;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank & 0x1) {
        /* If odd, wait for message */
        MPI_Recv(buffer, 0, MPI_INT, rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    MPI_Comm_idup(*comm2_ptr, &comm3, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Barrier(comm3);
    MPI_Recv(buffer, 0, MPI_INT, rank, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    MPI_Comm_free(&comm3);
    /* Tell the main thread that we're done */
    MPI_Send(buffer, 0, MPI_INT, rank, 2, MPI_COMM_WORLD);

    return (MTEST_THREAD_RETURN_TYPE) 0;
}

int main(int argc, char *argv[])
{
    int rank, size;
    int provided;
    int buffer[1];
    MPI_Comm comm1, comm2, comm4;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* Check that we're multi-threaded */
    if (provided != MPI_THREAD_MULTIPLE) {
        if (rank == 0) {
            printf
                ("MPI_Init_thread must return MPI_THREAD_MULTIPLE in order for this test to run.\n");
            fflush(stdout);
        }
        MPI_Finalize();
        return 1;
    }

    /* The test is this:
     * The main thread on ODD processors tells the other thread to start
     * a comm dup(on comm2), then starts a comm dup(on comm1) after a delay.
     * The main thread on even processors starts a comm dup(on comm1)
     *
     * The second thread on ODD processors waits until it gets a message
     * (from the same process) before starting the comm dup on comm2.
     */

    /* Create two communicators */
    MPI_Comm_dup(MPI_COMM_WORLD, &comm1);
    MPI_Comm_dup(MPI_COMM_WORLD, &comm2);

    /* Start a thread that will perform a dup comm2 */
    MTest_Start_thread(dup_thread, (void *) &comm2);

    /* If we're odd, send to our new thread and then delay */
    if (rank & 0x1) {
        MPI_Ssend(buffer, 0, MPI_INT, rank, 0, MPI_COMM_WORLD);
        MTestSleep(1);
    }
    MPI_Comm_dup(comm1, &comm4);

    /* Tell the threads to exit after we've created our new comm */
    MPI_Barrier(comm4);
    MPI_Ssend(buffer, 0, MPI_INT, rank, 1, MPI_COMM_WORLD);
    MPI_Recv(buffer, 0, MPI_INT, rank, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    MPI_Comm_free(&comm4);
    MPI_Comm_free(&comm1);
    MPI_Comm_free(&comm2);

    MTest_Finalize(0);
    MPI_Finalize();
    return 0;
}
