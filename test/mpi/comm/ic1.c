/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * A simple test of the intercomm create routine, with a communication test
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    MPI_Comm intercomm;
    int remote_rank, rank, size, errs = 0;
    volatile int trigger;

    MTest_Init(&argc, &argv);

    trigger = 1;
/*    while (trigger) ; */

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size < 2) {
        printf("Size must be at least 2\n");
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* Make an intercomm of the first two elements of comm_world */
    if (rank < 2) {
        int lrank = rank, rrank = -1;
        MPI_Status status;

        remote_rank = 1 - rank;
        MPI_Intercomm_create(MPI_COMM_SELF, 0, MPI_COMM_WORLD, remote_rank, 27, &intercomm);

        /* Now, communicate between them */
        MPI_Sendrecv(&lrank, 1, MPI_INT, 0, 13, &rrank, 1, MPI_INT, 0, 13, intercomm, &status);

        if (rrank != remote_rank) {
            errs++;
            printf("%d Expected %d but received %d\n", rank, remote_rank, rrank);
        }

        MPI_Comm_free(&intercomm);
    }

    /* The next test should create an intercomm with groups of different
     * sizes FIXME */

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
