/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdlib.h>
#include <stdio.h>

#include <mpi.h>
#include "mpitest.h"

int sizes[] = { 64, 32768, 2000000 };

int main(int argc, char **argv)
{
    int errs = 0;
    int rank, i, j, msg_size;
    char *buf;
    MPI_Request req;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (i = 0; i < 3; i++) {
        msg_size = sizes[i];
        buf = malloc(msg_size);
        memset(buf, 0, msg_size);

        if (rank == 0) {
            MTestPrintfMsg(1, "Begin testing size = %d\n", msg_size);
            MPI_Recv_init(buf, msg_size, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
                          &req);

            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Start(&req);
            MPI_Wait(&req, MPI_STATUS_IGNORE);

            MPI_Request_free(&req);
        } else if (rank == 1) {
            for (j = 0; j < msg_size; j++)
                buf[j] = j % 100;

            MPI_Isend(buf, msg_size, MPI_BYTE, 0, 1, MPI_COMM_WORLD, &req);
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Wait(&req, MPI_STATUS_IGNORE);
        }

        for (j = 0; j < msg_size; j++)
            errs += (buf[j] != (j % 100));

        free(buf);
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
