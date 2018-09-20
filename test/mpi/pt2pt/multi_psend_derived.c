/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 *
 *  This program checks if MPICH can correctly handle multiple persistent
 *  communication calls with a derived datatype
 *
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

#define ITER 16

int main(int argc, char *argv[])
{
    int size, rank;
    int i;
    MPI_Request req;
    MPI_Datatype int_dup;
    int v = 1, errs = 0;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        fprintf(stderr, "Launch with two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 0) {
        MPI_Type_dup(MPI_INT, &int_dup);
        v = 42;
        MPI_Send_init(&v, 1, int_dup, 1, 0, MPI_COMM_WORLD, &req);
        MPI_Type_free(&int_dup);
        for (i = 0; i < ITER; i++) {
            MPI_Status s;
            MPI_Start(&req);
            MPI_Wait(&req, &s);
        }
        MPI_Request_free(&req);
    } else {
        for (i = 0; i < ITER; i++) {
            v = -1;
            MPI_Recv(&v, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (v != 42) {
                errs++;
                fprintf(stderr, "Psend-recv iteration %d: Expected 42 but got %d\n", i, v);
            }
        }
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
