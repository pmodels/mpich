/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test checks for large count functionality ("MPI_Count") mandated by
 * MPI-3, as well as behavior of corresponding pre-MPI-3 interfaces that now
 * have better defined behavior when an "int" quantity would overflow. */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/* assert-like macro that bumps the err count and emits a message */
#define check(x_)                                                                 \
    do {                                                                          \
        if (!(x_)) {                                                              \
            ++errs;                                                               \
            if (errs < 10) {                                                      \
                fprintf(stderr, "check failed: (%s), line %d\n", #x_, __LINE__); \
            }                                                                     \
        }                                                                         \
    } while (0)

int main(int argc, char *argv[])
{
    int errs = 0;
    int wrank, wsize;
    int size, elements, count;
    MPI_Aint lb, extent;
    MPI_Count size_x, lb_x, extent_x, elements_x;
    MPI_Count imx4i_true_extent;
    MPI_Datatype imax_contig = MPI_DATATYPE_NULL;
    MPI_Datatype four_ints = MPI_DATATYPE_NULL;
    MPI_Datatype imx4i = MPI_DATATYPE_NULL;
    MPI_Datatype imx4i_rsz = MPI_DATATYPE_NULL;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

    check(sizeof(MPI_Count) >= sizeof(int));
    check(sizeof(MPI_Count) >= sizeof(MPI_Aint));
    check(sizeof(MPI_Count) >= sizeof(MPI_Offset));

    /* the following two checks aren't explicitly required by the standard, but
     * it's hard to imagine a world without them holding true and so most of the
     * subsequent code probably depends on them to some degree */
    check(sizeof(MPI_Aint) >= sizeof(int));
    check(sizeof(MPI_Offset) >= sizeof(int));

    /* not much point in checking for integer overflow cases if MPI_Count is
     * only as large as an int */
    if (sizeof(MPI_Count) == sizeof(int))
        goto epilogue;

    /* a very large type */
    MPI_Type_contiguous(INT_MAX, MPI_CHAR, &imax_contig);
    MPI_Type_commit(&imax_contig);

    /* a small-ish contig */
    MPI_Type_contiguous(4, MPI_INT, &four_ints);
    MPI_Type_commit(&four_ints);

    /* a type with size>INT_MAX */
    MPI_Type_vector(INT_MAX / 2, 1, 3, four_ints, &imx4i);
    MPI_Type_commit(&imx4i);
    /* don't forget, ub for dtype w/ stride doesn't include any holes at the end
     * of the type, hence the more complicated calculation below */
    imx4i_true_extent = 3LL * 4LL * sizeof(int) * ((INT_MAX / 2) - 1) + 4LL * sizeof(int);

    /* sanity check that the MPI_COUNT predefined named datatype exists */
    MPI_Send(&imx4i_true_extent, 1, MPI_COUNT, MPI_PROC_NULL, 0, MPI_COMM_SELF);

    /* the same oversized type but with goofy extents */
    MPI_Type_create_resized(imx4i, /*lb= */ INT_MAX, /*extent= */ -1024, &imx4i_rsz);
    MPI_Type_commit(&imx4i_rsz);

    /* MPI_Type_size */
    MPI_Type_size(imax_contig, &size);
    check(size == INT_MAX);
    MPI_Type_size(four_ints, &size);
    check(size == 4 * sizeof(int));
    MPI_Type_size(imx4i, &size);
    check(size == MPI_UNDEFINED);       /* should overflow an int */
    MPI_Type_size(imx4i_rsz, &size);
    check(size == MPI_UNDEFINED);       /* should overflow an int */

    /* MPI_Type_size_x */
    MPI_Type_size_x(imax_contig, &size_x);
    check(size_x == INT_MAX);
    MPI_Type_size_x(four_ints, &size_x);
    check(size_x == 4 * sizeof(int));
    MPI_Type_size_x(imx4i, &size_x);
    check(size_x == 4LL * sizeof(int) * (INT_MAX / 2)); /* should overflow an int */
    MPI_Type_size_x(imx4i_rsz, &size_x);
    check(size_x == 4LL * sizeof(int) * (INT_MAX / 2)); /* should overflow an int */

    /* MPI_Type_get_extent */
    MPI_Type_get_extent(imax_contig, &lb, &extent);
    check(lb == 0);
    check(extent == INT_MAX);
    MPI_Type_get_extent(four_ints, &lb, &extent);
    check(lb == 0);
    check(extent == 4 * sizeof(int));
    MPI_Type_get_extent(imx4i, &lb, &extent);
    check(lb == 0);
    if (sizeof(MPI_Aint) == sizeof(int))
        check(extent == MPI_UNDEFINED);
    else
        check(extent == imx4i_true_extent);

    MPI_Type_get_extent(imx4i_rsz, &lb, &extent);
    check(lb == INT_MAX);
    check(extent == -1024);

    /* MPI_Type_get_extent_x */
    MPI_Type_get_extent_x(imax_contig, &lb_x, &extent_x);
    check(lb_x == 0);
    check(extent_x == INT_MAX);
    MPI_Type_get_extent_x(four_ints, &lb_x, &extent_x);
    check(lb_x == 0);
    check(extent_x == 4 * sizeof(int));
    MPI_Type_get_extent_x(imx4i, &lb_x, &extent_x);
    check(lb_x == 0);
    check(extent_x == imx4i_true_extent);
    MPI_Type_get_extent_x(imx4i_rsz, &lb_x, &extent_x);
    check(lb_x == INT_MAX);
    check(extent_x == -1024);

    /* MPI_Type_get_true_extent */
    MPI_Type_get_true_extent(imax_contig, &lb, &extent);
    check(lb == 0);
    check(extent == INT_MAX);
    MPI_Type_get_true_extent(four_ints, &lb, &extent);
    check(lb == 0);
    check(extent == 4 * sizeof(int));
    MPI_Type_get_true_extent(imx4i, &lb, &extent);
    check(lb == 0);
    if (sizeof(MPI_Aint) == sizeof(int))
        check(extent == MPI_UNDEFINED);
    else
        check(extent == imx4i_true_extent);
    MPI_Type_get_true_extent(imx4i_rsz, &lb, &extent);
    check(lb == 0);
    if (sizeof(MPI_Aint) == sizeof(int))
        check(extent == MPI_UNDEFINED);
    else
        check(extent == imx4i_true_extent);

    /* MPI_Type_get_true_extent_x */
    MPI_Type_get_true_extent_x(imax_contig, &lb_x, &extent_x);
    check(lb_x == 0);
    check(extent_x == INT_MAX);
    MPI_Type_get_true_extent_x(four_ints, &lb_x, &extent_x);
    check(lb_x == 0);
    check(extent_x == 4 * sizeof(int));
    MPI_Type_get_true_extent_x(imx4i, &lb_x, &extent_x);
    check(lb_x == 0);
    check(extent_x == imx4i_true_extent);
    MPI_Type_get_true_extent_x(imx4i_rsz, &lb_x, &extent_x);
    check(lb_x == 0);
    check(extent_x == imx4i_true_extent);


    /* MPI_{Status_set_elements,Get_elements}{,_x} */

    /* set simple */
    MPI_Status_set_elements(&status, MPI_INT, 10);
    MPI_Get_elements(&status, MPI_INT, &elements);
    MPI_Get_elements_x(&status, MPI_INT, &elements_x);
    MPI_Get_count(&status, MPI_INT, &count);
    check(elements == 10);
    check(elements_x == 10);
    check(count == 10);

    /* set_x simple */
    MPI_Status_set_elements_x(&status, MPI_INT, 10);
    MPI_Get_elements(&status, MPI_INT, &elements);
    MPI_Get_elements_x(&status, MPI_INT, &elements_x);
    MPI_Get_count(&status, MPI_INT, &count);
    check(elements == 10);
    check(elements_x == 10);
    check(count == 10);

    /* Sets elements corresponding to count=1 of the given MPI datatype, using
     * set_elements and set_elements_x.  Checks expected values are returned by
     * get_elements, get_elements_x, and get_count (including MPI_UNDEFINED
     * clipping) */
#define check_set_elements(type_, elts_)                          \
    do {                                                          \
        elements = elements_x = count = 0xfeedface;               \
        /* can't use legacy "set" for large element counts */     \
        if ((elts_) <= INT_MAX) {                                 \
            MPI_Status_set_elements(&status, (type_), 1);         \
            MPI_Get_elements(&status, (type_), &elements);        \
            MPI_Get_elements_x(&status, (type_), &elements_x);    \
            MPI_Get_count(&status, (type_), &count);              \
            check(elements == (elts_));                           \
            check(elements_x == (elts_));                         \
            check(count == 1);                                    \
        }                                                         \
                                                                  \
        elements = elements_x = count = 0xfeedface;               \
        MPI_Status_set_elements_x(&status, (type_), 1);           \
        MPI_Get_elements(&status, (type_), &elements);            \
        MPI_Get_elements_x(&status, (type_), &elements_x);        \
        MPI_Get_count(&status, (type_), &count);                  \
        if ((elts_) > INT_MAX) {                                  \
            check(elements == MPI_UNDEFINED);                     \
        }                                                         \
        else {                                                    \
            check(elements == (elts_));                           \
        }                                                         \
        check(elements_x == (elts_));                             \
        check(count == 1);                                        \
    } while (0)                                                   \

    check_set_elements(imax_contig, INT_MAX);
    check_set_elements(four_ints, 4);
    check_set_elements(imx4i, 4LL * (INT_MAX / 2));
    check_set_elements(imx4i_rsz, 4LL * (INT_MAX / 2));

  epilogue:
    if (imax_contig != MPI_DATATYPE_NULL)
        MPI_Type_free(&imax_contig);
    if (four_ints != MPI_DATATYPE_NULL)
        MPI_Type_free(&four_ints);
    if (imx4i != MPI_DATATYPE_NULL)
        MPI_Type_free(&imx4i);
    if (imx4i_rsz != MPI_DATATYPE_NULL)
        MPI_Type_free(&imx4i_rsz);

    MPI_Reduce((wrank == 0 ? MPI_IN_PLACE : &errs), &errs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (wrank == 0) {
        if (errs) {
            printf("found %d errors\n", errs);
        }
        else {
            printf(" No errors\n");
        }
    }

    MPI_Finalize();

    return 0;
}
