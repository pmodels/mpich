/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define DEFAULT_RNDV_SIZE 24*1024
/* Prototypes */
int SendRecvTest(int, int);
int IsendIrecvTest(int);
int IsenIrecvTest2(int, int);
int OutOfOrderTest(int, int);
int ForceUnexpectedTest(int, int);
int RndvTest(int, int, int);

int SendRecvTest(int rank, int n)
{
    int tag = 1;
    MPI_Status status;
    char buffer[100];
    int i;

    if (rank == 0) {
        strcpy(buffer, "Hello process one.");
        for (i = 0; i < n; i++)
            MPI_Send(buffer, 100, MPI_BYTE, 1, tag, MPI_COMM_WORLD);
    }
    else if (rank == 1) {
        for (i = 0; i < n; i++)
            MPI_Recv(buffer, 100, MPI_BYTE, 0, tag, MPI_COMM_WORLD, &status);
        /*printf("Rank 1: received message '%s'\n", buffer);fflush(stdout); */
    }

    return TRUE;
}

int IsendIrecvTest(int rank)
{
    int tag = 1;
    MPI_Status status;
    MPI_Request request;
    char buffer[100];

    if (rank == 0) {
        strcpy(buffer, "Hello process one.");
        MPI_Isend(buffer, 100, MPI_BYTE, 1, tag, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);
    }
    else if (rank == 1) {
        MPI_Irecv(buffer, 100, MPI_BYTE, 0, tag, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);
        /*printf("Rank 1: received message '%s'\n", buffer);fflush(stdout); */
    }

    return TRUE;
}

int IsendIrecvTest2(int rank, int buf_size);
int IsendIrecvTest2(int rank, int buf_size)
{
    int tag1 = 1;
    int tag2 = 2;
    MPI_Status status;
    MPI_Request request1, request2;
    char *buffer;

    buffer = (char *) malloc(buf_size);
    if (buffer == NULL)
        return FALSE;
    if (rank == 0) {
        strcpy(buffer, "Hello process one.");
        MPI_Isend(buffer, buf_size, MPI_BYTE, 1, tag1, MPI_COMM_WORLD, &request1);
        MPI_Isend(buffer, buf_size, MPI_BYTE, 1, tag2, MPI_COMM_WORLD, &request2);
        MPI_Wait(&request1, &status);
        MPI_Wait(&request2, &status);
    }
    else if (rank == 1) {
        MPI_Irecv(buffer, buf_size, MPI_BYTE, 0, tag1, MPI_COMM_WORLD, &request1);
        MPI_Irecv(buffer, buf_size, MPI_BYTE, 0, tag2, MPI_COMM_WORLD, &request2);
        MPI_Wait(&request1, &status);
        MPI_Wait(&request2, &status);
        /*printf("Rank 1: received message '%s'\n", buffer);fflush(stdout); */
    }

    free(buffer);
    return TRUE;
}

int OutOfOrderTest(int rank, int buf_size)
{
    int tag1 = 1;
    int tag2 = 2;
    MPI_Status status;
    MPI_Request request1, request2;
    char *buffer;

    buffer = (char *) malloc(buf_size);
    if (buffer == NULL)
        return FALSE;
    if (rank == 0) {
        strcpy(buffer, "Hello process one.");
        MPI_Isend(buffer, buf_size, MPI_BYTE, 1, tag1, MPI_COMM_WORLD, &request1);
        MPI_Isend(buffer, buf_size, MPI_BYTE, 1, tag2, MPI_COMM_WORLD, &request2);
        MPI_Wait(&request1, &status);
        MPI_Wait(&request2, &status);
    }
    else if (rank == 1) {
        MPI_Irecv(buffer, buf_size, MPI_BYTE, 0, tag2, MPI_COMM_WORLD, &request1);
        MPI_Irecv(buffer, buf_size, MPI_BYTE, 0, tag1, MPI_COMM_WORLD, &request2);
        MPI_Wait(&request2, &status);
        MPI_Wait(&request1, &status);
        /*printf("Rank 1: received message '%s'\n", buffer);fflush(stdout); */
    }

    free(buffer);
    return TRUE;
}

