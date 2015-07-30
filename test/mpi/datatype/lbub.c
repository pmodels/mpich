/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

/*
   The default behavior of the test routines should be to briefly indicate
   the cause of any errors - in this test, that means that verbose needs
   to be set. Verbose should turn on output that is independent of error
   levels.
*/
static int verbose = 1;

/* tests */
int int_with_lb_ub_test(void);
int contig_of_int_with_lb_ub_test(void);
int contig_negextent_of_int_with_lb_ub_test(void);
int vector_of_int_with_lb_ub_test(void);
int vector_blklen_of_int_with_lb_ub_test(void);
int vector_blklen_stride_of_int_with_lb_ub_test(void);
int vector_blklen_stride_negextent_of_int_with_lb_ub_test(void);
int vector_blklen_negstride_negextent_of_int_with_lb_ub_test(void);
int int_with_negextent_test(void);
int vector_blklen_negstride_of_int_with_lb_ub_test(void);

/* helper functions */
int parse_args(int argc, char **argv);

int main(int argc, char **argv)
{
    int err, errs = 0;

    MTest_Init(&argc, &argv);
    parse_args(argc, argv);

    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    /* perform some tests */
    err = int_with_lb_ub_test();
    if (err && verbose)
        fprintf(stderr, "found %d errors in simple lb/ub test\n", err);
    errs += err;

    err = contig_of_int_with_lb_ub_test();
    if (err && verbose)
        fprintf(stderr, "found %d errors in contig test\n", err);
    errs += err;

    err = contig_negextent_of_int_with_lb_ub_test();
    if (err && verbose)
        fprintf(stderr, "found %d errors in negextent contig test\n", err);
    errs += err;

    err = vector_of_int_with_lb_ub_test();
    if (err && verbose)
        fprintf(stderr, "found %d errors in simple vector test\n", err);
    errs += err;

    err = vector_blklen_of_int_with_lb_ub_test();
    if (err && verbose)
        fprintf(stderr, "found %d errors in vector blklen test\n", err);
    errs += err;

    err = vector_blklen_stride_of_int_with_lb_ub_test();
    if (err && verbose)
        fprintf(stderr, "found %d errors in strided vector test\n", err);
    errs += err;

    err = vector_blklen_negstride_of_int_with_lb_ub_test();
    if (err && verbose)
        fprintf(stderr, "found %d errors in negstrided vector test\n", err);
    errs += err;

    err = int_with_negextent_test();
    if (err && verbose)
        fprintf(stderr, "found %d errors in negextent lb/ub test\n", err);
    errs += err;

    err = vector_blklen_stride_negextent_of_int_with_lb_ub_test();
    if (err && verbose)
        fprintf(stderr, "found %d errors in strided negextent vector test\n", err);
    errs += err;

    err = vector_blklen_negstride_negextent_of_int_with_lb_ub_test();
    if (err && verbose)
        fprintf(stderr, "found %d errors in negstrided negextent vector test\n", err);
    errs += err;

    MTest_Finalize(errs);
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

int int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint lb, extent, aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { -3, 0, 6 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };

    MPI_Datatype eviltype;

    err = MPI_Type_struct(3, blocks, disps, types, &eviltype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_struct failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_size failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (val != 4) {
        errs++;
        if (verbose)
            fprintf(stderr, "  size of type = %d; should be %d\n", val, 4);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 9) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %ld; should be %d\n", (long) aval, 9);
    }

    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_lb failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != -3) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -3);
    }

    err = MPI_Type_get_extent(eviltype, &lb, &extent);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (lb != -3) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -3);
    }

    if (extent != 9) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d; should be %d\n", (int) extent, 9);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_ub failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 6) {
        errs++;
        if (verbose)
            fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, 6);
    }

    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (true_lb != 0) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, 0);
    }

    if (aval != 4) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 4);
    }

    MPI_Type_free(&eviltype);

    return errs;
}

