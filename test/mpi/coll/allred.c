/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*      Warning - this test will fail for MPI_PROD & maybe MPI_SUM
 *        if more than 10 MPI processes are used.  Loss of precision
 *        will occur as the number of processors is increased.
 */

#include "mpitest.h"
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "mtest_dtp.h"
/* For simplicity, we precalculate solutions in integers. Use "long" type
 * to prevent intermediate overflows, especially during get_pow(). On the
 * other hand, the truncations during assignment and during allreduce
 * operation are usually consistent.
 */
#define LONG long long

struct int_test {
    int a;
    int b;
};
struct long_test {
    long a;
    int b;
};
struct short_test {
    short a;
    int b;
};
struct float_test {
    float a;
    int b;
};
struct double_test {
    double a;
    int b;
};

enum type_category {
    CATEGORY_INT,
    CATEGORY_FLOAT,
    CATEGORY_COMPLEX,
    CATEGORY_PAIR,
};

/* global variables */

static int rank, size;
static mtest_mem_type_e memtype;
static int count;

static MPI_Datatype int_types[] = {
    MPI_INT,
    MPI_LONG,
    MPI_SHORT,
    MPI_UNSIGNED_SHORT,
    MPI_UNSIGNED,
    MPI_UNSIGNED_LONG,
    MPI_UNSIGNED_CHAR,
#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
    MPI_INT8_T,
    MPI_INT16_T,
    MPI_INT32_T,
    MPI_INT64_T,
    MPI_UINT8_T,
    MPI_UINT16_T,
    MPI_UINT32_T,
    MPI_UINT64_T,
    MPI_AINT,
    MPI_OFFSET,
#endif
#if MTEST_HAVE_MIN_MPI_VERSION(3,0)
    MPI_COUNT,
#endif
};

static MPI_Datatype float_types[] = {
    MPI_FLOAT,
    MPI_DOUBLE,
};

static MPI_Datatype byte_types[] = {
    MPI_BYTE,
};

#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
static MPI_Datatype complex_types[] = {
    MPI_C_FLOAT_COMPLEX,
    MPI_C_DOUBLE_COMPLEX,
#ifdef HAVE_LONG_DOUBLE__COMPLEX
    MPI_C_LONG_DOUBLE_COMPLEX,
#endif
};
#endif

#if MTEST_HAVE_MIN_MPI_VERSION(2,2) && defined(HAVE__BOOL)
static MPI_Datatype logical_types[] = {
    MPI_C_BOOL,
};
#endif

static MPI_Datatype pair_types[] = {
    MPI_2INT,
    MPI_LONG_INT,
    MPI_SHORT_INT,
    MPI_FLOAT_INT,
    MPI_DOUBLE_INT
};

#define MAX_TYPE_SIZE 16
static void *in, *out, *sol;
static void *in_d, *out_d;

/* internal routines */
static const char *mpi_op2str(MPI_Op op)
{
    return ((op == MPI_SUM) ? "MPI_SUM" :
            (op == MPI_PROD) ? "MPI_PROD" :
            (op == MPI_MAX) ? "MPI_MAX" :
            (op == MPI_MIN) ? "MPI_MIN" :
            (op == MPI_LOR) ? "MPI_LOR" :
            (op == MPI_LXOR) ? "MPI_LXOR" :
            (op == MPI_LAND) ? "MPI_LAND" :
            (op == MPI_BOR) ? "MPI_BOR" :
            (op == MPI_BAND) ? "MPI_BAND" :
            (op == MPI_BXOR) ? "MPI_BXOR" :
            (op == MPI_MAXLOC) ? "MPI_MAXLOC" : (op == MPI_MINLOC) ? "MPI_MINLOC" : "MPI_NO_OP");
}

/* calloc to avoid spurious valgrind warnings when "type" has padding bytes */
static void malloc_in_out_sol(int memtype, int rank)
{
    MTestCalloc(count * MAX_TYPE_SIZE, memtype, &in, &in_d, rank);
    MTestCalloc(count * MAX_TYPE_SIZE, memtype, &out, &out_d, rank);
    sol = calloc(count, MAX_TYPE_SIZE);
}

