/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Tests that commit of a couple of basic types succeeds. */

#include "mpi.h"
#include <stdio.h>
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

static int verbose = 0;

int parse_args(int argc, char **argv);

int main(int argc, char **argv)
{
    int mpi_err, errs = 0;
    MPI_Datatype type;

    MPI_Init(&argc, &argv);
    parse_args(argc, argv);

    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    type = MPI_INT;
    mpi_err = MPI_Type_commit(&type);
    if (mpi_err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "MPI_Type_commit of MPI_INT failed.\n");
        }
        errs++;
    }

    type = MPI_FLOAT_INT;
    mpi_err = MPI_Type_commit(&type);
    if (mpi_err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "MPI_Type_commit of MPI_FLOAT_INT failed.\n");
        }
        errs++;
    }

    /* print message and exit */
    if (errs) {
        fprintf(stderr, "Found %d errors\n", errs);
    }
    else {
        printf(" No Errors\n");
    }
    MPI_Finalize();
    return 0;
}

int parse_args(int argc, char **argv)
{
    /*
     * int ret;
     *
     * while ((ret = getopt(argc, argv, "v")) >= 0)
     * {
     * switch (ret) {
     * case 'v':
     * verbose = 1;
     * break;
     * }
     * }
     */
    if (argc > 1 && strcmp(argv[1], "-v") == 0)
        verbose = 1;
    return 0;
}
