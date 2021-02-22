/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2019 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 *
 */

/* This is a very weak sanity test that persistent Reduce collectives specified by
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
    int *sbuf = NULL;
    int *rbuf = NULL;
    MPI_Comm comm;
    int count = 10;
    int j = 0;
    int root = 1;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    sbuf = malloc(NUM_INTS * sizeof(int));
    rbuf = malloc(NUM_INTS * sizeof(int));
    my_assert(sbuf);
    my_assert(rbuf);

    for (i = 0; i < NUM_INTS; ++i) {
        sbuf[i] = rank + i;
    }

    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Request req;

    MPI_Reduce_init(sbuf, rbuf, NUM_INTS, MPI_INT, MPI_SUM, root, comm, info, &req);

    for (i = 0; i < count; i++) {
        MPI_Start(&req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        /* Check the results */
        if (rank == root) {
            for (j = 0; j < NUM_INTS; j++) {
                int result = j * size + (size * (size - 1)) / 2;
                if (rbuf[j] != result) {
                    errs++;
                    if (errs < 10) {
                        fprintf(stderr, "rbuf[%d] = %d expected %d\n", j, rbuf[j], result);
                    }
                }
            }
        }
    }
    MPI_Request_free(&req);
    MPI_Info_free(&info);

    free(sbuf);
    free(rbuf);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
