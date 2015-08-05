/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
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
    int size, rank, i, *excl;
    MPI_Group world_group, even_group;
    MPI_Comm even_comm;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size % 2) {
        fprintf(stderr, "this program requires a multiple of 2 number of processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    excl = malloc((size / 2) * sizeof(int));
    assert(excl);

    /* exclude the odd ranks */
    for (i = 0; i < size / 2; i++)
        excl[i] = (2 * i) + 1;

    /* Create some groups */
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    MPI_Group_excl(world_group, size / 2, excl, &even_group);
    MPI_Group_free(&world_group);

    if (rank % 2 == 0) {
        /* Even processes create a group for themselves */
        MPI_Comm_create_group(MPI_COMM_WORLD, even_group, 0, &even_comm);
        MPI_Barrier(even_comm);
        MPI_Comm_free(&even_comm);
    }

    MPI_Group_free(&even_group);
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0)
        printf(" No errors\n");

    MPI_Finalize();
    return 0;
}
