/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
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
