/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "mpitest.h"

#define NUM_LOOPS  (128)

int main(int argc, char **argv)
{
    int i, rank, size;
    MPI_Request *req;
    MPI_Datatype newtype;
    int snd_buf[3], rcv_buf[3];
    int count = 2;
    int *displs;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        fprintf(stderr, "Must run with at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    displs = (int *) malloc(count * sizeof(int));
    for (i = 0; i < count; i++)
        displs[i] = i * 2;

    req = (MPI_Request *) malloc(NUM_LOOPS * sizeof(MPI_Request));

    MPI_Barrier(MPI_COMM_WORLD);

    /* test isends */
    MPI_Type_create_indexed_block(count, 1, displs, MPI_INT, &newtype);
    MPI_Type_commit(&newtype);

    if (rank == 0) {
        for (i = 0; i < NUM_LOOPS; i++)
            MPI_Isend(snd_buf, 1, newtype, !rank, 0, MPI_COMM_WORLD, &req[i]);
    } else {
        for (i = 0; i < NUM_LOOPS; i++)
            MPI_Recv(rcv_buf, 1, newtype, !rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    MPI_Type_free(&newtype);
    if (rank == 0)
        MPI_Waitall(NUM_LOOPS, req, MPI_STATUSES_IGNORE);

    MPI_Barrier(MPI_COMM_WORLD);

    /* test issends */
    MPI_Type_create_indexed_block(count, 1, displs, MPI_INT, &newtype);
    MPI_Type_commit(&newtype);

    if (rank == 0) {
        for (i = 0; i < NUM_LOOPS; i++)
            MPI_Issend(snd_buf, 1, newtype, !rank, 0, MPI_COMM_WORLD, &req[i]);
    } else {
        for (i = 0; i < NUM_LOOPS; i++)
            MPI_Recv(rcv_buf, 1, newtype, !rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    MPI_Type_free(&newtype);
    if (rank == 0)
        MPI_Waitall(NUM_LOOPS, req, MPI_STATUSES_IGNORE);

    MPI_Barrier(MPI_COMM_WORLD);

    /* test irsends */
    MPI_Type_create_indexed_block(count, 1, displs, MPI_INT, &newtype);
    MPI_Type_commit(&newtype);

    if (rank == 0) {
        MPI_Barrier(MPI_COMM_WORLD);
        for (i = 0; i < NUM_LOOPS; i++)
            MPI_Irsend(snd_buf, 1, newtype, !rank, 0, MPI_COMM_WORLD, &req[i]);
    } else {
        for (i = 0; i < NUM_LOOPS; i++)
            MPI_Irecv(rcv_buf, 1, newtype, !rank, 0, MPI_COMM_WORLD, &req[i]);
        MPI_Barrier(MPI_COMM_WORLD);
    }
    MPI_Type_free(&newtype);
    MPI_Waitall(NUM_LOOPS, req, MPI_STATUSES_IGNORE);

    MPI_Barrier(MPI_COMM_WORLD);

    MTest_Finalize(0);

    free(displs);
    free(req);

    return 0;
}
