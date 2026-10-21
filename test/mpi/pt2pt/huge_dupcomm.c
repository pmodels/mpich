/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * This program checks if MPICH can correctly handle huge message
 * sends over multiple different communicators simultaneously
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run pt2pt_huge_dupcomm
int run(const char *arg);
#endif

#define COUNT (4*1024*1024)
#define NCOMMS 4

int run(const char *arg)
{
    int *buff;
    int size, rank;
    int i;
    MPI_Comm comms[NCOMMS];
    MPI_Request reqs[NCOMMS];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        fprintf(stderr, "Launch with two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    buff = malloc(COUNT * NCOMMS * sizeof(int));

    for (i = 0; i < NCOMMS; i++)
        MPI_Comm_dup(MPI_COMM_WORLD, &comms[i]);

    for (i = 0; i < NCOMMS; i++) {
        if (rank == 0)
            MPI_Isend(buff + COUNT * i, COUNT, MPI_INT, 1 /* dest */ , 0 /* tag */ , comms[i],
                      &reqs[i]);
        else
            MPI_Irecv(buff + COUNT * i, COUNT, MPI_INT, 0 /* src */ , 0 /* tag */ , comms[i],
                      &reqs[i]);
    }
    MPI_Waitall(NCOMMS, reqs, MPI_STATUSES_IGNORE);

    for (i = 0; i < NCOMMS; i++)
        MPI_Comm_free(&comms[i]);

    free(buff);

    return 0;
}
