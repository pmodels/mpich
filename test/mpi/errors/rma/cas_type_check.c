/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <assert.h>
#include "mpitest.h"

#define CAS_CHECK_TYPE(c_type, mpi_type, expected_err)  \
do {                                                    \
    int            err, err_class, i;                   \
    c_type         val, cmp_val;                        \
    c_type         buf, res;                            \
    MPI_Win        win;                                 \
                                                        \
    val = cmp_val = buf = 0;                            \
                                                        \
    MPI_Win_create(&buf, sizeof(c_type), sizeof(c_type),       \
                    MPI_INFO_NULL, MPI_COMM_WORLD, &win);      \
                                                        \
    MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);   \
                                                        \
    MPI_Win_fence(MPI_MODE_NOPRECEDE, win);           \
    for (i = 0; i < 10000; i++) {                       \
    err = MPI_Compare_and_swap(&val, &cmp_val, &res,  \
                                 mpi_type, 0, 0, win); \
    MPI_Error_class(err, &err_class);                 \
    assert(err_class == expected_err);                \
    }                                                   \
    MPI_Win_fence(MPI_MODE_NOSUCCEED, win);            \
                                                        \
    MPI_Win_free(&win);                               \
} while (0);


int main(int argc, char *argv[])
{
    int rank;
    MPI_Datatype my_int;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* C Integer */
    CAS_CHECK_TYPE(signed char, MPI_SIGNED_CHAR, MPI_SUCCESS);
    CAS_CHECK_TYPE(short, MPI_SHORT, MPI_SUCCESS);
    CAS_CHECK_TYPE(int, MPI_INT, MPI_SUCCESS);
    CAS_CHECK_TYPE(long, MPI_LONG, MPI_SUCCESS);
    CAS_CHECK_TYPE(long long, MPI_LONG_LONG, MPI_SUCCESS);
    CAS_CHECK_TYPE(unsigned char, MPI_UNSIGNED_CHAR, MPI_SUCCESS);
    CAS_CHECK_TYPE(unsigned short, MPI_UNSIGNED_SHORT, MPI_SUCCESS);
    CAS_CHECK_TYPE(unsigned, MPI_UNSIGNED, MPI_SUCCESS);
    CAS_CHECK_TYPE(unsigned long, MPI_UNSIGNED_LONG, MPI_SUCCESS);
    CAS_CHECK_TYPE(unsigned long long, MPI_UNSIGNED_LONG_LONG, MPI_SUCCESS);

    /* Multilanguage Types */
    CAS_CHECK_TYPE(MPI_Aint, MPI_AINT, MPI_SUCCESS);
    CAS_CHECK_TYPE(MPI_Offset, MPI_OFFSET, MPI_SUCCESS);
    CAS_CHECK_TYPE(MPI_Count, MPI_COUNT, MPI_SUCCESS);

    /* Byte */
    CAS_CHECK_TYPE(char, MPI_BYTE, MPI_SUCCESS);

    /* Logical */
    CAS_CHECK_TYPE(char, MPI_C_BOOL, MPI_SUCCESS);

    /* ERR: Derived datatypes */
    MPI_Type_contiguous(sizeof(int), MPI_BYTE, &my_int);
    MPI_Type_commit(&my_int);
    CAS_CHECK_TYPE(int, my_int, MPI_ERR_TYPE);
    MPI_Type_free(&my_int);

    /* ERR: Character types */
    CAS_CHECK_TYPE(char, MPI_CHAR, MPI_SUCCESS);

    /* ERR: Floating point */
    CAS_CHECK_TYPE(float, MPI_FLOAT, MPI_ERR_TYPE);
    CAS_CHECK_TYPE(double, MPI_DOUBLE, MPI_ERR_TYPE);
#ifdef HAVE_LONG_DOUBLE
    if (MPI_LONG_DOUBLE != MPI_DATATYPE_NULL) {
        CAS_CHECK_TYPE(long double, MPI_LONG_DOUBLE, MPI_ERR_TYPE);
    }
#endif

    if (rank == 0)
        printf(" No Errors\n");
    MPI_Finalize();

    return 0;
}
