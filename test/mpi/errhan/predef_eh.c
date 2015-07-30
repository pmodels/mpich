/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "mpi.h"

/* Ensure that setting a user-defined error handler on predefined
 * communicators does not cause a problem at finalize time.  Regression
 * test for ticket #1591 */

void errf(MPI_Comm * comm, int *ec)
{
    /* do nothing */
}

int main(int argc, char **argv)
{
    MPI_Errhandler errh;
    int wrank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_create_errhandler((MPI_Comm_errhandler_function *) errf, &errh);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, errh);
    MPI_Comm_set_errhandler(MPI_COMM_SELF, errh);
    MPI_Errhandler_free(&errh);
    MPI_Finalize();
    /* Test harness requirement is that only one process write No Errors */
    if (wrank == 0)
        printf(" No Errors\n");
    return 0;
}
