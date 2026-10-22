/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run errhan_dynamic_errcode_predefined_errclass
int run(const char *arg);
#endif

int run(const char *arg)
{
    int errcode, errclass, errs = 0;


    MPI_Add_error_code(MPI_ERR_ARG, &errcode);
    MPI_Error_class(errcode, &errclass);

    if (errclass != MPI_ERR_ARG) {
        printf("ERROR: Got 0x%x, expected 0x%x\n", errclass, MPI_ERR_ARG);
        errs++;
    }

    return errs;
}
