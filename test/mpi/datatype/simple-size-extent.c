/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Tests that Type_get_extent of a couple of basic types succeeds. */

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
    int mpi_err, errs = 0, size;
    MPI_Aint lb, ub, extent;
    MPI_Datatype type;

    struct {
        float a;
        int b;
    } foo;

    MPI_Init(&argc, &argv);
    parse_args(argc, argv);

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
    mpi_err = MPI_Type_ub(type, &ub);
    if (mpi_err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "MPI_Type_ub of MPI_INT failed.\n");
        }
        errs++;
    }
    if (ub != extent - lb) {
        if (verbose) {
            fprintf(stderr, "MPI_Type_ub of MPI_INT returned incorrect ub (%d); should be %d.\n",
                    (int) ub, (int) (extent - lb));
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
    mpi_err = MPI_Type_ub(type, &ub);
    if (mpi_err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "MPI_Type_ub of MPI_FLOAT_INT failed.\n");
        }
        errs++;
    }
    if (ub != extent - lb) {
        if (verbose) {
            fprintf(stderr,
                    "MPI_Type_ub of MPI_FLOAT_INT returned incorrect ub (%d); should be %d.\n",
                    (int) ub, (int) (extent - lb));
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
