/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

static int verbose = 0;

int main(int argc, char *argv[])
{
    int errs = 0, resultlen = -1;
    char version[MPI_MAX_LIBRARY_VERSION_STRING];

    MTest_Init(&argc, &argv);

    MPI_Get_library_version(version, &resultlen);
    if (resultlen < 0) {
        errs++;
        printf("Resultlen is %d\n", resultlen);
    }
    else {
        if (verbose)
            printf("%s\n", version);
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;

}
