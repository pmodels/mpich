/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run init_version
int run(const char *arg);
#endif

int run(const char *arg)
{
    int errs = 0;
    int majversion, subversion;


    MPI_Get_version(&majversion, &subversion);
    if (majversion != MPI_VERSION) {
        errs++;
        printf("Major version is %d but is %d in the mpi.h file\n", majversion, MPI_VERSION);
    }
    if (subversion != MPI_SUBVERSION) {
        errs++;
        printf("Minor version is %d but is %d in the mpi.h file\n", subversion, MPI_SUBVERSION);
    }

    return errs;
}
