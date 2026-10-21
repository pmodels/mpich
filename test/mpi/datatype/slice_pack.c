/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <math.h>
#include "mpitest.h"
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>

#ifdef MULTI_TESTS
#define run datatype_slice_pack
int run(const char *arg);
#endif
#endif

static int verbose = 0;
static int a[100][100][100], e[9][9][9];

static int parse_args(const char *arg)
{
    MTestArgList *head = MTestArgListCreate_arg(arg);
    verbose = MTestArgListGetInt_with_default(head, "v", verbose);
    MTestArgListDestroy(head);
    return 0;
}


/* helper functions */

int run(const char *arg)
{
    /* Variable declarations */
    MPI_Datatype oneslice, twoslice, threeslice;
    int errs = 0;
    MPI_Aint sizeofint, tmp_lb;

    int bufsize, position;
    void *buffer;

    int i, j, k;

    /* Initialize a to some known values. */
    for (i = 0; i < 100; i++) {
        for (j = 0; j < 100; j++) {
            for (k = 0; k < 100; k++) {
                a[i][j][k] = i * 1000000 + j * 1000 + k;
            }
        }
    }

    /* Initialize MPI */
    MPI_Type_get_extent(MPI_INT, &tmp_lb, &sizeofint);

    parse_args(arg);

    /* Create data types. */
    /* NOTE: This differs from the way that it's done on the sheet. */
    /* On the sheet, the slice is a[0, 2, 4, ..., 16][2-10][1-9]. */
    /* Below, the slice is a[0-8][2-10][1, 3, 5, ..., 17]. */
    MPI_Type_vector(9, 1, 2, MPI_INT, &oneslice);
    MPI_Type_create_hvector(9, 1, 100 * sizeofint, oneslice, &twoslice);
    MPI_Type_create_hvector(9, 1, 100 * 100 * sizeofint, twoslice, &threeslice);

    MPI_Type_commit(&threeslice);

    /* Pack it into a buffer. */
    position = 0;
    MPI_Pack_size(1, threeslice, MPI_COMM_WORLD, &bufsize);
    buffer = (void *) malloc((unsigned) bufsize);

    /* -1 to indices on sheet to compensate for Fortran --> C */
    MPI_Pack(&(a[0][2][1]), 1, threeslice, buffer, bufsize, &position, MPI_COMM_WORLD);

    /* Unpack the buffer into e. */
    position = 0;
    MPI_Unpack(buffer, bufsize, &position, e, 9 * 9 * 9, MPI_INT, MPI_COMM_WORLD);

    /* Display errors, if any. */
    for (i = 0; i < 9; i++) {
        for (j = 0; j < 9; j++) {
            for (k = 0; k < 9; k++) {
                /* The truncation in integer division makes this safe. */
                if (e[i][j][k] != a[i][j + 2][k * 2 + 1]) {
                    errs++;
                    if (verbose) {
                        printf("Error in location %d x %d x %d: %d, should be %d.\n",
                               i, j, k, e[i][j][k], a[i][j + 2][k * 2 + 1]);
                    }
                }
            }
        }
    }

    /* Release memory. */
    free(buffer);

    MPI_Type_free(&oneslice);
    MPI_Type_free(&twoslice);
    MPI_Type_free(&threeslice);

    return errs;
}

/* parse_args()
 */
