/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
/* USE_STRICT_MPI may be defined in mpitestconf.h */
#include "mpitestconf.h"
#include <stdio.h>
#include <stdlib.h>

/* FIXME: This test only checks that the MPIX_Comm_split_type routine
   doesn't fail.  It does not check for correct behavior */

int main(int argc, char *argv[])
{
    int rank, size, verbose=0;
    int wrank;
    MPI_Comm comm;

    MPI_Init(&argc, &argv);

    if (getenv("MPITEST_VERBOSE"))
        verbose = 1;

    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );
#if !defined(USE_STRICT_MPI) && defined(MPICH2)
    MPIX_Comm_split_type(MPI_COMM_WORLD, MPIX_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &comm);
#else
    MPI_Comm_split(MPI_COMM_WORLD, 0, 0, &comm);
#endif /* USE_STRICT_MPI */

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    if (rank == 0 && verbose)
        printf("Created subcommunicator of size %d\n", size);

    MPI_Comm_free(&comm);

    /* Use wrank because Comm_split_type may return more than one communicator
       across the job, and if so, each will have a rank 0 entry.  Test 
       output rules are for a single process to write the successful 
       test (No Errors) output. */
    if (wrank == 0)
        printf(" No errors\n");

    MPI_Finalize();

    return 0;
}