int contig_of_int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint lb, extent, aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { -3, 0, 6 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };
    char *typemapstring = 0;

    MPI_Datatype inttype, eviltype;

    /* build same type as in int_with_lb_ub_test() */
    typemapstring = (char *) "{ (LB,-3),4*(BYTE,0),(UB,6) }";
    err = MPI_Type_struct(3, blocks, disps, types, &inttype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_struct of %s failed.\n", typemapstring);
        if (verbose)
            MTestPrintError(err);
        /* no point in continuing */
        return errs;
    }

    typemapstring = (char *)
        "{ (LB,-3),4*(BYTE,0),(UB,6),(LB,6),4*(BYTE,9),(UB,15),(LB,15),4*(BYTE,18),(UB,24)}";
    err = MPI_Type_contiguous(3, inttype, &eviltype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_contiguous of %s failed.\n", typemapstring);
        if (verbose)
            MTestPrintError(err);
        /* no point in continuing */
        return errs;
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_size of %s failed.\n", typemapstring);
        if (verbose)
            MTestPrintError(err);
    }

    if (val != 12) {
        errs++;
        if (verbose)
            fprintf(stderr, "  size of type = %d; should be %d\n", val, 12);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 27) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, 27);
        if (verbose)
            fprintf(stderr, " for type %s\n", typemapstring);
    }

    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_lb failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != -3) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d from Type_lb; should be %d in %s\n", (int) aval, -3,
                    typemapstring);
    }

    err = MPI_Type_get_extent(eviltype, &lb, &extent);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (lb != -3) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d from Type_get_extent; should be %d in %s\n",
                    (int) aval, -3, typemapstring);
    }

    if (extent != 27) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d from Type_get_extent; should be %d in %s\n",
                    (int) extent, 27, typemapstring);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_ub failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 24) {
        errs++;
        if (verbose)
            fprintf(stderr, "  ub of type = %d in Type_ub; should be %din %s\n", (int) aval, 24,
                    typemapstring);
    }

    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (true_lb != 0) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true_lb of type = %d; should be %d in %s\n", (int) true_lb, 0,
                    typemapstring);
    }

    if (aval != 22) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true extent of type = %d; should be %d in %s\n", (int) aval, 22,
                    typemapstring);
    }

    MPI_Type_free(&inttype);
    MPI_Type_free(&eviltype);

    return errs;
}

int contig_negextent_of_int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint lb, extent, aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { 6, 0, -3 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };
    char *typemapstring = 0;

    MPI_Datatype inttype, eviltype;

    /* build same type as in int_with_lb_ub_test() */
    typemapstring = (char *) "{ (LB,6),4*(BYTE,0),(UB,-3) }";
    err = MPI_Type_struct(3, blocks, disps, types, &inttype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_struct of %s failed.\n", typemapstring);
        if (verbose)
            MTestPrintError(err);
        /* No point in continuing */
        return errs;
    }

    typemapstring = (char *)
        "{ (LB,6),4*(BYTE,0),(UB,-3),(LB,-3),4*(BYTE,-9),(UB,-12),(LB,-12),4*(BYTE,-18),(UB,-21) }";
    err = MPI_Type_contiguous(3, inttype, &eviltype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_contiguous of %s failed.\n", typemapstring);
        if (verbose)
            MTestPrintError(err);
        /* No point in continuing */
        return errs;
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_size of %s failed.\n", typemapstring);
        if (verbose)
            MTestPrintError(err);
    }

    if (val != 12) {
        errs++;
        if (verbose)
            fprintf(stderr, "  size of type = %d; should be %d\n", val, 12);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 9) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, 9);
    }

    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_lb failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != -12) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -12);
    }

    err = MPI_Type_get_extent(eviltype, &lb, &extent);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (lb != -12) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -12);
    }

    if (extent != 9) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d; should be %d\n", (int) extent, 9);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_ub failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != -3) {
        errs++;
        if (verbose)
            fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, -3);
    }

    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (true_lb != -18) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, -18);
    }

    if (aval != 22) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 22);
    }

    MPI_Type_free(&inttype);
    MPI_Type_free(&eviltype);

    return errs;
}

int vector_of_int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint lb, extent, aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { -3, 0, 6 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };

    MPI_Datatype inttype, eviltype;

    /* build same type as in int_with_lb_ub_test() */
    err = MPI_Type_struct(3, blocks, disps, types, &inttype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_struct failed.\n");
        if (verbose)
            MTestPrintError(err);
        /* no point in continuing */
        return errs;
    }

    err = MPI_Type_vector(3, 1, 1, inttype, &eviltype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_vector failed.\n");
        if (verbose)
            MTestPrintError(err);
        /* no point in continuing */
        return errs;
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_size failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (val != 12) {
        errs++;
        if (verbose)
            fprintf(stderr, "  size of type = %d; should be %d\n", val, 12);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 27) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, 27);
    }

    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_lb failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != -3) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -3);
    }

    err = MPI_Type_get_extent(eviltype, &lb, &extent);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (lb != -3) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -3);
    }

    if (extent != 27) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d; should be %d\n", (int) extent, 27);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_ub failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 24) {
        errs++;
        if (verbose)
            fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, 24);
    }

    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (true_lb != 0) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, 0);
    }

    if (aval != 22) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 22);
    }

    MPI_Type_free(&inttype);
    MPI_Type_free(&eviltype);

    return errs;
}

