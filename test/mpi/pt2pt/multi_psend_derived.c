/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * This program checks if MPICH can correctly handle multiple
 * persistent communication calls with a derived datatype
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

    if (rank == 0)
        MTestPrintfMsg(1, "Test 1: Persistent send - Recv\n");

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

    if (rank == 0)
        MTestPrintfMsg(1, "Test 2: Send - Persistent recv (with ANY_SOURCE)\n");

    if (rank == 0) {
        for (i = 0; i < ITER; i++) {
            v = i;
            MPI_Send(&v, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        }
    } else {
        MPI_Type_dup(MPI_INT, &int_dup);
        MPI_Recv_init(&v, 1, int_dup, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &req);
        MPI_Type_free(&int_dup);
        for (i = 0; i < ITER; i++) {
            MPI_Status s;
            v = -1;
            MPI_Start(&req);
            MPI_Wait(&req, &s);
            if (v != i) {
                errs++;
                fprintf(stderr, "Send-precv(anysrc) iteration %d: Expected %d but got %d\n",
                        i, i, v);
            }
        }
        MPI_Request_free(&req);
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
