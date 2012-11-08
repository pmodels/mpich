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

    MPI_Init(&argc, &argv);
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

    MPI_Reduce((wrank == 0 ? MPI_IN_PLACE : &errs), &errs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (wrank == 0) {
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
