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
    int rank, size, verbose = 0, errs=0, tot_errs=0;
    int wrank;
    MPI_Comm comm;
    MPI_Info info;

    MPI_Init(&argc, &argv);

    if (getenv("MPITEST_VERBOSE"))
        verbose = 1;

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    /* Check to see if MPI_COMM_TYPE_SHARED works correctly */
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &comm);
    if (comm == MPI_COMM_NULL) {
        printf("Expected a non-null communicator, but got MPI_COMM_NULL\n");
	errs++;
    }
    else {
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        if (rank == 0 && verbose)
            printf("Created shared subcommunicator of size %d\n", size);
        MPI_Comm_free(&comm);
    }

#ifdef MPIX_COMM_TYPE_NEIGHBORHOOD
    /* the MPICH-specific MPIX_COMM_TYPE_NEIGHBORHOOD*/
    /* test #1: expected behavior -- user provided a directory, and we
     * determine which processes share access to it */
    MPI_Info_create(&info);
    if (argc == 2)
	    MPI_Info_set(info, "nbhd_common_dirname", argv[1]);
    else
	MPI_Info_set(info, "nbhd_common_dirname", ".");
    MPI_Comm_split_type(MPI_COMM_WORLD, MPIX_COMM_TYPE_NEIGHBORHOOD, 0,
	    info, &comm);
    if (comm == MPI_COMM_NULL) {
        printf("Expected a non-null communicator, but got MPI_COMM_NULL\n");
	errs++;
    }
    else {
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        if (rank == 0 && verbose)
            printf("Correctly created common-file subcommunicator of size %d\n", size);
        MPI_Comm_free(&comm);
    }

    /* test #2: a hint we don't know about */
    MPI_Info_delete(info, "nbhd_common_dirname");
    MPI_Info_set(info, "mpix_tooth_fairy", "enable");
    MPI_Comm_split_type(MPI_COMM_WORLD, MPIX_COMM_TYPE_NEIGHBORHOOD, 0,
	    info, &comm);
    if (comm != MPI_COMM_NULL) {
        printf("Expected a NULL communicator, but got something else\n");
	errs++;
        MPI_Comm_free(&comm);
    }
    else {
        if (rank == 0 && verbose)
            printf("Unknown hint correctly resulted in NULL communicator\n");
    }


    MPI_Info_free(&info);
#endif

    /* Check to see if MPI_UNDEFINED is respected */
    MPI_Comm_split_type(MPI_COMM_WORLD, (wrank % 2 == 0) ? MPI_COMM_TYPE_SHARED : MPI_UNDEFINED,
                        0, MPI_INFO_NULL, &comm);
    if ((wrank % 2) && (comm != MPI_COMM_NULL)) {
        printf("Expected MPI_COMM_NULL, but did not get one\n");
	errs++;
    }
    if (wrank % 2 == 0) {
        if (comm == MPI_COMM_NULL) {
            printf("Expected a non-null communicator, but got MPI_COMM_NULL\n");
	    errs++;
	}
        else {
            MPI_Comm_rank(comm, &rank);
            MPI_Comm_size(comm, &size);
            if (rank == 0 && verbose)
                printf("Created shared subcommunicator of size %d\n", size);
            MPI_Comm_free(&comm);
        }
    }
    MPI_Reduce(&errs, &tot_errs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    /* Use wrank because Comm_split_type may return more than one communicator
     * across the job, and if so, each will have a rank 0 entry.  Test
     * output rules are for a single process to write the successful
     * test (No Errors) output. */
    if (wrank == 0 && errs == 0)
        printf(" No errors\n");

    MPI_Finalize();

    return 0;
}
