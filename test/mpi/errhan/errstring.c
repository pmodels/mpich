/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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
    return 0;
}
