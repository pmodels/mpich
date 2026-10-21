/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run topo_graphcr
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Create a communicator with a graph that contains no processes";
*/

int run(const char *arg)
{
    int errs = 0;
    int *index = 0, *edges = 0;
    MPI_Comm comm;


    /* MPI 2.1, page 246, lines 29-30 make it clear that this is a valid
     * (not erroneous) call that must return MPI_COMM_NULL */
    MPI_Graph_create(MPI_COMM_WORLD, 0, index, edges, 0, &comm);
    if (comm != MPI_COMM_NULL) {
        errs++;
        fprintf(stderr, "Expected MPI_COMM_NULL from empty graph create\n");
    }

    return errs;
}
