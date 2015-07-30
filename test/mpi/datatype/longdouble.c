/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

/* Some MPI implementations should not support MPI_LONG_DOUBLE because it has
 * different representations/sizes among several concurrently supported
 * compilers.  For example, a 16-byte GCC implementation and an 8-byte Cray
 * compiler implementation.
 *
 * This test ensures that simplistic build logic/configuration did not result in
 * a defined, yet incorrectly sized, MPI predefined datatype for long double and
 * long double _Complex.  See tt#1671 for more info.
 *
 * Based on a test suggested by Jim Hoekstra @ Iowa State University. */

int main(int argc, char *argv[])
{
    int rank, size, i, type_size;
    int errs = 0;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
#ifdef HAVE_LONG_DOUBLE
        if (MPI_LONG_DOUBLE != MPI_DATATYPE_NULL) {
            MPI_Type_size(MPI_LONG_DOUBLE, &type_size);
            if (type_size != sizeof(long double)) {
                printf("type_size != sizeof(long double) : (%d != %zd)\n",
                       type_size, sizeof(long double));
                ++errs;
            }
        }
#endif
#if defined(HAVE_LONG_DOUBLE__COMPLEX) && defined(USE_LONG_DOUBLE_COMPLEX)
        if (MPI_C_LONG_DOUBLE_COMPLEX != MPI_DATATYPE_NULL) {
            MPI_Type_size(MPI_C_LONG_DOUBLE_COMPLEX, &type_size);
            if (type_size != sizeof(long double _Complex)) {
                printf("type_size != sizeof(long double _Complex) : (%d != %zd)\n",
                       type_size, sizeof(long double _Complex));
                ++errs;
            }
        }
#endif
        if (errs) {
            printf("found %d errors\n", errs);
        }
        else {
            printf(" No errors\n");
        }
    }

    MPI_Finalize();
    return 0;
}
