/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksa.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void int_with_lb_ub_test(void);
void contig_of_int_with_lb_ub_test(void);
void contig_negextent_of_int_with_lb_ub_test(void);
void vector_of_int_with_lb_ub_test(void);
void vector_blklen_of_int_with_lb_ub_test(void);
void vector_blklen_stride_of_int_with_lb_ub_test(void);
void vector_blklen_stride_negextent_of_int_with_lb_ub_test(void);
void vector_blklen_negstride_negextent_of_int_with_lb_ub_test(void);
void int_with_negextent_test(void);
void vector_blklen_negstride_of_int_with_lb_ub_test(void);

int main(int argc, char **argv)
{
    yaksa_init(NULL);

    int_with_lb_ub_test();
    contig_of_int_with_lb_ub_test();
    contig_negextent_of_int_with_lb_ub_test();
    vector_of_int_with_lb_ub_test();
    vector_blklen_of_int_with_lb_ub_test();
    vector_blklen_stride_of_int_with_lb_ub_test();
    vector_blklen_negstride_of_int_with_lb_ub_test();
    int_with_negextent_test();
    vector_blklen_stride_negextent_of_int_with_lb_ub_test();
    vector_blklen_negstride_negextent_of_int_with_lb_ub_test();

    yaksa_finalize();
    return 0;
}

void int_with_lb_ub_test(void)
{
    uintptr_t val;
    intptr_t lb, extent;
    yaksa_type_t tmptype, eviltype;

    yaksa_type_create_contig(4, YAKSA_TYPE__BYTE, NULL, &tmptype);
    yaksa_type_create_resized(tmptype, -3, 9, NULL, &eviltype);
    yaksa_type_get_size(eviltype, &val);
    assert(val == 4);

    yaksa_type_get_extent(eviltype, &lb, &extent);
    assert(lb == -3);
    assert(extent == 9);

    yaksa_type_get_true_extent(eviltype, &lb, &extent);
    assert(lb == 0);
    assert(extent == 4);

    yaksa_type_free(tmptype);
    yaksa_type_free(eviltype);
}

void contig_of_int_with_lb_ub_test(void)
{
    uintptr_t val;
    intptr_t lb, extent;
    yaksa_type_t tmptype, inttype, eviltype;

    yaksa_type_create_contig(4, YAKSA_TYPE__BYTE, NULL, &tmptype);
    yaksa_type_create_resized(tmptype, -3, 9, NULL, &inttype);
    yaksa_type_create_contig(3, inttype, NULL, &eviltype);

    yaksa_type_get_size(eviltype, &val);
    assert(val == 12);

    yaksa_type_get_extent(eviltype, &lb, &extent);
    assert(lb == -3);
    assert(extent == 27);

    yaksa_type_get_true_extent(eviltype, &lb, &extent);
    assert(lb == 0);
    assert(extent == 22);

    yaksa_type_free(tmptype);
    yaksa_type_free(inttype);
    yaksa_type_free(eviltype);
}

void contig_negextent_of_int_with_lb_ub_test(void)
{
    uintptr_t val;
    intptr_t lb, extent;
    yaksa_type_t tmptype, inttype, eviltype;

    yaksa_type_create_contig(4, YAKSA_TYPE__BYTE, NULL, &tmptype);
    yaksa_type_create_resized(tmptype, 6, -9, NULL, &inttype);
    yaksa_type_create_contig(3, inttype, NULL, &eviltype);

    yaksa_type_get_size(eviltype, &val);
    assert(val == 12);

    yaksa_type_get_extent(eviltype, &lb, &extent);
    assert(lb == -12);
    assert(extent == 9);

    yaksa_type_get_true_extent(eviltype, &lb, &extent);
    assert(lb == -18);
    assert(extent == 22);

    yaksa_type_free(tmptype);
    yaksa_type_free(inttype);
    yaksa_type_free(eviltype);
}

void vector_of_int_with_lb_ub_test(void)
{
    uintptr_t val;
    intptr_t lb, extent;
    yaksa_type_t tmptype, inttype, eviltype;

    yaksa_type_create_contig(4, YAKSA_TYPE__BYTE, NULL, &tmptype);
    yaksa_type_create_resized(tmptype, -3, 9, NULL, &inttype);
    yaksa_type_create_vector(3, 1, 1, inttype, NULL, &eviltype);

    yaksa_type_get_size(eviltype, &val);
    assert(val == 12);

    yaksa_type_get_extent(eviltype, &lb, &extent);
    assert(lb == -3);
    assert(extent == 27);

    yaksa_type_get_true_extent(eviltype, &lb, &extent);
    assert(lb == 0);
    assert(extent == 22);

    yaksa_type_free(tmptype);
    yaksa_type_free(inttype);
    yaksa_type_free(eviltype);
}

