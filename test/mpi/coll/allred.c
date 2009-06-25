/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "mpitest.h"

int count, size, rank;
int cerrcnt;

struct int_test { int a; int b; };
struct long_test { long a; int b; };
struct short_test { short a; int b; };
struct float_test { float a; int b; };
struct double_test { double a; int b; };

#define mpi_type2str(type)                                 \
    ((type == MPI_INT) ? "MPI_INT" :                       \
     (type == MPI_LONG) ? "MPI_LONG" :                     \
     (type == MPI_SHORT) ? "MPI_SHORT" :                   \
     (type == MPI_UNSIGNED_SHORT) ? "MPI_UNSIGNED_SHORT" : \
     (type == MPI_UNSIGNED) ? "MPI_UNSIGNED" :             \
     (type == MPI_UNSIGNED_LONG) ? "MPI_UNSIGNED_LONG" :   \
     (type == MPI_FLOAT) ? "MPI_FLOAT" :                   \
     (type == MPI_DOUBLE) ? "MPI_DOUBLE" :                 \
     (type == MPI_2INT) ? "MPI_2INT" :                     \
     (type == MPI_LONG_INT) ? "MPI_LONG_INT" :             \
     (type == MPI_SHORT_INT) ? "MPI_SHORT_INT" :           \
     (type == MPI_FLOAT_INT) ? "MPI_FLOAT_INT" :           \
     (type == MPI_DOUBLE_INT) ? "MPI_DOUBLE_INT" :         \
     "MPI_NULL_DATATYPE")

#define mpi_op2str(op)                   \
    ((op == MPI_SUM) ? "MPI_SUM" :       \
     (op == MPI_PROD) ? "MPI_PROD" :     \
     (op == MPI_MAX) ? "MPI_MAX" :       \
     (op == MPI_MIN) ? "MPI_MIN" :       \
     (op == MPI_LOR) ? "MPI_LOR" :       \
     (op == MPI_LXOR) ? "MPI_LXOR" :     \
     (op == MPI_LAND) ? "MPI_LAND" :     \
     (op == MPI_BOR) ? "MPI_BOR" :       \
     (op == MPI_BAND) ? "MPI_BAND" :     \
     (op == MPI_BXOR) ? "MPI_BXOR" :     \
     (op == MPI_MAXLOC) ? "MPI_MAXLOC" : \
     (op == MPI_MINLOC) ? "MPI_MINLOC" : \
     "MPI_NO_OP")

#define DECL_MALLOC_IN_OUT_SOL(type)                 \
    type *in, *out, *sol;                            \
    in = (type *) malloc(count * sizeof(type));      \
    out = (type *) malloc(count * sizeof(type));     \
    sol = (type *) malloc(count * sizeof(type));

#define SET_INDEX_CONST(arr, val)               \
    {                                           \
        int i;                                  \
        for (i = 0; i < count; i++)             \
            arr[i] = val;                       \
    }

#define SET_INDEX_SUM(arr, val)                 \
    {                                           \
        int i;                                  \
        for (i = 0; i < count; i++)             \
            arr[i] = i + val;                   \
    }

#define SET_INDEX_FACTOR(arr, val)              \
    {                                           \
        int i;                                  \
        for (i = 0; i < count; i++)             \
            arr[i] = i * (val);                 \
    }

#define SET_INDEX_POWER(arr, val)               \
    {                                           \
        int i, j;                               \
        for (i = 0; i < count; i++) {           \
            (arr)[i] = 1;                       \
            for (j = 0; j < (val); j++)         \
                arr[i] *= i;                    \
        }                                       \
    }

#define ERROR_CHECK_AND_FREE(lerrcnt, mpi_type, mpi_op)                 \
    {                                                                   \
        if (lerrcnt) {                                                  \
            fprintf(stderr, "(%d) Error for type %s and op %s\n",       \
                    rank, mpi_type2str(mpi_type), mpi_op2str(mpi_op));  \
        }                                                               \
        free(in); free(out); free(sol);                                 \
    }

#define ALLREDUCE_AND_FREE(mpi_type, mpi_op, in, out, sol)              \
    {                                                                   \
        int i, lerrcnt = 0;                                             \
        MPI_Allreduce(in, out, count, mpi_type, mpi_op, MPI_COMM_WORLD); \
        for (i = 0; i < count; i++) {                                   \
            if (out[i] != sol[i]) {                                     \
                cerrcnt++;                                              \
                lerrcnt++;                                              \
            }                                                           \
        }                                                               \
        ERROR_CHECK_AND_FREE(lerrcnt, mpi_type, mpi_op);                \
    }

#define STRUCT_ALLREDUCE_AND_FREE(mpi_type, mpi_op, in, out, sol)       \
    {                                                                   \
        int i, lerrcnt = 0;                                             \
        MPI_Allreduce(in, out, count, mpi_type, mpi_op, MPI_COMM_WORLD); \
        for (i = 0; i < count; i++) {                                   \
            if ((out[i].a != sol[i].a) || (out[i].b != sol[i].b)) {     \
                cerrcnt++;                                              \
                lerrcnt++;                                              \
            }                                                           \
        }                                                               \
        ERROR_CHECK_AND_FREE(lerrcnt, mpi_type, mpi_op);                \
    }

