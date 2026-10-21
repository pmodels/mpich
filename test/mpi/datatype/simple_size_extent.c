/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Tests that Type_get_extent of a couple of basic types succeeds. */

#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run datatype_simple_size_extent
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
    int mpi_err, errs = 0, size;
    MPI_Aint lb, extent;
    MPI_Datatype type;

    struct {
        float a;
        int b;
    } foo;

    parse_args(arg);

    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    type = MPI_INT;
    mpi_err = MPI_Type_size(type, &size);
    if (mpi_err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "MPI_Type_size of MPI_INT failed.\n");
        }
        errs++;
    }
    if (size != sizeof(int)) {
        if (verbose) {
            fprintf(stderr, "MPI_Type_size of MPI_INT incorrect size (%d); should be %d.\n",
                    size, (int) sizeof(int));
        }
        errs++;
    }

    mpi_err = MPI_Type_get_extent(type, &lb, &extent);
    if (mpi_err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "MPI_Type_get_extent of MPI_INT failed.\n");
        }
        errs++;
    }
    if (extent != sizeof(int)) {
        if (verbose) {
            fprintf(stderr,
                    "MPI_Type_get_extent of MPI_INT returned incorrect extent (%d); should be %d.\n",
                    (int) extent, (int) sizeof(int));
        }
        errs++;
    }
    if (lb != 0) {
        if (verbose) {
            fprintf(stderr,
                    "MPI_Type_get_extent of MPI_INT returned incorrect lb (%d); should be 0.\n",
                    (int) lb);
        }
        errs++;
    }

    type = MPI_FLOAT_INT;
    mpi_err = MPI_Type_size(type, &size);
    if (mpi_err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "MPI_Type_size of MPI_FLOAT_INT failed.\n");
        }
        errs++;
    }
    if (size != sizeof(float) + sizeof(int)) {
        if (verbose) {
            fprintf(stderr,
                    "MPI_Type_size of MPI_FLOAT_INT returned incorrect size (%d); should be %d.\n",
                    size, (int) (sizeof(float) + sizeof(int)));
        }
        errs++;
    }

    mpi_err = MPI_Type_get_extent(type, &lb, &extent);
    if (mpi_err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "MPI_Type_get_extent of MPI_FLOAT_INT failed.\n");
        }
        errs++;
    }
    if (extent != sizeof(foo)) {
        if (verbose) {
            fprintf(stderr,
                    "MPI_Type_get_extent of MPI_FLOAT_INT returned incorrect extent (%d); should be %d.\n",
                    (int) extent, (int) sizeof(foo));
        }
        errs++;
    }
    if (lb != 0) {
        if (verbose) {
            fprintf(stderr,
                    "MPI_Type_get_extent of MPI_FLOAT_INT returned incorrect lb (%d); should be 0.\n",
                    (int) lb);
        }
        errs++;
    }

    return errs;
}