void vector_blklen_of_int_with_lb_ub_test(void)
{
    uintptr_t val;
    intptr_t lb, extent;
    yaksa_type_t tmptype, inttype, eviltype;

    yaksa_type_create_contig(4, YAKSA_TYPE__BYTE, NULL, &tmptype);
    yaksa_type_create_resized(tmptype, -3, 9, NULL, &inttype);
    yaksa_type_create_vector(3, 4, 1, inttype, NULL, &eviltype);

    yaksa_type_get_size(eviltype, &val);
    assert(val == 48);

    yaksa_type_get_extent(eviltype, &lb, &extent);
    assert(lb == -3);
    assert(extent == 54);

    yaksa_type_get_true_extent(eviltype, &lb, &extent);
    assert(lb == 0);
    assert(extent == 49);

    yaksa_type_free(tmptype);
    yaksa_type_free(inttype);
    yaksa_type_free(eviltype);
}

void vector_blklen_stride_of_int_with_lb_ub_test(void)
{
    uintptr_t val;
    intptr_t lb, extent;
    yaksa_type_t tmptype, inttype, eviltype;

    yaksa_type_create_contig(4, YAKSA_TYPE__BYTE, NULL, &tmptype);
    yaksa_type_create_resized(tmptype, -3, 9, NULL, &inttype);
    yaksa_type_create_vector(3, 4, 5, inttype, NULL, &eviltype);

    yaksa_type_get_size(eviltype, &val);
    assert(val == 48);

    yaksa_type_get_extent(eviltype, &lb, &extent);
    assert(lb == -3);
    assert(extent == 126);

    yaksa_type_get_true_extent(eviltype, &lb, &extent);
    assert(lb == 0);
    assert(extent == 121);

    yaksa_type_free(tmptype);
    yaksa_type_free(inttype);
    yaksa_type_free(eviltype);
}

void vector_blklen_negstride_of_int_with_lb_ub_test(void)
{
    uintptr_t val;
    intptr_t lb, extent;
    yaksa_type_t tmptype, inttype, eviltype;

    yaksa_type_create_contig(4, YAKSA_TYPE__BYTE, NULL, &tmptype);
    yaksa_type_create_resized(tmptype, -3, 9, NULL, &inttype);
    yaksa_type_create_vector(3, 4, -5, inttype, NULL, &eviltype);

    yaksa_type_get_size(eviltype, &val);
    assert(val == 48);

    yaksa_type_get_extent(eviltype, &lb, &extent);
    assert(lb == -93);
    assert(extent == 126);

    yaksa_type_get_true_extent(eviltype, &lb, &extent);
    assert(lb == -90);
    assert(extent == 121);

    yaksa_type_free(tmptype);
    yaksa_type_free(inttype);
    yaksa_type_free(eviltype);
}

void int_with_negextent_test(void)
{
    uintptr_t val;
    intptr_t lb, extent;
    yaksa_type_t tmptype, eviltype;

    yaksa_type_create_contig(4, YAKSA_TYPE__BYTE, NULL, &tmptype);
    yaksa_type_create_resized(tmptype, 6, -9, NULL, &eviltype);

    yaksa_type_get_size(eviltype, &val);
    assert(val == 4);

    yaksa_type_get_extent(eviltype, &lb, &extent);
    assert(lb == 6);
    assert(extent == -9);

    yaksa_type_get_true_extent(eviltype, &lb, &extent);
    assert(lb == 0);
    assert(extent == 4);

    yaksa_type_free(tmptype);
    yaksa_type_free(eviltype);
}

void vector_blklen_stride_negextent_of_int_with_lb_ub_test(void)
{
    uintptr_t val;
    intptr_t lb, extent;
    yaksa_type_t tmptype, inttype, eviltype;

    yaksa_type_create_contig(4, YAKSA_TYPE__BYTE, NULL, &tmptype);
    yaksa_type_create_resized(tmptype, 6, -9, NULL, &inttype);
    yaksa_type_create_vector(3, 4, 5, inttype, NULL, &eviltype);

    yaksa_type_get_size(eviltype, &val);
    assert(val == 48);

    yaksa_type_get_extent(eviltype, &lb, &extent);
    assert(lb == -111);
    assert(extent == 108);

    yaksa_type_get_true_extent(eviltype, &lb, &extent);
    assert(lb == -117);
    assert(extent == 121);

    yaksa_type_free(tmptype);
    yaksa_type_free(inttype);
    yaksa_type_free(eviltype);
}

void vector_blklen_negstride_negextent_of_int_with_lb_ub_test(void)
{
    uintptr_t val;
    intptr_t lb, extent;
    yaksa_type_t tmptype, inttype, eviltype;

    yaksa_type_create_contig(4, YAKSA_TYPE__BYTE, NULL, &tmptype);
    yaksa_type_create_resized(tmptype, 6, -9, NULL, &inttype);
    yaksa_type_create_vector(3, 4, -5, inttype, NULL, &eviltype);

    yaksa_type_get_size(eviltype, &val);
    assert(val == 48);

    yaksa_type_get_extent(eviltype, &lb, &extent);
    assert(lb == -21);
    assert(extent == 108);

    yaksa_type_get_true_extent(eviltype, &lb, &extent);
    assert(lb == -27);
    assert(extent == 121);

    yaksa_type_free(tmptype);
    yaksa_type_free(inttype);
    yaksa_type_free(eviltype);
}
