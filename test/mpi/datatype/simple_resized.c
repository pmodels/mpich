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
#define run datatype_simple_resized
int run(const char *arg);
#endif

static int verbose = 0;

/* tests */
static int derived_resized_test(void);

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
    err = derived_resized_test();
    if (err && verbose)
        fprintf(stderr, "%d errors in derived_resized test.\n", err);
    errs += err;

    return errs;
}

/* derived_resized_test()
 *
 * Tests behavior with resizing of a simple derived type.
 *
 * Returns the number of errors encountered.
 */
static int derived_resized_test(void)
{
    int err, errs = 0;

    int count = 2;
    MPI_Datatype newtype, resizedtype;

    int size;
    MPI_Aint extent, tmp_lb;

    err = MPI_Type_contiguous(count, MPI_INT, &newtype);
    if (err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "error creating type in derived_resized_test()\n");
        }
        errs++;
    }

    err = MPI_Type_create_resized(newtype,
                                  (MPI_Aint) 0, (MPI_Aint) (2 * sizeof(int) + 10), &resizedtype);

    err = MPI_Type_size(resizedtype, &size);
    if (err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "error obtaining type size in derived_resized_test()\n");
        }
        errs++;
    }

    if (size != 2 * sizeof(int)) {
        if (verbose) {
            fprintf(stderr,
                    "error: size != %d in derived_resized_test()\n", (int) (2 * sizeof(int)));
        }
        errs++;
    }

    err = MPI_Type_get_extent(resizedtype, &tmp_lb, &extent);
    if (err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "error obtaining type extent in derived_resized_test()\n");
        }
        errs++;
    }

    if (extent != 2 * sizeof(int) + 10) {
        if (verbose) {
            fprintf(stderr,
                    "error: invalid extent (%d) in derived_resized_test(); should be %d\n",
                    (int) extent, (int) (2 * sizeof(int) + 10));
        }
        errs++;
    }

    MPI_Type_free(&newtype);
    MPI_Type_free(&resizedtype);

    return errs;
}
