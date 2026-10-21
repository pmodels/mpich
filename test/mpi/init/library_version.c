/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run init_library_version
int run(const char *arg);
#endif

static int verbose = 0;

int run(const char *arg)
{
    int errs = 0, resultlen = -1;
    char version[MPI_MAX_LIBRARY_VERSION_STRING];


    MPI_Get_library_version(version, &resultlen);
    if (resultlen < 0) {
        errs++;
        printf("Resultlen is %d\n", resultlen);
    } else {
        if (verbose)
            printf("%s\n", version);
    }

    return errs;
}
