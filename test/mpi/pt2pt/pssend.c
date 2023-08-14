/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * This program checks the semantics of persistent synchronous send
 */

#include "mpitest.h"

int main(int argc, char *argv[])
{
    int errs = 0;
    int size, rank;
    MPI_Request req;
    int flag;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        fprintf(stderr, "Launch with two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 0) {
        MPI_Ssend_init(NULL, 0, MPI_DATATYPE_NULL, 1, 0, MPI_COMM_WORLD, &req);
        MPI_Start(&req);

        /* ssend cannot be complete at this point */
        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        if (flag != 0) {
            errs++;
            fprintf(stderr, "ERROR: synchronous send completed before matching recv was posted\n");
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        MPI_Request_free(&req);
    } else if (rank == 1) {
        MPI_Recv(NULL, 0, MPI_DATATYPE_NULL, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    MTest_Finalize(errs);

    return 0;
}
