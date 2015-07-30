/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "stdio.h"
#include "mpi.h"

int main(int argc, char *argv[])
{
    int provided, rank;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Finalize();

    /* The test here is a simple one that Finalize exits, so the only action
     * is to write no errors */
    if (rank == 0) {
        printf(" No Errors\n");
    }

    return 0;

}
