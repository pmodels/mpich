/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This is a very weak sanity test that persistenr broadcast collectives specified by
 * proposed MPI-4 is present in the library and take arguments as expected.  This test
 * does not check for progress, matching issues, or sensible output buffer
 * values. */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

#define NUM_INTS (2)

#define my_assert(cond_)                                                  \
    do {                                                                  \
        if (!(cond_)) {                                                   \
            fprintf(stderr, "assertion (%s) failed, aborting\n", #cond_); \
            MPI_Abort(MPI_COMM_WORLD, 1);                                 \
        }                                                                 \
    } while (0)

int main(int argc, char **argv)
{
    int errs = 0;
    int i;
    int rank, size;
    int *sbuf = NULL;
    MPI_Comm comm;

    /* intentionally not using MTest_Init/MTest_Finalize in order to make it
     * easy to take this test and use it as an persistence sanity test outside of the
     * MPICH test suite */
    MPI_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    sbuf = malloc(NUM_INTS * sizeof(int));
    my_assert(sbuf);

    for (i = 0; i < NUM_INTS; ++i) {
        sbuf[i] = i;
    }

    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Request req;

    MPIX_Bcast_init(sbuf, NUM_INTS, MPI_INT, 0, comm, info, &req);
    MPI_Start(&req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    MPI_Request_free(&req);

    MPI_Info_free(&info);

    if (sbuf)
        free(sbuf);

    if (rank == 0) {
        if (errs)
            fprintf(stderr, "Found %d errors\n", errs);
        else
            printf(" No errors\n");
    }
    MPI_Finalize();
    return 0;
}