static void free_in_out_sol(int memtype)
{
    MTestFree(memtype, in, in_d);
    MTestFree(memtype, out, out_d);
    free(sol);
}

static int get_mpi_type_size(MPI_Datatype mpi_type)
{
    MPI_Aint lb, extent;
    MPI_Type_get_extent(mpi_type, &lb, &extent);
    return extent;
}

static void set_int_value(void *p, int type_size, LONG val)
{
    switch (type_size) {
        case 1:
            *(int8_t *) p = (int8_t) val;
            break;
        case 2:
            *(int16_t *) p = (int16_t) val;
            break;
        case 4:
            *(int32_t *) p = (int32_t) val;
            break;
        case 8:
            *(int64_t *) p = (int64_t) val;
            break;
        default:
            assert(0);
    }
}

static void set_float_value(void *p, int type_size, LONG val)
{
    if (type_size == 4) {
        *(float *) p = (float) val;
    } else if (type_size == 8) {
        *(double *) p = (double) val;
    } else {
        assert(0);
    }
}

static void set_complex_value(void *p, int type_size, LONG val)
{
    if (type_size == 8) {
        *(float _Complex *) p = (float _Complex) val;
    } else if (type_size == 16) {
        *(double _Complex *) p = (double _Complex) val;
#ifdef HAVE_LONG_DOUBLE__COMPLEX
    } else if (type_size == 32) {
        *(long double _Complex *) p = (long double _Complex) val;
#endif
    } else {
        assert(0);
    }
}

static void set_category_value(int category, void *p, int type_size, LONG val)
{
    if (category == CATEGORY_INT) {
        set_int_value(p, type_size, val);
    } else if (category == CATEGORY_FLOAT) {
        set_float_value(p, type_size, val);
    } else if (category == CATEGORY_COMPLEX) {
        set_complex_value(p, type_size, val);
    } else {
        assert(0);
    }
}

static void set_index_const(MPI_Datatype mpi_type, int category, void *arr, int val, int count)
{
    int type_size = get_mpi_type_size(mpi_type);
    char *p = arr;
    for (int i = 0; i < count; i++) {
        set_category_value(category, p, type_size, val);
        p += type_size;
    }
}

static void set_index_sum(MPI_Datatype mpi_type, int category, void *arr, int val, int count)
{
    int type_size = get_mpi_type_size(mpi_type);
    char *p = arr;
    for (int i = 0; i < count; i++) {
        set_category_value(category, p, type_size, i + val);
        p += type_size;
    }
}

static void set_index_factor(MPI_Datatype mpi_type, int category, void *arr, int val, int count)
{
    int type_size = get_mpi_type_size(mpi_type);
    char *p = arr;
    for (int i = 0; i < count; i++) {
        set_category_value(category, p, type_size, i * val);
        p += type_size;
    }
}

static LONG get_pow(int base, int n)
{
    LONG ans = 1;
    for (int i = 0; i < n; i++) {
        ans *= base;
    }
    return ans;
}

static void set_index_power(MPI_Datatype mpi_type, int category, void *arr, int val, int count)
{
    int type_size = get_mpi_type_size(mpi_type);
    char *p = arr;
    for (int i = 0; i < count; i++) {
        set_category_value(category, p, type_size, get_pow(i, val));
        p += type_size;
    }
}

#define SET_PAIR(type) \
    do { \
        struct type ## _test *pp = p; \
        pp->a = val_a; \
        pp->b = val_b; \
    } while (0)

static void set_pair_val(MPI_Datatype mpi_type, void *p, int val_a, int val_b)
{
    if (mpi_type == MPI_2INT) {
        SET_PAIR(int);
    } else if (mpi_type == MPI_LONG_INT) {
        SET_PAIR(long);
    } else if (mpi_type == MPI_SHORT_INT) {
        SET_PAIR(short);
    } else if (mpi_type == MPI_FLOAT_INT) {
        SET_PAIR(float);
    } else if (mpi_type == MPI_DOUBLE_INT) {
        SET_PAIR(double);
    } else {
        assert(0);
    }
}

