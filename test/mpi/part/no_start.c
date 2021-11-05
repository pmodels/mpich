/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test for a partitioned request that is never started";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    MTest_Init(&argc, &argv);

    int size;
    int rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size < 2) {
        fprintf(stderr, "This test requires at least two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int a;
    MPI_Request request;
    if (rank == 0) {
        MPI_Psend_init(&a, 1, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_INFO_NULL, &request);
    } else if (rank == 1) {
        MPI_Precv_init(&a, 1, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_INFO_NULL, &request);
    }

    if (rank < 2) {
        MPI_Request_free(&request);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
