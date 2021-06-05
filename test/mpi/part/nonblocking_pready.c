/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test to confirm MPI_PREADY does not block";
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
        MPI_Start(&request);
        MPI_Pready(0, request);
        MPI_Send(NULL, 0, MPI_DATATYPE_NULL, 1, 0, MPI_COMM_WORLD);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
    } else if (rank == 1) {
        MPI_Precv_init(&a, 1, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_INFO_NULL, &request);
        MPI_Recv(NULL, 0, MPI_DATATYPE_NULL, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Start(&request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
    }

    if (rank < 2) {
        MPI_Request_free(&request);
    }
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
