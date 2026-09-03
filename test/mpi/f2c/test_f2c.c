/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

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


int main(int argc, char **argv)
{
    int errs = 0;
    MTest_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* simple test to ensure that c2f/f2c routines are present (initially missed
     * in MPICH impl) */
    {
        MPI_Message msg;
        MPI_Fint f_handle = 0xdeadbeef;
        f_handle = MPI_Message_c2f(MPI_MESSAGE_NULL);
        msg = MPI_Message_f2c(f_handle);
        check(f_handle != 0xdeadbeef);
        check(msg == MPI_MESSAGE_NULL);

        /* PMPI_ versions should also exists */
        f_handle = 0xdeadbeef;
        f_handle = PMPI_Message_c2f(MPI_MESSAGE_NULL);
        msg = PMPI_Message_f2c(f_handle);
        check(f_handle != 0xdeadbeef);
        check(msg == MPI_MESSAGE_NULL);
    }

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
