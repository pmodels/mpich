/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <mpi.h>
#include <string.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run errhan_errstring2
int run(const char *arg);
#endif

int run(const char *arg)
{
    int errorclass;
    char errorstring[MPI_MAX_ERROR_STRING] = { 64, 0 };
    int slen;
    int errs = 0;

    MPI_Add_error_class(&errorclass);
    MPI_Error_string(errorclass, errorstring, &slen);
    if (strncmp(errorstring, "", 1)) {
        fprintf(stderr, "errorclass:%d errorstring:'%s' len:%d\n", errorclass, errorstring, slen);
        errs++;
    }
    return errs;
}
