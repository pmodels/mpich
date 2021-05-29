/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This is a very weak sanity test that persistent broadcast collectives specified by
 * proposed MPI-4 is present in the library and take arguments as expected.  This test
 * does not check for progress or  matching issues. */

#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>

#define NUM_INTS (10)

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
    int count = 1000;
    int j;
    int root = 0;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    sbuf = malloc(NUM_INTS * sizeof(int));
    my_assert(sbuf);

    if (rank == root) {
        for (i = 0; i < NUM_INTS; ++i) {
            sbuf[i] = i;
        }
    }

    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Request req;

    MPI_Bcast_init(sbuf, NUM_INTS, MPI_INT, root, comm, info, &req);

    for (i = 0; i < count; i++) {
        if (rank != root) {
            for (j = 0; j < NUM_INTS; ++j) {
                sbuf[j] = -1;
            }
        }
        MPI_Start(&req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        /* Check the results */
        for (j = 0; j < NUM_INTS; j++) {
            if (sbuf[j] != j) {
                errs++;
                if (errs < 10) {
                    fprintf(stderr, "sbuf[%d] = %d expected %d\n", j, sbuf[j], j);
                }
            }
        }
    }
    MPI_Request_free(&req);

    MPI_Info_free(&info);

    free(sbuf);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
