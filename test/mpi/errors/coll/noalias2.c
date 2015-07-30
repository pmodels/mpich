/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This is a very weak sanity test that all nonblocking collectives specified by
 * MPI-3 are present in the library and take arguments as expected.  This test
 * does not check for progress, matching issues, or sensible output buffer
 * values. */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

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
    int *rbuf = NULL;
    int *scounts = NULL;
    int *rcounts = NULL;
    int *sdispls = NULL;
    int *rdispls = NULL;
    int *types = NULL;
    MPI_Comm comm;

    /* intentionally not using MTest_Init/MTest_Finalize in order to make it
     * easy to take this test and use it as an NBC sanity test outside of the
     * MPICH test suite */
    MPI_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    /* enough space for every process to contribute at least NUM_INTS ints to any
     * collective operation */
    sbuf = malloc(NUM_INTS * size * sizeof(int));
    my_assert(sbuf);
    rbuf = malloc(NUM_INTS * size * sizeof(int));
    my_assert(rbuf);
    scounts = malloc(size * sizeof(int));
    my_assert(scounts);
    rcounts = malloc(size * sizeof(int));
    my_assert(rcounts);
    sdispls = malloc(size * sizeof(int));
    my_assert(sdispls);
    rdispls = malloc(size * sizeof(int));
    my_assert(rdispls);
    types = malloc(size * sizeof(int));
    my_assert(types);

    for (i = 0; i < size; ++i) {
        sbuf[2 * i] = i;
        sbuf[2 * i + 1] = i;
        rbuf[2 * i] = i;
        rbuf[2 * i + 1] = i;
        scounts[i] = NUM_INTS;
        rcounts[i] = NUM_INTS;
        sdispls[i] = i * NUM_INTS;
        rdispls[i] = i * NUM_INTS;
        types[i] = MPI_INT;
    }

    if (rank == 0 && MPI_SUCCESS ==
        MPI_Gather(sbuf, NUM_INTS, MPI_INT, sbuf, NUM_INTS, MPI_INT, 0, comm))
        errs++;

    if (rank == 0 && MPI_SUCCESS ==
        MPI_Gatherv(sbuf, NUM_INTS, MPI_INT, sbuf, rcounts, rdispls, MPI_INT, 0, comm))
        errs++;

    if (rank == 0 && MPI_SUCCESS ==
        MPI_Scatter(sbuf, NUM_INTS, MPI_INT, sbuf, NUM_INTS, MPI_INT, 0, comm))
        errs++;

    if (rank == 0 && MPI_SUCCESS ==
        MPI_Scatterv(sbuf, scounts, sdispls, MPI_INT, sbuf, NUM_INTS, MPI_INT, 0, comm))
        errs++;

    if (MPI_SUCCESS == MPI_Allgather(&sbuf[rank], 1, MPI_INT, sbuf, 1, MPI_INT, comm))
        errs++;

    if (MPI_SUCCESS ==
        MPI_Allgatherv(&sbuf[rank * rcounts[rank]], rcounts[rank], MPI_INT, sbuf, rcounts, rdispls,
                       MPI_INT, comm))
        errs++;

    if (MPI_SUCCESS == MPI_Alltoall(sbuf, NUM_INTS, MPI_INT, sbuf, NUM_INTS, MPI_INT, comm))
        errs++;

    if (MPI_SUCCESS ==
        MPI_Alltoallv(sbuf, scounts, sdispls, MPI_INT, sbuf, scounts, sdispls, MPI_INT, comm))
        errs++;

    if (MPI_SUCCESS ==
        MPI_Alltoallw(sbuf, scounts, sdispls, types, sbuf, scounts, sdispls, types, comm))
        errs++;

    if (rank == 0 && MPI_SUCCESS == MPI_Reduce(sbuf, sbuf, NUM_INTS, MPI_INT, MPI_SUM, 0, comm))
        errs++;

    if (MPI_SUCCESS == MPI_Allreduce(sbuf, sbuf, NUM_INTS, MPI_INT, MPI_SUM, comm))
        errs++;

    if (MPI_SUCCESS == MPI_Reduce_scatter(sbuf, sbuf, rcounts, MPI_INT, MPI_SUM, comm))
        errs++;

    if (MPI_SUCCESS == MPI_Reduce_scatter_block(sbuf, sbuf, NUM_INTS, MPI_INT, MPI_SUM, comm))
        errs++;

    if (MPI_SUCCESS == MPI_Scan(sbuf, sbuf, NUM_INTS, MPI_INT, MPI_SUM, comm))
        errs++;

    if (MPI_SUCCESS == MPI_Exscan(sbuf, sbuf, NUM_INTS, MPI_INT, MPI_SUM, comm))
        errs++;

    if (sbuf)
        free(sbuf);
    if (rbuf)
        free(rbuf);
    if (scounts)
        free(scounts);
    if (rcounts)
        free(rcounts);
    if (sdispls)
        free(sdispls);
    if (rdispls)
        free(rdispls);

    if (rank == 0) {
        if (errs)
            fprintf(stderr, "Found %d errors\n", errs);
        else
            printf(" No errors\n");
    }
    MPI_Finalize();
    return 0;
}
