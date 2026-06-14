/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpi.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    char msg[MPI_MAX_ERROR_STRING + 1];
    int i, len;

    MPI_Init(0, 0);
    for (i = 0; i < 54; i++) {
        MPI_Error_string(i, msg, &len);
        printf("msg for %d is %s\n", i, msg);
    }

    MPI_Finalize();
    return MTestReturnValue(errs);
}
