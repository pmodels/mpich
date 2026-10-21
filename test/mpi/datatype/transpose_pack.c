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
#define run datatype_transpose_pack
int run(const char *arg);
#endif
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
    /* Variable declarations */
    int a[100][100], b[100][100];
    MPI_Datatype row, xpose;
    MPI_Aint sizeofint, tmp_lb;

    int errs = 0;
    int bufsize, position = 0;
    void *buffer;

    int i, j;

    /* Initialize a to some known values. */
    for (i = 0; i < 100; i++) {
        for (j = 0; j < 100; j++) {
            a[i][j] = i * 1000 + j;
            b[i][j] = -1;
        }
    }

    /* Initialize MPI */
    parse_args(arg);

    MPI_Type_get_extent(MPI_INT, &tmp_lb, &sizeofint);

    /* Create datatypes. */
    MPI_Type_vector(100, 1, 100, MPI_INT, &row);
    MPI_Type_create_hvector(100, 1, sizeofint, row, &xpose);
    MPI_Type_commit(&xpose);

    /* Pack it. */
    MPI_Pack_size(1, xpose, MPI_COMM_WORLD, &bufsize);
    buffer = (char *) malloc((unsigned) bufsize);

    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Pack(a, 1, xpose, buffer, bufsize, &position, MPI_COMM_WORLD);

    /* Unpack the buffer into b. */
    position = 0;
    MPI_Unpack(buffer, bufsize, &position, b, 100 * 100, MPI_INT, MPI_COMM_WORLD);

    for (i = 0; i < 100; i++) {
        for (j = 0; j < 100; j++) {
            if (b[i][j] != a[j][i]) {
                errs++;
                if (verbose)
                    fprintf(stderr, "b[%d][%d] = %d, should be %d\n", i, j, b[i][j], a[j][i]);
            }
        }
    }

    MPI_Type_free(&xpose);
    MPI_Type_free(&row);

    free(buffer);
    return errs;
}