static void set_index_pair_const(MPI_Datatype mpi_type, void *arr, int val_a, int val_b, int count)
{
    int type_size = get_mpi_type_size(mpi_type);
    char *p = arr;
    for (int i = 0; i < count; i++) {
        set_pair_val(mpi_type, p, val_a, val_b);
        p += type_size;
    }
}

static void set_index_pair_sum(MPI_Datatype mpi_type, void *arr, int val_a, int val_b, int count)
{
    int type_size = get_mpi_type_size(mpi_type);
    char *p = arr;
    for (int i = 0; i < count; i++) {
        set_pair_val(mpi_type, p, i + val_a, val_b);
        p += type_size;
    }
}

static int cmp_float(void *p, void *q, int type_size)
{
    if (type_size == 4) {
        if (*(float *) p != *(float *) q) {
            return 1;
        }
    } else if (type_size == 8) {
        if (*(double *) p != *(double *) q) {
            return 1;
        }
    } else {
        assert(0);
    }
    return 0;
}

static int cmp_complex(void *p, void *q, int type_size)
{
    if (type_size == 8) {
        if (*(float _Complex *) p != *(float _Complex *) q) {
            return 1;
        }
    } else if (type_size == 16) {
        if (*(double _Complex *) p != *(double _Complex *) q) {
            return 1;
        }
#ifdef HAVE_LONG_DOUBLE__COMPLEX
    } else if (type_size == 32) {
        if (*(long double _Complex *) p != *(long double _Complex *) q) {
            return 1;
        }
#endif
    } else {
        assert(0);
    }
    return 0;
}

static int cmp_int(void *p, void *q, int type_size)
{
    if (memcmp(p, q, type_size) == 0) {
        return 0;
    } else {
        return 1;
    }
}

#define CMP_PAIR(type) \
    do { \
        struct type ## _test *pp, *qq; \
        pp = p; \
        qq = q; \
        if (pp->a == qq->a && pp->b == qq->b) { \
            return 0; \
        } else { \
            return 1; \
        } \
    } while (0)

static int cmp_pair(void *p, void *q, MPI_Datatype mpi_type)
{
    if (mpi_type == MPI_2INT) {
        CMP_PAIR(int);
    } else if (mpi_type == MPI_LONG_INT) {
        CMP_PAIR(long);
    } else if (mpi_type == MPI_SHORT_INT) {
        CMP_PAIR(short);
    } else if (mpi_type == MPI_FLOAT_INT) {
        CMP_PAIR(float);
    } else if (mpi_type == MPI_DOUBLE_INT) {
        CMP_PAIR(double);
    } else {
        assert(0);
    }
    return 1;
}

static int cmp_arr_value(MPI_Datatype mpi_type, int category, void *arr1, void *arr2, int count)
{
    int err = 0;

    int type_size = get_mpi_type_size(mpi_type);
    char *p = arr1;
    char *q = arr2;
    for (int i = 0; i < count; i++) {
        if (category == CATEGORY_INT) {
            err += cmp_int(p, q, type_size);
        } else if (category == CATEGORY_FLOAT) {
            err += cmp_float(p, q, type_size);
        } else if (category == CATEGORY_COMPLEX) {
            err += cmp_complex(p, q, type_size);
        } else if (category == CATEGORY_PAIR) {
            err += cmp_pair(p, q, mpi_type);
        } else {
            assert(0);
        }
        p += type_size;
        q += type_size;
    }
    return err;
}

static void report_error(int rank, MPI_Datatype mpi_type, MPI_Op op)
{
    char name[MPI_MAX_OBJECT_NAME] = { 0 };
    int len = 0;

    MPI_Type_get_name(mpi_type, name, &len);
    printf("(%d) Error for type %s and op %s\n", rank, name, mpi_op2str(op));
}

