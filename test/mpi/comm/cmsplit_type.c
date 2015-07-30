/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
/* USE_STRICT_MPI may be defined in mpitestconf.h */
#include "mpitestconf.h"
#include <stdio.h>
#include <stdlib.h>

/* FIXME: This test only checks that the MPI_Comm_split_type routine
   doesn't fail.  It does not check for correct behavior */

int main(int argc, char *argv[])
{
    int rank, size, verbose = 0;
    int wrank;
    MPI_Comm comm;

    MPI_Init(&argc, &argv);

    if (getenv("MPITEST_VERBOSE"))
        verbose = 1;

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    /* Check to see if MPI_COMM_TYPE_SHARED works correctly */
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &comm);
    if (comm == MPI_COMM_NULL)
        printf("Expected a non-null communicator, but got MPI_COMM_NULL\n");
    else {
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        if (rank == 0 && verbose)
            printf("Created subcommunicator of size %d\n", size);
        MPI_Comm_free(&comm);
    }

    /* Check to see if MPI_UNDEFINED is respected */
    MPI_Comm_split_type(MPI_COMM_WORLD, (wrank % 2 == 0) ? MPI_COMM_TYPE_SHARED : MPI_UNDEFINED,
                        0, MPI_INFO_NULL, &comm);
    if ((wrank % 2) && (comm != MPI_COMM_NULL))
        printf("Expected MPI_COMM_NULL, but did not get one\n");
    if (wrank % 2 == 0) {
        if (comm == MPI_COMM_NULL)
            printf("Expected a non-null communicator, but got MPI_COMM_NULL\n");
        else {
            MPI_Comm_rank(comm, &rank);
            MPI_Comm_size(comm, &size);
            if (rank == 0 && verbose)
                printf("Created subcommunicator of size %d\n", size);
            MPI_Comm_free(&comm);
        }
    }

    /* Use wrank because Comm_split_type may return more than one communicator
     * across the job, and if so, each will have a rank 0 entry.  Test
     * output rules are for a single process to write the successful
     * test (No Errors) output. */
    if (wrank == 0)
        printf(" No errors\n");

    MPI_Finalize();

    return 0;
}
