/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <stdio.h>

/* FIXME: This behavior of this test is implementation specific. */

static int verbose = 0;

int main(int argc, char **argv)
{
    int MY_ERROR_CLASS;
    int MY_ERROR_CODE;
    char MY_ERROR_STRING[10];

    sprintf(MY_ERROR_STRING, "MY ERROR");

    MPI_Init(&argc, &argv);

    if (verbose)
        printf("Adding My Error Class\n");
    MPI_Add_error_class(&MY_ERROR_CLASS);
    if (verbose)
        printf("Adding My Error Code\n");
    MPI_Add_error_code(MY_ERROR_CLASS, &MY_ERROR_CODE);
    if (verbose)
        printf("Adding My Error String\n");
    MPI_Add_error_string(MY_ERROR_CODE, MY_ERROR_STRING);

    if (verbose)
        printf("Calling Error Handler\n");
    MPI_Comm_call_errhandler(MPI_COMM_WORLD, MY_ERROR_CODE);

    /* We should not get here, because the default error handler
     * is ERRORS_ARE_FATAL.  This makes sure that the correct error
     * handler is called and that no failure occured (such as
     * a SEGV) in Comm_call_errhandler on the default
     * error handler. */
    printf("After the Error Handler Has Been Called\n");

    MPI_Finalize();
    return 0;
}
