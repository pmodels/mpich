/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"

int main(int argc, char **argv)
{
    int errcode, errclass, errs = 0;

    MTest_Init(&argc, &argv);

    MPI_Add_error_code(MPI_ERR_ARG, &errcode);
    MPI_Error_class(errcode, &errclass);

    if (errclass != MPI_ERR_ARG) {
        printf("ERROR: Got 0x%x, expected 0x%x\n", errclass, MPI_ERR_ARG);
        errs++;
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