/*
 * blklen = 4
 */
int vector_blklen_of_int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint lb, extent, aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { -3, 0, 6 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };

    MPI_Datatype inttype, eviltype;

    /* build same type as in int_with_lb_ub_test() */
    err = MPI_Type_struct(3, blocks, disps, types, &inttype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_struct failed.\n");
        if (verbose)
            MTestPrintError(err);
        /* no point in continuing */
        return errs;
    }

    err = MPI_Type_vector(3, 4, 1, inttype, &eviltype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_vector failed.\n");
        if (verbose)
            MTestPrintError(err);
        /* no point in continuing */
        return errs;
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_size failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (val != 48) {
        errs++;
        if (verbose)
            fprintf(stderr, "  size of type = %d; should be %d\n", val, 48);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 54) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, 54);
    }

    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_lb failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != -3) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -3);
        if (verbose)
            MTestPrintError(err);
    }

    err = MPI_Type_get_extent(eviltype, &lb, &extent);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (lb != -3) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -3);
    }

    if (extent != 54) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d; should be %d\n", (int) extent, 54);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_ub failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 51) {
        errs++;
        if (verbose)
            fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, 51);
    }

    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (true_lb != 0) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, 0);
    }

    if (aval != 49) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 49);
    }

    MPI_Type_free(&inttype);
    MPI_Type_free(&eviltype);

    return errs;
}

int vector_blklen_stride_of_int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint lb, extent, aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { -3, 0, 6 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };
    char *typemapstring = 0;

    MPI_Datatype inttype, eviltype;

    /* build same type as in int_with_lb_ub_test() */
    typemapstring = (char *) "{ (LB,-3),4*(BYTE,0),(UB,6) }";
    err = MPI_Type_struct(3, blocks, disps, types, &inttype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_struct of %s failed.\n", typemapstring);
        if (verbose)
            MTestPrintError(err);
        /* No point in continuing */
        return errs;
    }

    err = MPI_Type_vector(3, 4, 5, inttype, &eviltype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_vector failed.\n");
        if (verbose)
            MTestPrintError(err);
        /* no point in continuing */
        return errs;
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_size failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (val != 48) {
        errs++;
        if (verbose)
            fprintf(stderr, "  size of type = %d; should be %d\n", val, 48);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 126) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, 126);
    }

    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_lb failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != -3) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -3);
    }

    err = MPI_Type_get_extent(eviltype, &lb, &extent);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (lb != -3) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -3);
    }

    if (extent != 126) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d; should be %d\n", (int) extent, 126);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_ub failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 123) {
        errs++;
        if (verbose)
            fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, 123);
    }

    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (true_lb != 0) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, 0);
    }

    if (aval != 121) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 121);
    }

    MPI_Type_free(&inttype);
    MPI_Type_free(&eviltype);

    return errs;
}

int vector_blklen_negstride_of_int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint lb, extent, aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { -3, 0, 6 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };

    MPI_Datatype inttype, eviltype;

    /* build same type as in int_with_lb_ub_test() */
    err = MPI_Type_struct(3, blocks, disps, types, &inttype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_struct failed.\n");
        if (verbose)
            MTestPrintError(err);
        /* no point in continuing */
        return errs;
    }

    err = MPI_Type_vector(3, 4, -5, inttype, &eviltype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_vector failed.\n");
        if (verbose)
            MTestPrintError(err);
        /* no point in continuing */
        return errs;
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_size failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (val != 48) {
        errs++;
        if (verbose)
            fprintf(stderr, "  size of type = %d; should be %d\n", val, 48);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 126) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, 126);
    }

    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_lb failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != -93) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -93);
    }

    err = MPI_Type_get_extent(eviltype, &lb, &extent);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (lb != -93) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -93);
    }

    if (extent != 126) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d; should be %d\n", (int) extent, 126);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_ub failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 33) {
        errs++;
        if (verbose)
            fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, 33);
    }

    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (true_lb != -90) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, -90);
    }

    if (aval != 121) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 121);
    }

    MPI_Type_free(&inttype);
    MPI_Type_free(&eviltype);

    return errs;
}

