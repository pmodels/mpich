/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <mpi.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int errorclass;
    char errorstring[MPI_MAX_ERROR_STRING] = { 64, 0 };
    int slen;
    MPI_Init(&argc, &argv);
    MPI_Add_error_class(&errorclass);
    MPI_Error_string(errorclass, errorstring, &slen);
    if (strncmp(errorstring, "", 1)) {
        fprintf(stderr, "errorclass:%d errorstring:'%s' len:%d\n", errorclass, errorstring, slen);
    }
    else {
        printf(" No Errors\n");
    }
    MPI_Finalize();
    return 0;
}
