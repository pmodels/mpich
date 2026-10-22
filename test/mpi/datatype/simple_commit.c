/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Tests that commit of a couple of basic types succeeds. */

#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run datatype_simple_commit
int run(const char *arg);
#endif

static int verbose = 0;

static int parse_args(const char *arg)
{
    MTestArgList *head = MTestArgListCreate_arg(arg);
    verbose = MTestArgListGetInt_with_default(head, "v", verbose);
    MTestArgListDestroy(head);
    return 0;
}

int run(const char *arg)
{
    int mpi_err, errs = 0;
    MPI_Datatype type;

    parse_args(arg);

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

    return errs;
}
