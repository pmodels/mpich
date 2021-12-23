/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <string.h>

#ifdef MULTI_TESTS
#define run pt2pt_waitany_null
int run(const char *arg);
#endif

int run(const char *arg)
{
    int i, err, errs = 0, rank;

    int index;
    MPI_Request requests[10];
    MPI_Status statuses[10];

    for (i = 0; i < 10; i++) {
        requests[i] = MPI_REQUEST_NULL;
    }

    /* begin testing */
    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    err = MPI_Waitany(10, requests, &index, statuses);

    if (err != MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "MPI_Waitany did not return MPI_SUCCESS\n");
    }

    if (index != MPI_UNDEFINED) {
        errs++;
        fprintf(stderr, "MPI_Waitany did not set index to MPI_UNDEFINED\n");
    }

    /* end testing */

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_ARE_FATAL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    return errs;
}
