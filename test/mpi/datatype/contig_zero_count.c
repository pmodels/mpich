/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run datatype_contig_zero_count
int run(const char *arg);
#endif

static int verbose = 0;

/* tests */
static int contig_test(void);

/* helper functions */

static int parse_args(const char *arg)
{
    MTestArgList *head = MTestArgListCreate_arg(arg);
    verbose = MTestArgListGetInt_with_default(head, "v", verbose);
    MTestArgListDestroy(head);
    return 0;
}

int run(const char *arg)
{
    int err, errs = 0;

    parse_args(arg);

    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    /* perform some tests */
    err = contig_test();
    if (err && verbose)
        fprintf(stderr, "%d errors in contig test.\n", err);
    errs += err;

    return errs;
}

/* contig_test()
 *
 * Tests behavior with a zero-count contig.
 *
 * Returns the number of errors encountered.
 */
static int contig_test(void)
{
    int err, errs = 0;

    int count = 0;
    MPI_Datatype newtype;

    int size;
    MPI_Aint extent, tmp_lb;

    err = MPI_Type_contiguous(count, MPI_INT, &newtype);
    if (err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "error creating type in contig_test()\n");
        }
        errs++;
    }

    err = MPI_Type_size(newtype, &size);
    if (err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "error obtaining type size in contig_test()\n");
        }
        errs++;
    }

    if (size != 0) {
        if (verbose) {
            fprintf(stderr, "error: size != 0 in contig_test()\n");
        }
        errs++;
    }

    err = MPI_Type_get_extent(newtype, &tmp_lb, &extent);
    if (err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "error obtaining type extent in contig_test()\n");
        }
        errs++;
    }

    if (extent != 0) {
        if (verbose) {
            fprintf(stderr, "error: extent != 0 in contig_test()\n");
        }
        errs++;
    }

    MPI_Type_free(&newtype);

    return errs;
}