int int_with_negextent_test(void)
{
    int err, errs = 0, val;
    MPI_Aint lb, extent, aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { 6, 0, -3 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };
    char *typemapstring = 0;

    MPI_Datatype eviltype;

    typemapstring = (char *) "{ (LB,6),4*(BYTE,0),(UB,-3) }";
    err = MPI_Type_struct(3, blocks, disps, types, &eviltype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_struct of %s failed.\n", typemapstring);
        if (verbose)
            MTestPrintError(err);
        /* No point in contiuing */
        return errs;
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_size failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (val != 4) {
        errs++;
        if (verbose)
            fprintf(stderr, "  size of type = %d; should be %d\n", val, 4);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != -9) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, -9);
    }

    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_lb failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 6) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, 6);
    }

    err = MPI_Type_get_extent(eviltype, &lb, &extent);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (lb != 6) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, 6);
    }

    if (extent != -9) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d; should be %d\n", (int) extent, -9);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_ub failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != -3) {
        errs++;
        if (verbose)
            fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, -3);
    }

    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (true_lb != 0) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, 0);
    }

    if (aval != 4) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 4);
    }

    MPI_Type_free(&eviltype);

    return errs;
}

int vector_blklen_stride_negextent_of_int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint lb, extent, true_lb, aval;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { 6, 0, -3 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };
    MPI_Datatype inttype, eviltype;
    char *typemapstring = 0;

    /* build same type as in int_with_lb_ub_test() */
    typemapstring = (char *) "{ (LB,6),4*(BYTE,0),(UB,-3) }";
    err = MPI_Type_struct(3, blocks, disps, types, &inttype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_struct of %s failed.\n", typemapstring);
        if (verbose)
            MTestPrintError(err);
        /* No point in continuing */
        return errs;
    }

    err = MPI_Type_vector(3, 4, 5, inttype, &eviltype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_vector failed.\n");
        if (verbose)
            MTestPrintError(err);
        /* no point in continuing */
        return errs;
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_size failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (val != 48) {
        errs++;
        if (verbose)
            fprintf(stderr, "  size of type = %d; should be %d\n", val, 48);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 108) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, 108);
    }

    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_lb failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != -111) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -111);
    }

    err = MPI_Type_get_extent(eviltype, &lb, &extent);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (lb != -111) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -111);
    }

    if (extent != 108) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %d; should be %d\n", (int) extent, 108);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_ub failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != -3) {
        errs++;
        if (verbose)
            fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, -3);
    }

    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (true_lb != -117) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, -117);
    }

    if (aval != 121) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 121);
    }

    MPI_Type_free(&inttype);
    MPI_Type_free(&eviltype);

    return errs;
}

int vector_blklen_negstride_negextent_of_int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint extent, lb, aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { 6, 0, -3 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };

    MPI_Datatype inttype, eviltype;

    /* build same type as in int_with_lb_ub_test() */
    err = MPI_Type_struct(3, blocks, disps, types, &inttype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_struct failed.\n");
        if (verbose)
            MTestPrintError(err);
        /* no point in continuing */
        return errs;
    }

    err = MPI_Type_vector(3, 4, -5, inttype, &eviltype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_vector failed.\n");
        if (verbose)
            MTestPrintError(err);
        /* no point in continuing */
        return errs;
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_size failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (val != 48) {
        errs++;
        if (verbose)
            fprintf(stderr, "  size of type = %d; should be %d\n", val, 48);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 108) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %ld; should be %d\n", (long) aval, 108);
    }

    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_lb failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != -21) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %ld; should be %d\n", (long) aval, -21);
    }

    err = MPI_Type_get_extent(eviltype, &lb, &extent);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (lb != -21) {
        errs++;
        if (verbose)
            fprintf(stderr, "  lb of type = %ld; should be %d\n", (long) aval, -21);
    }

    if (extent != 108) {
        errs++;
        if (verbose)
            fprintf(stderr, "  extent of type = %ld; should be %d\n", (long) extent, 108);
    }


    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_ub failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (aval != 87) {
        errs++;
        if (verbose)
            fprintf(stderr, "  ub of type = %ld; should be %d\n", (long) aval, 87);
    }

    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose)
            fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
        if (verbose)
            MTestPrintError(err);
    }

    if (true_lb != -27) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true_lb of type = %ld; should be %d\n", (long) true_lb, -27);
    }

    if (aval != 121) {
        errs++;
        if (verbose)
            fprintf(stderr, "  true extent of type = %ld; should be %d\n", (long) aval, 121);
    }

    MPI_Type_free(&inttype);
    MPI_Type_free(&eviltype);

    return errs;
}