#define SET_INDEX_STRUCT_CONST(arr, val, el)                    \
    {                                                           \
        int i;                                                  \
        for (i = 0; i < count; i++)                             \
            arr[i].el = val;                                    \
    }

#define SET_INDEX_STRUCT_SUM(arr, val, el)                      \
    {                                                           \
        int i;                                                  \
        for (i = 0; i < count; i++)                             \
            arr[i].el = i + (val);                              \
    }

#define sum_test1(type, mpi_type)                                       \
    {                                                                   \
        DECL_MALLOC_IN_OUT_SOL(type);                                   \
        SET_INDEX_SUM(in, 0);                                           \
        SET_INDEX_FACTOR(sol, size);                                    \
        SET_INDEX_CONST(out, 0);                                        \
        ALLREDUCE_AND_FREE(mpi_type, MPI_SUM, in, out, sol);            \
    }

#define prod_test1(type, mpi_type)                                      \
    {                                                                   \
        DECL_MALLOC_IN_OUT_SOL(type);                                   \
        SET_INDEX_SUM(in, 0);                                           \
        SET_INDEX_POWER(sol, size);                                     \
        SET_INDEX_CONST(out, 0);                                        \
        ALLREDUCE_AND_FREE(mpi_type, MPI_PROD, in, out, sol);           \
    }

#define max_test1(type, mpi_type)                                       \
    {                                                                   \
        DECL_MALLOC_IN_OUT_SOL(type);                                   \
        SET_INDEX_SUM(in, rank);                                        \
        SET_INDEX_SUM(sol, size - 1);                                   \
        SET_INDEX_CONST(out, 0);                                        \
        ALLREDUCE_AND_FREE(mpi_type, MPI_MAX, in, out, sol);            \
    }

#define min_test1(type, mpi_type)                                       \
    {                                                                   \
        DECL_MALLOC_IN_OUT_SOL(type);                                   \
        SET_INDEX_SUM(in, rank);                                        \
        SET_INDEX_SUM(sol, 0);                                          \
        SET_INDEX_CONST(out, 0);                                        \
        ALLREDUCE_AND_FREE(mpi_type, MPI_MIN, in, out, sol);            \
    }

#define const_test(type, mpi_type, mpi_op, val1, val2, val3)            \
    {                                                                   \
        DECL_MALLOC_IN_OUT_SOL(type);                                   \
        SET_INDEX_CONST(in, (val1));                                    \
        SET_INDEX_CONST(sol, (val2));                                   \
        SET_INDEX_CONST(out, (val3));                                   \
        ALLREDUCE_AND_FREE(mpi_type, mpi_op, in, out, sol);             \
    }

#define lor_test1(type, mpi_type)                                       \
    const_test(type, mpi_type, MPI_LOR, (rank & 0x1), (size > 1), 0)
#define lor_test2(type, mpi_type)                       \
    const_test(type, mpi_type, MPI_LOR, 0, 0, 0)
#define lxor_test1(type, mpi_type)                                      \
    const_test(type, mpi_type, MPI_LXOR, (rank == 1), (size > 1), 0)
#define lxor_test2(type, mpi_type)                      \
    const_test(type, mpi_type, MPI_LXOR, 0, 0, 0)
#define lxor_test3(type, mpi_type)                      \
    const_test(type, mpi_type, MPI_LXOR, 1, 0, 0)
#define land_test1(type, mpi_type)                              \
    const_test(type, mpi_type, MPI_LAND, (rank & 0x1), 0, 0)
#define land_test2(type, mpi_type)                      \
    const_test(type, mpi_type, MPI_LAND, 1, 1, 0)
#define bor_test1(type, mpi_type)                                       \
    const_test(type, mpi_type, MPI_BOR, (rank & 0x3), ((size < 3) ? size - 1 : 0x3), 0)
#define bxor_test1(type, mpi_type)                                      \
    const_test(type, mpi_type, MPI_BXOR, (rank == 1) * 0xf0, (size > 1) * 0xf0, 0)
#define bxor_test2(type, mpi_type)                      \
    const_test(type, mpi_type, MPI_BXOR, 0, 0, 0)
#define bxor_test3(type, mpi_type)                      \
    const_test(type, mpi_type, MPI_BXOR, ~0, 0, 0)

#define band_test1(type, mpi_type)                                      \
    {                                                                   \
        DECL_MALLOC_IN_OUT_SOL(type);                                   \
        if (rank == size-1) {                                           \
            SET_INDEX_SUM(in, 0);                                       \
        }                                                               \
        else {                                                          \
            SET_INDEX_CONST(in, ~0);                                    \
        }                                                               \
        SET_INDEX_SUM(sol, 0);                                          \
        SET_INDEX_CONST(out, 0);                                        \
        ALLREDUCE_AND_FREE(mpi_type, MPI_BAND, in, out, sol);           \
    }

