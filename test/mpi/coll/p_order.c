/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test that persistent collectives can be started out of matching order";
*/

int main(int argc, char **argv)
{
    int errs = 0;

    MTest_Init(&argc, &argv);

    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size < 2) {
        fprintf(stderr, "This test requires at least two processes.");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int a = -1;
    MPI_Request requests[2];
    MPI_Bcast_init(&a, 1, MPI_INT, 0, MPI_COMM_WORLD, MPI_INFO_NULL, &requests[0]);
    MPI_Barrier_init(MPI_COMM_WORLD, MPI_INFO_NULL, &requests[1]);

    if (rank == 0) {
        a = 42;
        MPI_Start(&requests[0]);
        MPI_Start(&requests[1]);
    } else {
        MPI_Start(&requests[1]);
        MPI_Start(&requests[0]);
    }

    MPI_Waitall(2, requests, MPI_STATUSES_IGNORE);

    if (a != 42) {
        errs++;
    }

    MPI_Request_free(&requests[0]);
    MPI_Request_free(&requests[1]);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