int ForceUnexpectedTest(int rank, int buf_size)
{
    int tag1 = 1;
    int tag2 = 2;
    MPI_Status status;
    MPI_Request request1, request2;
    char *buffer;

    buffer = (char *) malloc(buf_size);
    if (buffer == NULL)
        return FALSE;

    if (rank == 0) {
        strcpy(buffer, "Hello process one.");
        MPI_Isend(buffer, buf_size, MPI_BYTE, 1, tag1, MPI_COMM_WORLD, &request1);
        MPI_Isend(buffer, buf_size, MPI_BYTE, 1, tag2, MPI_COMM_WORLD, &request2);
        MPI_Wait(&request1, &status);
        MPI_Wait(&request2, &status);
        MPI_Recv(buffer, buf_size, MPI_BYTE, 1, tag1, MPI_COMM_WORLD, &status);
    }
    else if (rank == 1) {
        MPI_Irecv(buffer, buf_size, MPI_BYTE, 0, tag2, MPI_COMM_WORLD, &request2);
        MPI_Wait(&request2, &status);
        MPI_Irecv(buffer, buf_size, MPI_BYTE, 0, tag1, MPI_COMM_WORLD, &request1);
        MPI_Wait(&request1, &status);
        /*printf("Rank 1: received message '%s'\n", buffer);fflush(stdout); */
        MPI_Send(buffer, buf_size, MPI_BYTE, 0, tag1, MPI_COMM_WORLD);
    }

    free(buffer);

    return TRUE;
}

int RndvTest(int rank, int size, int reps)
{
    int tag = 1;
    MPI_Status status;
    char *buffer;
    int i;

    buffer = (char *) malloc(size);
    if (buffer == NULL) {
        printf("malloc failed to allocate %d bytes.\n", size);
        exit(0);
    }
    if (rank == 0) {
        for (i = 0; i < reps; i++) {
            if (reps == 1) {
                printf("0: sending to process 1\n");
                fflush(stdout);
            }
            MPI_Send(buffer, size, MPI_BYTE, 1, tag, MPI_COMM_WORLD);
            if (reps == 1) {
                printf("0: receiving from process 1\n");
                fflush(stdout);
            }
            MPI_Recv(buffer, size, MPI_BYTE, 1, tag, MPI_COMM_WORLD, &status);
            if (reps == 1) {
                printf("0: done\n");
                fflush(stdout);
            }
        }
    }
    else if (rank == 1) {
        for (i = 0; i < reps; i++) {
            if (reps == 1) {
                printf("1: receiving from process 0\n");
                fflush(stdout);
            }
            MPI_Recv(buffer, size, MPI_BYTE, 0, tag, MPI_COMM_WORLD, &status);
            if (reps == 1) {
                printf("1: sending to process 0\n");
                fflush(stdout);
            }
            MPI_Send(buffer, size, MPI_BYTE, 0, tag, MPI_COMM_WORLD);
            if (reps == 1) {
                printf("1: done\n");
                fflush(stdout);
            }
        }
    }
    free(buffer);

    return TRUE;
}

