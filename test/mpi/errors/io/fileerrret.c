/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitestconf.h"
#include <stdio.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "mpitest.h"

static int ncalls = 0;
void efn(MPI_File * fh, int *code, ...);
void efn(MPI_File * fh, int *code, ...)
{
    ncalls++;
    *code = MPI_SUCCESS;
}

int main(int argc, char *argv[])
{
    MPI_File fh;
    MPI_Errhandler eh;
    char filename[10];
    int errs = 0, rc;

    MTest_Init(&argc, &argv);

    /* Test that the default error handler is errors return for files */
    strncpy(filename, "t1", sizeof(filename));

    rc = MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDWR, MPI_INFO_NULL, &fh);
    if (!rc) {
        errs++;
        printf("Did not get error from open for writing without CREATE\n");
    }

    /* Test that we can change the default error handler by changing
     * the error handler on MPI_FILE_NULL. */
    MPI_File_create_errhandler(efn, &eh);
    MPI_File_set_errhandler(MPI_FILE_NULL, eh);
    MPI_Errhandler_free(&eh);

    rc = MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDWR, MPI_INFO_NULL, &fh);
    if (rc) {
        errs++;
        printf("Returned error from open (should have called error handler instead)\n");
    }
    if (ncalls != 1) {
        errs++;
        printf
            ("Did not invoke error handler when opening a non-existent file for writing and reading (without MODE_CREATE)\n");
    }

    /* Find out how many errors we saw */
    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
