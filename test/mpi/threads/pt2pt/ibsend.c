/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
   This program performs a short test of MPI_BSEND in a
   multithreaded environment.

   It starts a single receiver thread that expects
   NUMSENDS from NUMTHREADS sender threads, that
   use [i]bsend/[i]send to send a message of size MSGSIZE
   to its right neighbor or rank 0 if (my_rank==comm_size-1), i.e.
   target_rank = (my_rank+1)%size .

   After all messages have been received, the
   receiver thread prints a message, the threads
   are joined into the main thread and the application
   terminates.
   */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include "mpitest.h"
#include "mpithreadtest.h"

#define NUMSENDS 4
#define NUMTHREADS 8
#define BUFSIZE 10000000
#define MSGSIZE 1024

int rank, size;

static MTEST_THREAD_RETURN_TYPE receiver(void *ptr)
{
    int k;
    char buf[MSGSIZE];

    for (k = 0; k < NUMSENDS * NUMTHREADS; k++)
        MPI_Recv(buf, MSGSIZE, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    return NULL;
}


static MTEST_THREAD_RETURN_TYPE sender_bsend(void *ptr)
{
    char buffer[MSGSIZE];
    int i;
    MTEST_VG_MEM_INIT(buffer, MSGSIZE * sizeof(char));
    for (i = 0; i < NUMSENDS; i++)
        MPI_Bsend(buffer, MSGSIZE, MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD);

    return NULL;
}

static MTEST_THREAD_RETURN_TYPE sender_ibsend(void *ptr)
{
    char buffer[MSGSIZE];
    int i;
    MPI_Request reqs[NUMSENDS];
    MTEST_VG_MEM_INIT(buffer, MSGSIZE * sizeof(char));
    for (i = 0; i < NUMSENDS; i++)
        MPI_Ibsend(buffer, MSGSIZE, MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD, &reqs[i]);
    MPI_Waitall(NUMSENDS, reqs, MPI_STATUSES_IGNORE);

    return NULL;
}

static MTEST_THREAD_RETURN_TYPE sender_isend(void *ptr)
{
    char buffer[MSGSIZE];
    int i;
    MPI_Request reqs[NUMSENDS];
    MTEST_VG_MEM_INIT(buffer, MSGSIZE * sizeof(char));
    for (i = 0; i < NUMSENDS; i++)
        MPI_Isend(buffer, MSGSIZE, MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD, &reqs[i]);
    MPI_Waitall(NUMSENDS, reqs, MPI_STATUSES_IGNORE);

    return NULL;
}

static MTEST_THREAD_RETURN_TYPE sender_send(void *ptr)
{
    char buffer[MSGSIZE];
    int i;
    MTEST_VG_MEM_INIT(buffer, MSGSIZE * sizeof(char));
    for (i = 0; i < NUMSENDS; i++)
        MPI_Send(buffer, MSGSIZE, MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD);

    return NULL;
}

int main(int argc, char *argv[])
{

    int provided, k;
    char *buffer, *ptr_dt;
    buffer = (char *) malloc(BUFSIZE * sizeof(char));
    MTEST_VG_MEM_INIT(buffer, BUFSIZE * sizeof(char));
    MPI_Comm communicator;
    int bs;

    MTest_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    if (provided != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI_THREAD_MULTIPLE not supported by the MPI implementation\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    MPI_Buffer_attach(buffer, BUFSIZE);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_dup(MPI_COMM_WORLD, &communicator);        /* We do not use this communicator in this program, but
                                                         * with this call, the problem appears more reliably.
                                                         * If the MPI_Comm_dup() call is commented out, it is still
                                                         * evident but does not appear that often (don't know why) */
    MTest_Start_thread(receiver, NULL);
    for (k = 0; k < NUMTHREADS; k++)
        MTest_Start_thread(sender_bsend, NULL);
    MTest_Join_threads();
    MPI_Barrier(MPI_COMM_WORLD);

    MTest_Start_thread(receiver, NULL);
    for (k = 0; k < NUMTHREADS; k++)
        MTest_Start_thread(sender_ibsend, NULL);
    MTest_Join_threads();
    MPI_Barrier(MPI_COMM_WORLD);

    MTest_Start_thread(receiver, NULL);
    for (k = 0; k < NUMTHREADS; k++)
        MTest_Start_thread(sender_isend, NULL);
    MTest_Join_threads();
    MPI_Barrier(MPI_COMM_WORLD);

    MTest_Start_thread(receiver, NULL);
    for (k = 0; k < NUMTHREADS; k++)
        MTest_Start_thread(sender_send, NULL);
    MTest_Join_threads();
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Comm_free(&communicator);
    MPI_Buffer_detach(&ptr_dt, &bs);
    free(buffer);

    MTest_Finalize(0);
    return 0;
}
