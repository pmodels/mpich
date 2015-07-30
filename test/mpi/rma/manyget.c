/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test triggers a limitation in the Portals4 netmod where
 * too many large messages can overflow the available ME entries
 * (PTL_NO_SPACE). Our approach is to queue the entire send message
 * in the Rportals layer until we know there is ME space available.
 */
#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

#define BUFSIZE (128*1024)

int main(int argc, char *argv[])
{
    int i, rank, size;
    int *buf;
    MPI_Win win;

    MPI_Init(&argc, &argv);

    buf = malloc(BUFSIZE);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        printf("test must be run with 2 processes!\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 0)
        MPI_Win_create(buf, BUFSIZE, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    else
        MPI_Win_create(MPI_BOTTOM, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    MPI_Win_fence(0, win);

    if (rank == 1) {
        for (i = 0; i < 100000; i++)
            MPI_Get(buf, BUFSIZE / sizeof(int), MPI_INT, 0, 0, BUFSIZE / sizeof(int), MPI_INT, win);
    }

    MPI_Win_fence(0, win);
    MPI_Win_free(&win);

    if (rank == 0)
        printf(" No Errors\n");

    free(buf);
    MPI_Finalize();

    return 0;
}
