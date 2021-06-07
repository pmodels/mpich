/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2019 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* This is a very weak sanity test that persistenr broadcast collectives specified by
 * proposed MPI-4 is present in the library and take arguments as expected.  This test
 * does not check for progress or matching issues. */

#include "mpi.h"
#include "mpitest.h"
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
    int *sendbuf = NULL;
    int *recvbuf = NULL;
    int *recvcounts = NULL;
    int *displs = NULL;
    MPI_Comm comm;
    int count = 10;
    int j = 0;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    recvcounts = malloc(size * sizeof(int));
    displs = malloc(size * sizeof(int));
    sendbuf = malloc(size * sizeof(int));
    recvbuf = malloc(size * NUM_INTS * sizeof(int));
    my_assert(recvcounts);
    my_assert(displs);
    my_assert(sendbuf);
    my_assert(recvbuf);

    for (i = 0; i < size; i++) {
        recvcounts[i] = NUM_INTS;
        displs[i] = i * recvcounts[i];
    }

    for (i = 0; i < NUM_INTS; i++) {
        sendbuf[i] = rank * NUM_INTS + i;
    }

    for (i = 0; i < (size * NUM_INTS); i++) {
        recvbuf[i] = -1;
    }

    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Request req;

    MPI_Allgatherv_init(sendbuf, NUM_INTS, MPI_INT, recvbuf, recvcounts, displs, MPI_INT, comm,
                        info, &req);

    for (i = 0; i < count; i++) {
        MPI_Start(&req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        /* Check the results */
        for (j = 0; j < (size * NUM_INTS); j++) {
            if (recvbuf[j] != j) {
                errs++;
                if (errs < 10) {
                    fprintf(stderr, "rbuf[%d] = %d expected %d\n", j, recvbuf[j], j);
                }
            }
        }
    }

    MPI_Request_free(&req);

    MPI_Info_free(&info);

    free(sendbuf);
    free(recvbuf);
    free(recvcounts);
    free(displs);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
