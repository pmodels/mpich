/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"
#include "mpicolltest.h"
/* Very simple test that Allreduce detects invalid (datatype,operation)
   pair */

int verbose = 0;

int main(int argc, char *argv[])
{
    int a, b, ierr, errs = 0;
    char str[MPI_MAX_ERROR_STRING + 1];
    int slen;

    MTest_Init(&argc, &argv);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    ierr = MTest_Reduce(&a, &b, 1, MPI_BYTE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (ierr == MPI_SUCCESS) {
        errs++;
        printf("Did not detect invalid type/op pair (byte,max) in Allreduce\n");
    } else {
        if (verbose) {
            MPI_Error_string(ierr, str, &slen);
            printf("Found expected error; message is: %s\n", str);
        }
    }

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_ARE_FATAL);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
