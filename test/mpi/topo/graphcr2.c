/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Create a communicator with a graph that contains null edges and one that contains duplicate edges";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    int *index = 0, *edges = 0;
    int rank, size, i, j, crank, csize;
    MPI_Comm comm;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    index = (int *) malloc(size * sizeof(int));
    edges = (int *) malloc(size * sizeof(int));
    for (i = 0; i < size; i++) {
        index[i] = 1;
        edges[i] = i;
    }
    /* As of MPI 2.1, self edges are permitted */
    MPI_Graph_create(MPI_COMM_WORLD, size, index, edges, 0, &comm);
    MPI_Comm_rank(comm, &crank);
    MPI_Comm_size(comm, &csize);
    if (csize != size) {
        errs++;
        fprintf(stderr, "Graph create with self links has size %d should be %d", csize, size);
    }
    free(index);
    free(edges);
    MPI_Comm_free(&comm);

    /* Create a graph with duplicate links */
    index = (int *) malloc(size * sizeof(int));
    edges = (int *) malloc(size * 2 * sizeof(int));
    j = 0;
    for (i = 0; i < size; i++) {
        index[i] = j + 2;
        edges[j++] = (i + 1) % size;
        edges[j++] = (i + 1) % size;
    }
    /* As of MPI 2.1, duplicate edges are permitted */
    MPI_Graph_create(MPI_COMM_WORLD, size, index, edges, 0, &comm);
    MPI_Comm_rank(comm, &crank);
    MPI_Comm_size(comm, &csize);
    if (csize != size) {
        errs++;
        fprintf(stderr, "Graph create with duplicate links has size %d should be %d", csize, size);
    }
    free(index);
    free(edges);
    MPI_Comm_free(&comm);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
