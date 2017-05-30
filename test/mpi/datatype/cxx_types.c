/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test checks for the existence of four new C++ named predefined datatypes
 * that should be accessible from C (and Fortran, not tested here). */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "mpitest.h"

/* assert-like macro that bumps the err count and emits a message */
#define check(x_)                                                                 \
    do {                                                                          \
        if (!(x_)) {                                                              \
            ++errs;                                                               \
            if (errs < 10) {                                                      \
                fprintf(stderr, "check failed: (%s), line %d\n", #x_, __LINE__); \
            }                                                                     \
        }                                                                         \
    } while (0)

int main(int argc, char *argv[])
{
    int errs = 0;
    int wrank, wsize;
    int size;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

    /* perhaps the MPI library has no CXX support, in which case let's assume
     * that these constants exist and were set to MPI_DATATYPE_NULL (standard
     * MPICH behavior). */
#define check_type(type_)                 \
    do {                                  \
        size = -1;                        \
        if (type_ != MPI_DATATYPE_NULL) { \
            MPI_Type_size(type_, &size);  \
            check(size > 0);              \
        }                                 \
    } while (0)

    check_type(MPI_CXX_BOOL);
    check_type(MPI_CXX_FLOAT_COMPLEX);
    check_type(MPI_CXX_DOUBLE_COMPLEX);
    check_type(MPI_CXX_LONG_DOUBLE_COMPLEX);

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
