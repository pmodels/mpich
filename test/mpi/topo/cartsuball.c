/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int errs = 0;
    int size, dims[2], periods[2], remain[2];
    int result, rank;
    MPI_Comm comm, newcomm;

    MTest_Init(&argc, &argv);

    /* First, create a 1-dim cartesian communicator */
    periods[0] = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    dims[0] = size;
    MPI_Cart_create(MPI_COMM_WORLD, 1, dims, periods, 0, &comm);

    /* Now, extract a communicator with no dimensions */
    remain[0] = 0;
    MPI_Cart_sub(comm, remain, &newcomm);

    MPI_Comm_rank(comm, &rank);

    if (rank == 0) {
        /* This should be congruent to MPI_COMM_SELF */
        MPI_Comm_compare(MPI_COMM_SELF, newcomm, &result);
        if (result != MPI_CONGRUENT) {
            errs++;
            printf("cart sub to size 0 did not give self\n");
        }
        MPI_Comm_free(&newcomm);
    } else {
        /* Previously an MPICH developer argue that all other ranks should
         * return MPI_COMM_NULL and implemented this way. Open MPI argues
         * that other ranks should also return a self-equivalent comm since
         * MPI_Cart_sub is a split + zero-dimensional topology. I (hzhou)
         * kinda agree with the latter but couldn't care less. If application
         * ask for zero-dimensional topology, it must be unexpected and they
         * should fix their code!
         *
         * I am removing that part of test so we don't have to spend time debating
         * useless points.
         */
        if (newcomm != MPI_COMM_NULL) {
            MPI_Comm_free(&newcomm);
        }
    }

    /* Free the new communicator so that storage leak tests will
     * be happy */
    MPI_Comm_free(&comm);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
