/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <string.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run datatype_get_elements_pairtype
int run(const char *arg);
#endif

static int verbose = 0;

/* tests */
static int double_int_test(void);

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
    err = double_int_test();
    if (err && verbose)
        fprintf(stderr, "%d errors in double_int test.\n", err);
    errs += err;

    return errs;
}

/* send a { double, int, double} tuple and receive as a pair of
 * MPI_DOUBLE_INTs. this should (a) be valid, and (b) result in an
 * element count of 3.
 */
static int double_int_test(void)
{
    int err, errs = 0, count;

    struct {
        double a;
        int b;
        double c;
        int d;
    } foo;
    struct {
        double a;
        int b;
        double c;
        int d;
    } bar;

    int blks[4] = { 1, 1, 1, 1 };
    MPI_Aint disps[4] = { 0, 0, 0, 0 };
    MPI_Datatype types[] = { MPI_DOUBLE, MPI_INT, MPI_DOUBLE, MPI_INT };
    MPI_Datatype stype;

    MPI_Status recvstatus;

    /* fill in disps[1..2] with appropriate offset */
    disps[1] = (MPI_Aint) ((char *) &foo.b - (char *) &foo.a);
    disps[2] = (MPI_Aint) ((char *) &foo.c - (char *) &foo.a);
    disps[3] = (MPI_Aint) ((char *) &foo.d - (char *) &foo.a);

    MPI_Type_create_struct(4, blks, disps, types, &stype);
    MPI_Type_commit(&stype);

    err = MPI_Sendrecv((const void *) &foo, 1, stype, 0, 0,
                       (void *) &bar, 2, MPI_DOUBLE_INT, 0, 0, MPI_COMM_SELF, &recvstatus);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "MPI_Sendrecv returned error (%d)\n", err);
        return errs;
    }

    err = MPI_Get_elements(&recvstatus, MPI_DOUBLE_INT, &count);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "MPI_Get_elements returned error (%d)\n", err);
    }

    if (count != 4) {
        errs++;
        if (verbose)
            fprintf(stderr, "MPI_Get_elements returned count of %d, should be 3\n", count);
    }

    MPI_Type_free(&stype);

    return errs;
}
