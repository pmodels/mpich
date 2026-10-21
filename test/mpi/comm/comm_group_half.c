/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitestconf.h"
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run comm_comm_group_half
int run(const char *arg);
#endif

int run(const char *arg)
{
    int rank, size;
    MPI_Group full_group, half_group;
    int range[1][3];
    MPI_Comm comm;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Comm_group(MPI_COMM_WORLD, &full_group);
    range[0][0] = 0;
    range[0][1] = size / 2;
    range[0][2] = 1;
    MPI_Group_range_incl(full_group, 1, range, &half_group);

    if (rank <= size / 2) {
        MPI_Comm_create_group(MPI_COMM_WORLD, half_group, 0, &comm);
        MPI_Barrier(comm);
        MPI_Comm_free(&comm);
    }

    MPI_Group_free(&half_group);
    MPI_Group_free(&full_group);

    return 0;
}
