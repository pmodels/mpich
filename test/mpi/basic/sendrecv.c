/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LARGE_SIZE  100*1024
#define RNDV_SIZE   256*1024

int main(int argc, char *argv[])
{
    int size, rank;
    char buffer[100] = "garbage";
    char big_buffer[RNDV_SIZE] = "big garbage";
    MPI_Status status;
    int tag = 1;
    int reps = 1;
    int i;

    printf("Simple Send/Recv test.\n");
    fflush(stdout);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size < 2) {
        printf("Two processes needed.\n");
        MPI_Finalize();
        return 0;
    }

    if (argc > 1) {
        reps = atoi(argv[1]);
        if (reps < 1)
            reps = 1;
    }

    if (rank == 0) {
        printf("Rank 0: sending 100 bytes messages to process 1.\n");
        fflush(stdout);
        strcpy(buffer, "Hello process one.");
        for (i = 0; i < reps; i++) {
            MPI_Send(buffer, 100, MPI_BYTE, 1, tag, MPI_COMM_WORLD);
            MPI_Recv(buffer, 100, MPI_BYTE, 1, tag, MPI_COMM_WORLD, &status);
        }
        strcpy(big_buffer, "Hello again process one.");
        printf("Rank 0: sending %dk bytes messages to process 1.\n", LARGE_SIZE / 1024);
        fflush(stdout);
        for (i = 0; i < reps; i++) {
            MPI_Send(big_buffer, LARGE_SIZE, MPI_BYTE, 1, tag, MPI_COMM_WORLD);
            MPI_Recv(big_buffer, LARGE_SIZE, MPI_BYTE, 1, tag, MPI_COMM_WORLD, &status);
        }
        strcpy(big_buffer, "Hello yet again process one.");
        printf("Rank 0: sending %dk bytes messages to process 1.\n", RNDV_SIZE / 1024);
        fflush(stdout);
        for (i = 0; i < reps; i++) {
            MPI_Send(big_buffer, RNDV_SIZE, MPI_BYTE, 1, tag, MPI_COMM_WORLD);
            MPI_Recv(big_buffer, RNDV_SIZE, MPI_BYTE, 1, tag, MPI_COMM_WORLD, &status);
        }
    }
    else if (rank == 1) {
        printf("Rank 1: receiving messages from process 0.\n");
        fflush(stdout);
        for (i = 0; i < reps; i++) {
            MPI_Recv(buffer, 100, MPI_BYTE, 0, tag, MPI_COMM_WORLD, &status);
            MPI_Send(buffer, 100, MPI_BYTE, 0, tag, MPI_COMM_WORLD);
        }
        printf("Rank 1: received message '%s'\n", buffer);
        fflush(stdout);
        for (i = 0; i < reps; i++) {
            MPI_Recv(big_buffer, LARGE_SIZE, MPI_BYTE, 0, tag, MPI_COMM_WORLD, &status);
            MPI_Send(big_buffer, LARGE_SIZE, MPI_BYTE, 0, tag, MPI_COMM_WORLD);
        }
        printf("Rank 1: received message '%s'\n", big_buffer);
        fflush(stdout);
        for (i = 0; i < reps; i++) {
            MPI_Recv(big_buffer, RNDV_SIZE, MPI_BYTE, 0, tag, MPI_COMM_WORLD, &status);
            MPI_Send(big_buffer, RNDV_SIZE, MPI_BYTE, 0, tag, MPI_COMM_WORLD);
        }
        printf("Rank 1: received message '%s'\n", big_buffer);
        fflush(stdout);
    }
    else {
        printf("Rank %d, I am not participating.\n", rank);
        fflush(stdout);
    }

    MPI_Finalize();
    return 0;
}
