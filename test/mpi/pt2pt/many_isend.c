/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2015 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define ITER 5
#define BUF_COUNT 4096

int rank, nprocs;
char recvbuf[BUF_COUNT], sendbuf[BUF_COUNT];

int main(int argc, char *argv[])
{
    int x, i;
    MPI_Status *sendstats = NULL;
    MPI_Request *sendreqs = NULL;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    sendreqs = (MPI_Request *) malloc(nprocs * sizeof(MPI_Request));
    sendstats = (MPI_Status *) malloc(nprocs * sizeof(MPI_Status));

    for (x = 0; x < ITER; x++) {
        MPI_Barrier(MPI_COMM_WORLD);

        /* all to all send */
        for (i = 0; i < nprocs; i++) {
            MPI_Isend(sendbuf, BUF_COUNT, MPI_CHAR, i, 0, MPI_COMM_WORLD, &sendreqs[i]);
        }

        /* receive one by one */
        for (i = 0; i < nprocs; i++) {
            MPI_Recv(recvbuf, BUF_COUNT, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
        }

        /* ensure all send request completed */
        MPI_Waitall(nprocs, sendreqs, sendstats);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0)
        printf(" No Errors\n");

    free(sendreqs);
    free(sendstats);

    MPI_Finalize();

    return 0;
}
