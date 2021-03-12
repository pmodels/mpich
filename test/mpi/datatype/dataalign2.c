/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include "mpitest.h"
#include <stddef.h>
#include <stdio.h>
#include <complex.h>
#include <stdbool.h>
#include <inttypes.h>

struct C_BOOL {
    char a;
    bool b;
};
struct CHAR {
    char a;
    char b;
};
struct SHORT {
    char a;
    short b;
};
struct INT {
    char a;
    int b;
};
struct LONG {
    char a;
    long b;
};
struct FLOAT {
    char a;
    float b;
};
struct DOUBLE {
    char a;
    double b;
};
struct LONG_DOUBLE {
    char a;
    long double b;
};
struct INT8_T {
    char a;
    int8_t b;
};
struct INT16_T {
    char a;
    int16_t b;
};
struct INT32_T {
    char a;
    int32_t b;
};
struct INT64_T {
    char a;
    int64_t b;
};
struct AINT {
    char a;
    MPI_Aint b;
};
struct COUNT {
    char a;
    MPI_Count b;
};
struct OFFSET {
    char a;
    MPI_Offset b;
};
struct C_COMPLEX {
    char a;
    float complex b;
};
struct C_DOUBLE_COMPLEX {
    char a;
    double complex b;
};
struct C_LONG_DOUBLE_COMPLEX {
    char a;
    long double complex b;
};

/* The resulting struct datatype's extent will round up to the alignment.
 * It should agree with C's struct size */
#define TEST_TYPE(T) \
    do { \
        displs[1] = offsetof(struct T, b); \
        MPI_Type_dup(MPI_ ## T, &types[1]); \
        MPI_Type_create_struct(2, blklens, displs, types, &newtype); \
        MPI_Type_get_extent(newtype, &lb, &extent); \
        if (extent != sizeof(struct T)) { \
            printf("Wrong extent with struct %s, expect %zd, got %lld\n", #T, sizeof(struct T), (long long)extent); \
            errs++; \
        } \
        MPI_Type_free(&types[1]); \
        MPI_Type_free(&newtype); \
    } while (0)

int main(int argc, char *argv[])
{
    int errs = 0;

    MTest_Init(&argc, &argv);

    int blklens[2] = { 1, 1 };
    MPI_Aint displs[2];
    MPI_Datatype types[2];

    MPI_Datatype newtype;
    MPI_Aint lb, extent;

    displs[0] = 0;
    types[0] = MPI_CHAR;

    TEST_TYPE(CHAR);
    TEST_TYPE(SHORT);
    TEST_TYPE(INT);
    TEST_TYPE(LONG);
    TEST_TYPE(FLOAT);
    TEST_TYPE(DOUBLE);
    TEST_TYPE(LONG_DOUBLE);
    TEST_TYPE(INT8_T);
    TEST_TYPE(INT16_T);
    TEST_TYPE(INT32_T);
    TEST_TYPE(INT64_T);
    TEST_TYPE(AINT);
    TEST_TYPE(COUNT);
    TEST_TYPE(OFFSET);
    TEST_TYPE(C_BOOL);
    TEST_TYPE(C_COMPLEX);
    TEST_TYPE(C_DOUBLE_COMPLEX);
    TEST_TYPE(C_LONG_DOUBLE_COMPLEX);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