/* The logic on the error check on MPI_Allreduce assumes that all
   MPI_Allreduce routines return a failure if any do - this is sufficient
   for MPI implementations that reject some of the valid op/datatype pairs
   (and motivated this addition, as some versions of the IBM MPI
   failed in just this way).
*/

static int allreduce_and_check(MPI_Datatype mpi_type, int category, MPI_Op op, int memtype)
{
    int lerrcnt = 0;

    int type_size = get_mpi_type_size(mpi_type);
    MTestCopyContent(in, in_d, count * type_size, memtype);
    int rc = MPI_Allreduce(in_d, out_d, count, mpi_type, op, MPI_COMM_WORLD);
    MTestCopyContent(out_d, out, count * type_size, memtype);

    if (rc) {
        lerrcnt++;
        MTestPrintError(rc);
    } else {
        lerrcnt += cmp_arr_value(mpi_type, category, out, sol, count);
    }

    if (lerrcnt > 0) {
        report_error(rank, mpi_type, op);
    }
    return lerrcnt;
}

static int sum_test_1(MPI_Datatype mpi_type, int category, int memtype)
{
    set_index_sum(mpi_type, category, in, 0, count);
    set_index_factor(mpi_type, category, sol, size, count);
    set_index_const(mpi_type, category, out, 0, count);
    return allreduce_and_check(mpi_type, category, MPI_SUM, memtype);
}

static int prod_test_1(MPI_Datatype mpi_type, int category, int memtype)
{
    set_index_sum(mpi_type, category, in, 0, count);
    set_index_power(mpi_type, category, sol, size, count);
    set_index_const(mpi_type, category, out, 0, count);
    return allreduce_and_check(mpi_type, category, MPI_PROD, memtype);
}

static int max_test_1(MPI_Datatype mpi_type, int category, int memtype)
{
    set_index_sum(mpi_type, category, in, rank, count);
    set_index_sum(mpi_type, category, sol, size - 1, count);
    set_index_const(mpi_type, category, out, 0, count);
    return allreduce_and_check(mpi_type, category, MPI_MAX, memtype);
}

static int min_test_1(MPI_Datatype mpi_type, int category, int memtype)
{
    set_index_sum(mpi_type, category, in, rank, count);
    set_index_sum(mpi_type, category, sol, 0, count);
    set_index_const(mpi_type, category, out, 0, count);
    return allreduce_and_check(mpi_type, category, MPI_MIN, memtype);
}

#define set_const_test(val1, val2, val3)    \
    do { \
        set_index_const(mpi_type, category, in, val1, count); \
        set_index_const(mpi_type, category, sol, val2, count); \
        set_index_const(mpi_type, category, out, val3, count); \
    } while (0)

static int lor_test_1(MPI_Datatype mpi_type, int category, int memtype)
{
    set_const_test((rank & 0x1), (size > 1), 0);
    return allreduce_and_check(mpi_type, category, MPI_LOR, memtype);
}

static int lor_test_2(MPI_Datatype mpi_type, int category, int memtype)
{
    set_const_test(0, 0, 0);
    return allreduce_and_check(mpi_type, category, MPI_LOR, memtype);
}

static int lxor_test_1(MPI_Datatype mpi_type, int category, int memtype)
{
    set_const_test((rank == 1), (size > 1), 0);
    return allreduce_and_check(mpi_type, category, MPI_LXOR, memtype);
}

static int lxor_test_2(MPI_Datatype mpi_type, int category, int memtype)
{
    set_const_test(0, 0, 0);
    return allreduce_and_check(mpi_type, category, MPI_LXOR, memtype);
}

static int lxor_test_3(MPI_Datatype mpi_type, int category, int memtype)
{
    set_const_test(1, (size & 0x1), 0);
    return allreduce_and_check(mpi_type, category, MPI_LXOR, memtype);
}

static int land_test_1(MPI_Datatype mpi_type, int category, int memtype)
{
    set_const_test((rank & 0x1), 0, 0);
    return allreduce_and_check(mpi_type, category, MPI_LAND, memtype);
}

