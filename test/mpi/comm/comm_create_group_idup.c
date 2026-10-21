/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <string.h>
#include <assert.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run comm_comm_create_group_idup
int run(const char *arg);
#endif

int run(const char *arg)
{
    int size, rank;
    MPI_Group world_group;
    MPI_Comm group_comm, idup_comm;
    MPI_Request req;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size % 2) {
        fprintf(stderr, "this program requires even number of processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Create some groups */
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);

    if (rank % 2 == 0) {
        MPI_Comm_create_group(MPI_COMM_WORLD, world_group, 0, &group_comm);
        MPI_Comm_idup(MPI_COMM_WORLD, &idup_comm, &req);
    } else {
        MPI_Comm_idup(MPI_COMM_WORLD, &idup_comm, &req);
        MPI_Comm_create_group(MPI_COMM_WORLD, world_group, 0, &group_comm);
    }

    MPI_Wait(&req, MPI_STATUSES_IGNORE);
    /*Test new comm with a barrier */
    MPI_Barrier(idup_comm);
    MPI_Barrier(group_comm);

    MPI_Group_free(&world_group);
    MPI_Comm_free(&idup_comm);
    MPI_Comm_free(&group_comm);

    return 0;
}
