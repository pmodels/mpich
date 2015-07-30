/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
   This program performs a short test of MPI_BSEND in a
   multithreaded environment.

   It starts a single receiver thread that expects
   NUMSENDS messages and NUMSENDS sender threads, that
   use MPI_Bsend to send a message of size MSGSIZE
   to its right neigbour or rank 0 if (my_rank==comm_size-1), i.e.
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

#define NUMSENDS 32
#define BUFSIZE 10000000
#define MSGSIZE 1024

int rank, size;

void *receiver(void *ptr)
{
    int k;
    char buf[MSGSIZE];

    for (k = 0; k < NUMSENDS; k++)
        MPI_Recv(buf, MSGSIZE, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    return NULL;
}


void *sender_bsend(void *ptr)
{
    char buffer[MSGSIZE];
    MPI_Bsend(buffer, MSGSIZE, MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD);

    return NULL;
}

void *sender_ibsend(void *ptr)
{
    char buffer[MSGSIZE];
    MPI_Request req;
    MPI_Ibsend(buffer, MSGSIZE, MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    return NULL;
}

void *sender_isend(void *ptr)
{
    char buffer[MSGSIZE];
    MPI_Request req;
    MPI_Isend(buffer, MSGSIZE, MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    return NULL;
}

void *sender_send(void *ptr)
{
    char buffer[MSGSIZE];
    MPI_Send(buffer, MSGSIZE, MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD);

    return NULL;
}

int main(int argc, char *argv[])
{

    int provided, i[2], k;
    char *buffer, *ptr_dt;
    buffer = (char *) malloc(BUFSIZE * sizeof(char));
    MPI_Status status;
    pthread_t receiver_thread, sender_thread[NUMSENDS];
    pthread_attr_t attr;
    MPI_Comm communicator;
    int bs;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    if (provided != MPI_THREAD_MULTIPLE) {
        printf("Error\n");
        MPI_Abort(911, MPI_COMM_WORLD);
    }

    MPI_Buffer_attach(buffer, BUFSIZE);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_dup(MPI_COMM_WORLD, &communicator);        /* We do not use this communicator in this program, but
                                                         * with this call, the problem appears more reliably.
                                                         * If the MPI_Comm_dup() call is commented out, it is still
                                                         * evident but does not appear that often (don't know why) */

    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&receiver_thread, &attr, &receiver, NULL);
    for (k = 0; k < NUMSENDS; k++)
        pthread_create(&sender_thread[k], &attr, &sender_bsend, NULL);
    pthread_join(receiver_thread, NULL);
    for (k = 0; k < NUMSENDS; k++)
        pthread_join(sender_thread[k], NULL);
    MPI_Barrier(MPI_COMM_WORLD);

    pthread_create(&receiver_thread, &attr, &receiver, NULL);
    for (k = 0; k < NUMSENDS; k++)
        pthread_create(&sender_thread[k], &attr, &sender_ibsend, NULL);
    pthread_join(receiver_thread, NULL);
    for (k = 0; k < NUMSENDS; k++)
        pthread_join(sender_thread[k], NULL);
    MPI_Barrier(MPI_COMM_WORLD);

    pthread_create(&receiver_thread, &attr, &receiver, NULL);
    for (k = 0; k < NUMSENDS; k++)
        pthread_create(&sender_thread[k], &attr, &sender_isend, NULL);
    pthread_join(receiver_thread, NULL);
    for (k = 0; k < NUMSENDS; k++)
        pthread_join(sender_thread[k], NULL);
    MPI_Barrier(MPI_COMM_WORLD);

    pthread_create(&receiver_thread, &attr, &receiver, NULL);
    for (k = 0; k < NUMSENDS; k++)
        pthread_create(&sender_thread[k], &attr, &sender_send, NULL);
    pthread_join(receiver_thread, NULL);
    for (k = 0; k < NUMSENDS; k++)
        pthread_join(sender_thread[k], NULL);
    MPI_Barrier(MPI_COMM_WORLD);

    pthread_attr_destroy(&attr);
    if (!rank)
        printf(" No Errors\n");

    MPI_Comm_free(&communicator);
    MPI_Buffer_detach(&ptr_dt, &bs);
    free(buffer);
    MPI_Finalize();
    return 0;
}