#define band_test2(type, mpi_type)                                      \
    {                                                                   \
        DECL_MALLOC_IN_OUT_SOL(type);                                   \
        if (rank == size-1) {                                           \
            SET_INDEX_SUM(in, 0);                                       \
        }                                                               \
        else {                                                          \
            SET_INDEX_CONST(in, 0);                                     \
        }                                                               \
        SET_INDEX_CONST(sol, 0);                                        \
        SET_INDEX_CONST(out, 0);                                        \
        ALLREDUCE_AND_FREE(mpi_type, MPI_BAND, in, out, sol);           \
    }

#define maxloc_test(type, mpi_type)                                     \
    {                                                                   \
        DECL_MALLOC_IN_OUT_SOL(type);                                   \
        SET_INDEX_STRUCT_SUM(in, rank, a);                              \
        SET_INDEX_STRUCT_CONST(in, rank, b);                            \
        SET_INDEX_STRUCT_SUM(sol, size - 1, a);                         \
        SET_INDEX_STRUCT_CONST(sol, size - 1, b);                       \
        SET_INDEX_STRUCT_CONST(out, 0, a);                              \
        SET_INDEX_STRUCT_CONST(out, -1, b);                             \
        STRUCT_ALLREDUCE_AND_FREE(mpi_type, MPI_MAXLOC, in, out, sol);   \
    }

#define minloc_test(type, mpi_type)                                     \
    {                                                                   \
        DECL_MALLOC_IN_OUT_SOL(type);                                   \
        SET_INDEX_STRUCT_SUM(in, rank, a);                              \
        SET_INDEX_STRUCT_CONST(in, rank, b);                            \
        SET_INDEX_STRUCT_SUM(sol, 0, a);                                \
        SET_INDEX_STRUCT_CONST(sol, 0, b);                              \
        SET_INDEX_STRUCT_CONST(out, 0, a);                              \
        SET_INDEX_STRUCT_CONST(out, -1, b);                             \
        STRUCT_ALLREDUCE_AND_FREE(mpi_type, MPI_MINLOC, in, out, sol);  \
    }

#define test_types_set1(op, post)                                   \
    {                                                               \
        op##_test##post(int, MPI_INT);                              \
        op##_test##post(long, MPI_LONG);                            \
        op##_test##post(short, MPI_SHORT);                          \
        op##_test##post(unsigned short, MPI_UNSIGNED_SHORT);        \
        op##_test##post(unsigned, MPI_UNSIGNED);                    \
        op##_test##post(unsigned long, MPI_UNSIGNED_LONG);          \
        op##_test##post(unsigned char, MPI_UNSIGNED_CHAR);          \
    }

#define test_types_set2(op, post)               \
    {                                           \
        test_types_set1(op, post);              \
        op##_test##post(float, MPI_FLOAT);      \
        op##_test##post(double, MPI_DOUBLE);    \
    }

#define test_types_set3(op, post)                                   \
    {                                                               \
        op##_test##post(unsigned char, MPI_BYTE);                   \
    }

int main( int argc, char **argv )
{
    MTest_Init( &argc, &argv );

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    count = 10;

    test_types_set2(sum, 1);
    test_types_set2(prod, 1);
    test_types_set2(max, 1);
    test_types_set2(min, 1);

    test_types_set1(lor, 1);
    test_types_set1(lor, 2);

    test_types_set1(lxor, 1);
    test_types_set1(lxor, 2);
    test_types_set1(lxor, 3);

    test_types_set1(land, 1);
    test_types_set1(land, 2);

    test_types_set1(bor, 1);
    test_types_set1(band, 1);
    test_types_set1(band, 2);

    test_types_set1(bxor, 1);
    test_types_set1(bxor, 2);
    test_types_set1(bxor, 3);

    test_types_set3(bor, 1);
    test_types_set3(band, 1);
    test_types_set3(band, 2);

    test_types_set3(bxor, 1);
    test_types_set3(bxor, 2);
    test_types_set3(bxor, 3);

    maxloc_test(struct int_test, MPI_2INT);
    maxloc_test(struct long_test, MPI_LONG_INT);
    maxloc_test(struct short_test, MPI_SHORT_INT);
    maxloc_test(struct float_test, MPI_FLOAT_INT);
    maxloc_test(struct double_test, MPI_DOUBLE_INT);

    minloc_test(struct int_test, MPI_2INT);
    minloc_test(struct long_test, MPI_LONG_INT);
    minloc_test(struct short_test, MPI_SHORT_INT);
    minloc_test(struct float_test, MPI_FLOAT_INT);
    minloc_test(struct double_test, MPI_DOUBLE_INT);

    MTest_Finalize( cerrcnt );
    MPI_Finalize();
    return 0;
}