static int land_test_2(MPI_Datatype mpi_type, int category, int memtype)
{
    set_const_test(1, 1, 0);
    return allreduce_and_check(mpi_type, category, MPI_LAND, memtype);
}

static int bor_test_1(MPI_Datatype mpi_type, int category, int memtype)
{
    set_const_test((rank & 0x3), ((size < 3) ? size - 1 : 0x3), 0);
    return allreduce_and_check(mpi_type, category, MPI_BOR, memtype);
}

static int bxor_test_1(MPI_Datatype mpi_type, int category, int memtype)
{
    set_const_test((rank == 1) * 0xf0, (size > 1) * 0xf0, 0);
    return allreduce_and_check(mpi_type, category, MPI_BXOR, memtype);
}

static int bxor_test_2(MPI_Datatype mpi_type, int category, int memtype)
{
    set_const_test(0, 0, 0);
    return allreduce_and_check(mpi_type, category, MPI_BXOR, memtype);
}

static int bxor_test_3(MPI_Datatype mpi_type, int category, int memtype)
{
    set_const_test(~0, (size & 0x1) ? ~0 : 0, 0);
    return allreduce_and_check(mpi_type, category, MPI_BXOR, memtype);
}

static int band_test_1(MPI_Datatype mpi_type, int category, int memtype)
{
    if (rank == size - 1) {
        set_index_sum(mpi_type, category, in, 0, count);
    } else {
        set_index_const(mpi_type, category, in, ~0, count);
    }
    set_index_sum(mpi_type, category, sol, 0, count);
    set_index_const(mpi_type, category, out, 0, count);
    return allreduce_and_check(mpi_type, category, MPI_BAND, memtype);
}

static int band_test_2(MPI_Datatype mpi_type, int category, int memtype)
{
    if (rank == size - 1) {
        set_index_sum(mpi_type, category, in, 0, count);
    } else {
        set_index_const(mpi_type, category, in, 0, count);
    }
    set_index_const(mpi_type, category, sol, 0, count);
    set_index_const(mpi_type, category, out, 0, count);
    return allreduce_and_check(mpi_type, category, MPI_BAND, memtype);
}

static int maxloc_test(MPI_Datatype mpi_type, int memtype)
{
    set_index_pair_sum(mpi_type, in, rank, rank, count);
    set_index_pair_sum(mpi_type, sol, size - 1, size - 1, count);
    set_index_pair_const(mpi_type, out, 0, -1, count);
    return allreduce_and_check(mpi_type, CATEGORY_PAIR, MPI_MAXLOC, memtype);
}

static int minloc_test(MPI_Datatype mpi_type, int memtype)
{
    set_index_pair_sum(mpi_type, in, rank, rank, count);
    set_index_pair_sum(mpi_type, sol, 0, 0, count);
    set_index_pair_const(mpi_type, out, 0, 0, count);
    return allreduce_and_check(mpi_type, CATEGORY_PAIR, MPI_MINLOC, memtype);
}

