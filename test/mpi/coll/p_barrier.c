/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <unistd.h>

#ifdef MULTI_TESTS
#define run coll_p_barrier
int run(const char *arg);
#endif

int run(const char *arg)
{
    MPI_Request req;
    MPI_Info info;
    int rank, i;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Info_create(&info);
    MPI_Barrier_init(MPI_COMM_WORLD, info, &req);
    for (i = 0; i < 10; ++i) {
        MPI_Start(&req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }
    MPI_Request_free(&req);
    MPI_Info_free(&info);

    return 0;
}
