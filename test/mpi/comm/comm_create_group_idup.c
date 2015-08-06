/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitestconf.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[])
{
    int size, rank;
    MPI_Group world_group;
    MPI_Comm group_comm, idup_comm;
    MPI_Request req;
    MPI_Init(&argc, &argv);

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
    }
    else {
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
    if (rank == 0)
        printf(" No errors\n");

    MPI_Finalize();
    return 0;
}