static int test_allred(mtest_mem_type_e evenmem, mtest_mem_type_e oddmem)
{
    int errs = 0;

    /* Set errors return so that we can provide better information
     * should a routine reject one of the operand/datatype pairs */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    assert(count > 0);

    if (rank == 0) {
        MTestPrintfMsg(1, "./allred -evenmemtype=%s -oddmemtype=%s\n",
                       MTest_memtype_name(evenmem), MTest_memtype_name(oddmem));
    }

    if (rank % 2 == 0)
        memtype = evenmem;
    else
        memtype = oddmem;

    malloc_in_out_sol(memtype, rank);

    int num_int_types = sizeof(int_types) / sizeof(MPI_Datatype);
    for (int i = 0; i < num_int_types; i++) {
        MPI_Datatype mpi_type = int_types[i];
        int category = CATEGORY_INT;
        errs += sum_test_1(mpi_type, category, memtype);
        errs += prod_test_1(mpi_type, category, memtype);
        errs += max_test_1(mpi_type, category, memtype);
        errs += min_test_1(mpi_type, category, memtype);
        errs += lor_test_1(mpi_type, category, memtype);
        errs += lor_test_2(mpi_type, category, memtype);
        errs += lxor_test_1(mpi_type, category, memtype);
        errs += lxor_test_2(mpi_type, category, memtype);
        errs += lxor_test_3(mpi_type, category, memtype);
        errs += land_test_1(mpi_type, category, memtype);
        errs += land_test_2(mpi_type, category, memtype);
        errs += bor_test_1(mpi_type, category, memtype);
        errs += band_test_1(mpi_type, category, memtype);
        errs += band_test_2(mpi_type, category, memtype);
        errs += bxor_test_1(mpi_type, category, memtype);
        errs += bxor_test_2(mpi_type, category, memtype);
        errs += bxor_test_3(mpi_type, category, memtype);
    }

    int num_float_types = sizeof(float_types) / sizeof(MPI_Datatype);
    for (int i = 0; i < num_float_types; i++) {
        MPI_Datatype mpi_type = float_types[i];
        int category = CATEGORY_FLOAT;
        errs += sum_test_1(mpi_type, category, memtype);
        errs += prod_test_1(mpi_type, category, memtype);
        errs += max_test_1(mpi_type, category, memtype);
        errs += min_test_1(mpi_type, category, memtype);
    }

    int num_byte_types = sizeof(byte_types) / sizeof(MPI_Datatype);
    for (int i = 0; i < num_byte_types; i++) {
        MPI_Datatype mpi_type = byte_types[i];
        int category = CATEGORY_INT;
        errs += bor_test_1(mpi_type, category, memtype);
        errs += band_test_1(mpi_type, category, memtype);
        errs += band_test_2(mpi_type, category, memtype);
        errs += bxor_test_1(mpi_type, category, memtype);
        errs += bxor_test_2(mpi_type, category, memtype);
        errs += bxor_test_3(mpi_type, category, memtype);
    }

#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
    int num_complex_types = sizeof(complex_types) / sizeof(MPI_Datatype);
    for (int i = 0; i < num_byte_types; i++) {
        MPI_Datatype mpi_type = complex_types[i];
        int category = CATEGORY_COMPLEX;
        if (mpi_type != MPI_DATATYPE_NULL) {
            errs += sum_test_1(mpi_type, category, memtype);
            errs += prod_test_1(mpi_type, category, memtype);
        }
    }
#endif

#if MTEST_HAVE_MIN_MPI_VERSION(2,2) && defined(HAVE__BOOL)
    int num_logical_types = sizeof(logical_types) / sizeof(MPI_Datatype);
    for (int i = 0; i < num_byte_types; i++) {
        MPI_Datatype mpi_type = logical_types[i];
        int category = CATEGORY_INT;
        if (mpi_type != MPI_DATATYPE_NULL) {
            errs += lor_test_1(mpi_type, category, memtype);
            errs += lor_test_2(mpi_type, category, memtype);
            errs += lxor_test_1(mpi_type, category, memtype);
            errs += lxor_test_2(mpi_type, category, memtype);
            errs += lxor_test_3(mpi_type, category, memtype);
            errs += land_test_1(mpi_type, category, memtype);
            errs += land_test_2(mpi_type, category, memtype);
        }
    }
#endif

    int num_pair_types = sizeof(pair_types) / sizeof(MPI_Datatype);
    for (int i = 0; i < num_pair_types; i++) {
        errs += maxloc_test(pair_types[i], memtype);
        errs += minloc_test(pair_types[i], memtype);
    }

    free_in_out_sol(memtype);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_ARE_FATAL);

    return errs;
}

int main(int argc, char **argv)
{
    int errs = 0;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        fprintf(stderr, "At least 2 processes required\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    struct dtp_args dtp_args;
    dtp_args_init(&dtp_args, MTEST_COLL_COUNT, argc, argv);
    while (dtp_args_get_next(&dtp_args)) {
        count = dtp_args.count;
        errs += test_allred(dtp_args.u.coll.evenmem, dtp_args.u.coll.oddmem);
    }
    dtp_args_finalize(&dtp_args);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
