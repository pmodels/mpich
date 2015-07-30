/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int errs = 0;
    int newrank, merr, rank;
    int index[2], edges[2];

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    /* Graph map where there are no nodes for this process */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    /* Here is a singleton graph, containing only the root process */
    index[0] = 0;
    edges[0] = 0;
    merr = MPI_Graph_map(MPI_COMM_WORLD, 1, index, edges, &newrank);
    if (merr) {
        errs++;
        printf("Graph map returned an error\n");
        MTestPrintError(merr);
    }
    if (rank != 0 && newrank != MPI_UNDEFINED) {
        errs++;
        printf("Graph map with no local nodes did not return MPI_UNDEFINED\n");
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;

}
