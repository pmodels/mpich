/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2019 Intel Corporation.  Intel provides this material
 *  Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* This is a very weak sanity test that persistent Alltoall collectives specified by
 * proposed MPI-4 is present in the library and take arguments as expected.  This test
 * does not check for progress or matching issues. */

#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>

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
    int iters = 10;
    int j = 0;
    int count = 1;              /* sendcount and recvcount. */

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    sbuf = malloc(size * count * sizeof(int));
    rbuf = malloc(size * count * sizeof(int));
    my_assert(sbuf);
    my_assert(rbuf);

    for (i = 0; i < (count * size); ++i) {
        sbuf[i] = (rank * size) + i;
    }
    for (i = 0; i < (size * count); ++i) {
        rbuf[i] = 0;
    }

    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Request req;

    MPI_Alltoall_init(sbuf, count, MPI_INT, rbuf, count, MPI_INT, comm, info, &req);

    for (i = 0; i < iters; i++) {
        MPI_Start(&req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        /* Check the results */
        for (j = 0; j < (size * count); j++) {
            int result = (size * j) + rank;
            if (rbuf[j] != result) {
                errs++;
                if (errs < 10) {
                    fprintf(stderr, "rbuf[%d] = %d expected %d\n", j, rbuf[j], result);
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
