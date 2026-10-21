/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "mpitest.h"
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run datatype_pairtype_size_extent
int run(const char *arg);
#endif

static int verbose = 1;

static struct {
    MPI_Datatype atype, ptype;
    char name[32];
} pairtypes[] = { {
MPI_FLOAT, MPI_FLOAT_INT, "MPI_FLOAT_INT"}, {
MPI_DOUBLE, MPI_DOUBLE_INT, "MPI_DOUBLE_INT"}, {
MPI_LONG, MPI_LONG_INT, "MPI_LONG_INT"}, {
MPI_SHORT, MPI_SHORT_INT, "MPI_SHORT_INT"}, {
MPI_LONG_DOUBLE, MPI_LONG_DOUBLE_INT, "MPI_LONG_DOUBLE_INT"}, {
(MPI_Datatype) - 1, (MPI_Datatype) - 1, "end"}
};

static MPI_Aint pairtype_displacement(MPI_Datatype type, int *out_size_p);

static MPI_Aint pairtype_displacement(MPI_Datatype type, int *out_size_p)
{
    MPI_Aint disp;

    /* Note that a portable test may not use a switch statement for
     * datatypes, as they are not required to be compile-time constants */
    if (type == MPI_FLOAT_INT) {
        struct {
            float a;
            int b;
        } foo;
        disp = (MPI_Aint) ((char *) &foo.b - (char *) &foo.a);
        *out_size_p = sizeof(foo);
    } else if (type == MPI_DOUBLE_INT) {
        struct {
            double a;
            int b;
        } foo;
        disp = (MPI_Aint) ((char *) &foo.b - (char *) &foo.a);
        *out_size_p = sizeof(foo);
    } else if (type == MPI_LONG_INT) {
        struct {
            long a;
            int b;
        } foo;
        disp = (MPI_Aint) ((char *) &foo.b - (char *) &foo.a);
        *out_size_p = sizeof(foo);
    } else if (type == MPI_SHORT_INT) {
        struct {
            short a;
            int b;
        } foo;
        disp = (MPI_Aint) ((char *) &foo.b - (char *) &foo.a);
        *out_size_p = sizeof(foo);
    } else if (type == MPI_LONG_DOUBLE_INT && type != MPI_DATATYPE_NULL) {
        struct {
            long double a;
            int b;
        } foo;
        disp = (MPI_Aint) ((char *) &foo.b - (char *) &foo.a);
        *out_size_p = sizeof(foo);
    } else {
        disp = -1;
    }
    return disp;
}

static int parse_args(const char *arg)
{
    MTestArgList *head = MTestArgListCreate_arg(arg);
    verbose = MTestArgListGetInt_with_default(head, "v", verbose);
    MTestArgListDestroy(head);
    return 0;
}

int run(const char *arg)
{
    int errs = 0;

    int i;
    int blks[2] = { 1, 1 };
    MPI_Aint disps[2] = { 0, 0 };
    MPI_Datatype types[2] = { MPI_INT, MPI_INT };
    MPI_Datatype stype;

    parse_args(arg);

    for (i = 0; pairtypes[i].atype != (MPI_Datatype) - 1; i++) {
        int atype_size, ptype_size, stype_size, handbuilt_extent;
        MPI_Aint ptype_extent, stype_extent, dummy_lb;

        types[0] = pairtypes[i].atype;

        /* Check for undefined optional types, such as
         * LONG_DOUBLE_INT (if, for example, long double or
         * long long are not supported) */
        if (types[0] == MPI_DATATYPE_NULL)
            continue;

        MPI_Type_size(types[0], &atype_size);
        disps[1] = pairtype_displacement(pairtypes[i].ptype, &handbuilt_extent);

        MPI_Type_create_struct(2, blks, disps, types, &stype);

        MPI_Type_size(stype, &stype_size);
        MPI_Type_size(pairtypes[i].ptype, &ptype_size);
        if (stype_size != ptype_size) {
            errs++;

            if (verbose)
                fprintf(stderr,
                        "size of %s (%d) does not match size of hand-built MPI struct (%d)\n",
                        pairtypes[i].name, ptype_size, stype_size);
        }

        MPI_Type_get_extent(stype, &dummy_lb, &stype_extent);
        MPI_Type_get_extent(pairtypes[i].ptype, &dummy_lb, &ptype_extent);
        if (stype_extent != ptype_extent || stype_extent != handbuilt_extent) {
            errs++;

            if (verbose)
                fprintf(stderr,
                        "extent of %s (%d) does not match extent of either hand-built MPI struct (%d) or equivalent C struct (%d)\n",
                        pairtypes[i].name, (int) ptype_extent,
                        (int) stype_extent, handbuilt_extent);
        }
        MPI_Type_free(&stype);
    }

    return errs;
}
