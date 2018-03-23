/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

static int verbose = 0;

/* tests */
int builtin_struct_test(void);

/* helper functions */
int parse_args(int argc, char **argv);

int main(int argc, char **argv)
{
    int err, errs = 0;

    MPI_Init(&argc, &argv);     /* MPI-1.2 doesn't allow for MPI_Init(0,0) */
    parse_args(argc, argv);

    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    /* perform some tests */
    err = builtin_struct_test();
    if (err && verbose)
        fprintf(stderr, "%d errors in builtin struct test.\n", err);
    errs += err;

    /* print message and exit */
    if (errs) {
        fprintf(stderr, "Found %d errors\n", errs);
    } else {
        printf(" No Errors\n");
    }
    MPI_Finalize();
    return 0;
}

/* builtin_struct_test()
 *
 * Tests behavior with a zero-count struct of builtins.
 *
 * Returns the number of errors encountered.
 */
int builtin_struct_test(void)
{
    int err, errs = 0;

    int count = 0;
    MPI_Datatype newtype;

    int size;
    MPI_Aint extent;

    err = MPI_Type_create_struct(count, (int *) 0, (MPI_Aint *) 0, (MPI_Datatype *) 0, &newtype);
    if (err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "error creating struct type in builtin_struct_test()\n");
        }
        errs++;
    }

    err = MPI_Type_size(newtype, &size);
    if (err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "error obtaining type size in builtin_struct_test()\n");
        }
        errs++;
    }

    if (size != 0) {
        if (verbose) {
            fprintf(stderr, "error: size != 0 in builtin_struct_test()\n");
        }
        errs++;
    }

    err = MPI_Type_extent(newtype, &extent);
    if (err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "error obtaining type extent in builtin_struct_test()\n");
        }
        errs++;
    }

    if (extent != 0) {
        if (verbose) {
            fprintf(stderr, "error: extent != 0 in builtin_struct_test()\n");
        }
        errs++;
    }

    MPI_Type_free(&newtype);

    return errs;
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
