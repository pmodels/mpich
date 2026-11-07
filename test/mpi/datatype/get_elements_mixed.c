/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run datatype_get_elements_mixed
int run(const char *arg);
#endif

/* Tests MPI_Get_elements with struct types of different basic types, including
 * types of the same size, e.g. {int; float}, and mixes with pair types, which
 * count as two elements. */

static int check_elements(MPI_Datatype type, int recv_count, int nbytes, int expect,
                          const char *name)
{
    char sendbuf[256], recvbuf[256];
    MPI_Status status;
    int count = -99;

    MPI_Sendrecv(sendbuf, nbytes, MPI_BYTE, 0, 0, recvbuf, recv_count, type, 0, 0,
                 MPI_COMM_SELF, &status);
    MPI_Get_elements(&status, type, &count);
    if (count != expect) {
        printf("%s: %d bytes, expected %d elements, got %d\n", name, nbytes, expect, count);
        return 1;
    }
    return 0;
}

static MPI_Datatype make_struct(MPI_Datatype type1, MPI_Datatype type2)
{
    int blens[2] = { 1, 1 };
    MPI_Aint lb, extent;
    MPI_Aint displs[2];
    MPI_Datatype types[2] = { type1, type2 };
    MPI_Datatype newtype;

    MPI_Type_get_extent(type1, &lb, &extent);
    displs[0] = 0;
    displs[1] = extent;
    MPI_Type_create_struct(2, blens, displs, types, &newtype);
    MPI_Type_commit(&newtype);
    return newtype;
}

int run(const char *arg)
{
    int errs = 0;
    MPI_Datatype int_float, contig, nested, pair_double, floatint_int;
    MPI_Datatype pair_char, floatint_char, contig_floatint, contig_floatint_char;

    /* different basic types of the same size */
    int_float = make_struct(MPI_INT, MPI_FLOAT);
    errs += check_elements(int_float, 1, 8, 2, "{int; float}");
    errs += check_elements(int_float, 2, 12, 3, "{int; float}");

    MPI_Type_contiguous(3, int_float, &contig);
    MPI_Type_commit(&contig);
    errs += check_elements(contig, 1, 24, 6, "contig(3, {int; float})");
    errs += check_elements(contig, 1, 20, 5, "contig(3, {int; float})");

    /* elements of different sizes, recursing into {int; float} */
    nested = make_struct(MPI_DOUBLE, int_float);
    errs += check_elements(nested, 1, 16, 3, "{double; {int; float}}");
    errs += check_elements(nested, 1, 12, 2, "{double; {int; float}}");

    /* a builtin pair type holds two elements */
    pair_double = make_struct(MPI_2INT, MPI_DOUBLE);
    errs += check_elements(pair_double, 1, 16, 3, "{2int; double}");
    errs += check_elements(pair_double, 1, 8, 2, "{2int; double}");

    floatint_int = make_struct(MPI_FLOAT_INT, MPI_INT);
    errs += check_elements(floatint_int, 1, 12, 3, "{float_int; int}");
    errs += check_elements(floatint_int, 1, 8, 2, "{float_int; int}");

    pair_char = make_struct(MPI_2INT, MPI_CHAR);
    errs += check_elements(pair_char, 1, 9, 3, "{2int; char}");

    floatint_char = make_struct(MPI_FLOAT_INT, MPI_CHAR);
    errs += check_elements(floatint_char, 1, 9, 3, "{float_int; char}");

    MPI_Type_contiguous(2, MPI_FLOAT_INT, &contig_floatint);
    MPI_Type_commit(&contig_floatint);
    errs += check_elements(contig_floatint, 1, 16, 4, "contig(2, float_int)");
    errs += check_elements(contig_floatint, 1, 12, 3, "contig(2, float_int)");

    contig_floatint_char = make_struct(contig_floatint, MPI_CHAR);
    errs += check_elements(contig_floatint_char, 1, 17, 5, "{contig(2, float_int); char}");
    errs += check_elements(contig_floatint_char, 1, 12, 3, "{contig(2, float_int); char}");

    MPI_Type_free(&int_float);
    MPI_Type_free(&contig);
    MPI_Type_free(&nested);
    MPI_Type_free(&pair_double);
    MPI_Type_free(&floatint_int);
    MPI_Type_free(&pair_char);
    MPI_Type_free(&floatint_char);
    MPI_Type_free(&contig_floatint);
    MPI_Type_free(&contig_floatint_char);

    return errs;
}
