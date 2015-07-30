/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This is a very weak sanity test that all nonblocking collectives specified by
 * MPI-3 are present in the library and take arguments as expected.  This test
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
    int *rbuf = NULL;
    int *scounts = NULL;
    int *rcounts = NULL;
    int *sdispls = NULL;
    int *rdispls = NULL;
    int *types = NULL;
    MPI_Comm comm;
    MPI_Request req;

    /* intentionally not using MTest_Init/MTest_Finalize in order to make it
     * easy to take this test and use it as an NBC sanity test outside of the
     * MPICH test suite */
    MPI_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

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

    MPI_Ibarrier(comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Ibcast(sbuf, NUM_INTS, MPI_INT, 0, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Igather(sbuf, NUM_INTS, MPI_INT, rbuf, NUM_INTS, MPI_INT, 0, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    if (0 == rank)
        MPI_Igather(MPI_IN_PLACE, -1, MPI_DATATYPE_NULL, rbuf, NUM_INTS, MPI_INT, 0, comm, &req);
    else
        MPI_Igather(sbuf, NUM_INTS, MPI_INT, rbuf, NUM_INTS, MPI_INT, 0, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Igatherv(sbuf, NUM_INTS, MPI_INT, rbuf, rcounts, rdispls, MPI_INT, 0, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    if (0 == rank)
        MPI_Igatherv(MPI_IN_PLACE, -1, MPI_DATATYPE_NULL, rbuf, rcounts, rdispls, MPI_INT, 0, comm,
                     &req);
    else
        MPI_Igatherv(sbuf, NUM_INTS, MPI_INT, rbuf, rcounts, rdispls, MPI_INT, 0, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Iscatter(sbuf, NUM_INTS, MPI_INT, rbuf, NUM_INTS, MPI_INT, 0, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    if (0 == rank)
        MPI_Iscatter(sbuf, NUM_INTS, MPI_INT, MPI_IN_PLACE, -1, MPI_DATATYPE_NULL, 0, comm, &req);
    else
        MPI_Iscatter(sbuf, NUM_INTS, MPI_INT, rbuf, NUM_INTS, MPI_INT, 0, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Iscatterv(sbuf, scounts, sdispls, MPI_INT, rbuf, NUM_INTS, MPI_INT, 0, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    if (0 == rank)
        MPI_Iscatterv(sbuf, scounts, sdispls, MPI_INT, MPI_IN_PLACE, -1, MPI_DATATYPE_NULL, 0, comm,
                      &req);
    else
        MPI_Iscatterv(sbuf, scounts, sdispls, MPI_INT, rbuf, NUM_INTS, MPI_INT, 0, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Iallgather(sbuf, NUM_INTS, MPI_INT, rbuf, NUM_INTS, MPI_INT, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Iallgather(MPI_IN_PLACE, -1, MPI_DATATYPE_NULL, rbuf, NUM_INTS, MPI_INT, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Iallgatherv(sbuf, NUM_INTS, MPI_INT, rbuf, rcounts, rdispls, MPI_INT, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Iallgatherv(MPI_IN_PLACE, -1, MPI_DATATYPE_NULL, rbuf, rcounts, rdispls, MPI_INT, comm,
                    &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Ialltoall(sbuf, NUM_INTS, MPI_INT, rbuf, NUM_INTS, MPI_INT, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Ialltoall(MPI_IN_PLACE, -1, MPI_DATATYPE_NULL, rbuf, NUM_INTS, MPI_INT, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Ialltoallv(sbuf, scounts, sdispls, MPI_INT, rbuf, rcounts, rdispls, MPI_INT, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Ialltoallv(MPI_IN_PLACE, NULL, NULL, MPI_DATATYPE_NULL, rbuf, rcounts, rdispls, MPI_INT,
                   comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Ialltoallw(sbuf, scounts, sdispls, types, rbuf, rcounts, rdispls, types, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Ialltoallw(MPI_IN_PLACE, NULL, NULL, NULL, rbuf, rcounts, rdispls, types, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Ireduce(sbuf, rbuf, NUM_INTS, MPI_INT, MPI_SUM, 0, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    if (0 == rank)
        MPI_Ireduce(MPI_IN_PLACE, rbuf, NUM_INTS, MPI_INT, MPI_SUM, 0, comm, &req);
    else
        MPI_Ireduce(sbuf, rbuf, NUM_INTS, MPI_INT, MPI_SUM, 0, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Iallreduce(sbuf, rbuf, NUM_INTS, MPI_INT, MPI_SUM, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Iallreduce(MPI_IN_PLACE, rbuf, NUM_INTS, MPI_INT, MPI_SUM, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Ireduce_scatter(sbuf, rbuf, rcounts, MPI_INT, MPI_SUM, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Ireduce_scatter(MPI_IN_PLACE, rbuf, rcounts, MPI_INT, MPI_SUM, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Ireduce_scatter_block(sbuf, rbuf, NUM_INTS, MPI_INT, MPI_SUM, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Ireduce_scatter_block(MPI_IN_PLACE, rbuf, NUM_INTS, MPI_INT, MPI_SUM, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Iscan(sbuf, rbuf, NUM_INTS, MPI_INT, MPI_SUM, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Iscan(MPI_IN_PLACE, rbuf, NUM_INTS, MPI_INT, MPI_SUM, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Iexscan(sbuf, rbuf, NUM_INTS, MPI_INT, MPI_SUM, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    MPI_Iexscan(MPI_IN_PLACE, rbuf, NUM_INTS, MPI_INT, MPI_SUM, comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

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