int main(int argc, char *argv[])
{
    int result;
    int size, rank;
    int bDoAll = FALSE;
    int reps;
    int rndv_size = DEFAULT_RNDV_SIZE;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size < 2) {
        printf("Two processes needed.\n");
        printf("options:\n");
        printf(" sr [reps] ............... send/recv\n");
        printf(" isr ..................... isend/irecv\n");
        printf(" iisr .................... isend,isend/irecv,irecv wait\n");
        printf(" oo ...................... out of order isend/irecv\n");
        printf(" unex .................... force unexpected msg\n");
        printf(" rndv [size] ............. rndv\n");
        printf(" rndv_reps [reps] [size] . rndv\n");
        printf(" rndv_iisr [size] ........ rndv iisr\n");
        printf(" rndv_oo [size] .......... rndv oo\n");
        printf(" rndv_unex [size] ........ rndv unex\n");
        printf("default rndv size = %d bytes\n", DEFAULT_RNDV_SIZE);
        MPI_Finalize();
        return 0;
    }

    if (rank > 1) {
        printf("Rank %d, I am not participating.\n", rank);
        fflush(stdout);
    }
    else {
        if (argc < 2)
            bDoAll = TRUE;

        if (bDoAll || (strcmp(argv[1], "sr") == 0)) {
            reps = 1;
            if (argc > 2) {
                reps = atoi(argv[2]);
                if (reps < 1)
                    reps = 1;
            }
            if (rank == 0) {
                printf("Send/recv test: %d reps\n", reps);
                fflush(stdout);
            }
            result = SendRecvTest(rank, reps);
            printf(result ? "%d:SUCCESS - sr\n" : "%d:FAILURE - sr\n", rank);
            fflush(stdout);
        }

        if (bDoAll || (strcmp(argv[1], "isr") == 0)) {
            if (rank == 0) {
                printf("Isend/irecv wait test\n");
                fflush(stdout);
            }
            result = IsendIrecvTest(rank);
            printf(result ? "%d:SUCCESS - isr\n" : "%d:FAILURE - isr\n", rank);
            fflush(stdout);
        }

        if (bDoAll || (strcmp(argv[1], "iisr") == 0)) {
            if (rank == 0) {
                printf("Isend,isend/irecv,irecv wait wait test\n");
                fflush(stdout);
            }
            result = IsendIrecvTest2(rank, 100);
            printf(result ? "%d:SUCCESS - iisr\n" : "%d:FAILURE - iisr\n", rank);
            fflush(stdout);
        }

        if (bDoAll || (strcmp(argv[1], "oo") == 0)) {
            if (rank == 0) {
                printf("Out of order isend/irecv test\n");
                fflush(stdout);
            }
            result = OutOfOrderTest(rank, 100);
            printf(result ? "%d:SUCCESS - oo\n" : "%d:FAILURE - oo\n", rank);
            fflush(stdout);
        }

        if (bDoAll || (strcmp(argv[1], "unex") == 0)) {
            if (rank == 0) {
                printf("Force unexpected message test\n");
                fflush(stdout);
            }
            result = ForceUnexpectedTest(rank, 100);
            printf(result ? "%d:SUCCESS - unex\n" : "%d:FAILURE - unex\n", rank);
            fflush(stdout);
        }

        if (bDoAll || (strcmp(argv[1], "rndv") == 0)) {
            if (argc > 2) {
                rndv_size = atoi(argv[2]);
                if (rndv_size < 1024)
                    rndv_size = 1024;
            }
            if (rank == 0) {
                printf("Rndv test\n");
                fflush(stdout);
            }
            result = RndvTest(rank, rndv_size, 1);
            printf(result ? "%d:SUCCESS - rndv\n" : "%d:FAILURE - rndv\n", rank);
            fflush(stdout);
        }

        if (bDoAll || (strcmp(argv[1], "rndv_reps") == 0)) {
            reps = 100;
            if (argc > 2) {
                reps = atoi(argv[2]);
                if (reps < 1)
                    reps = 1;
            }
            if (argc > 3) {
                rndv_size = atoi(argv[3]);
            }
            if (rank == 0) {
                printf("Rndv test: %d reps of size %d\n", reps, rndv_size);
                fflush(stdout);
            }
            result = RndvTest(rank, rndv_size, reps);
            printf(result ? "%d:SUCCESS - rndv_reps\n" : "%d:FAILURE - rndv_reps\n", rank);
            fflush(stdout);
        }

        if (bDoAll || (strcmp(argv[1], "rndv_iisr") == 0)) {
            if (rank == 0) {
                printf("Rndv isend,isend/irecv,irecv wait wait test\n");
                fflush(stdout);
            }
            if (argc > 2) {
                rndv_size = atoi(argv[2]);
            }
            result = IsendIrecvTest2(rank, rndv_size);
            printf(result ? "%d:SUCCESS - rndv_iisr\n" : "%d:FAILURE - rndv_iisr\n", rank);
            fflush(stdout);
        }

        if (bDoAll || (strcmp(argv[1], "rndv_oo") == 0)) {
            if (rank == 0) {
                printf("Rndv out of order isend/irecv test\n");
                fflush(stdout);
            }
            if (argc > 2) {
                rndv_size = atoi(argv[2]);
            }
            result = OutOfOrderTest(rank, rndv_size);
            printf(result ? "%d:SUCCESS - rndv_oo\n" : "%d:FAILURE - rndv_oo\n", rank);
            fflush(stdout);
        }

        if (bDoAll || (strcmp(argv[1], "rndv_unex") == 0)) {
            if (argc > 2) {
                rndv_size = atoi(argv[2]);
                if (rndv_size < 1024)
                    rndv_size = 1024;
            }
            if (rank == 0) {
                printf("Force unexpected rndv message test\n");
                fflush(stdout);
            }
            result = ForceUnexpectedTest(rank, rndv_size);
            printf(result ? "%d:SUCCESS - rndv_unex\n" : "%d:FAILURE - rndv_unex\n", rank);
            fflush(stdout);
        }
    }

    MPI_Finalize();
    return 0;
}
