/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include "mpitest.h"

#define ACC_CHECK_TYPE(c_type, mpi_type, op, expected_err)                     \
do {                                                                           \
    int            err, err_class;                                             \
    c_type         val;                                                        \
    c_type         buf, res;                                                   \
    MPI_Win        win;                                                        \
                                                                               \
    memset(&val, 0, sizeof(val));                                              \
    memset(&buf, 0, sizeof(buf));                                              \
    memset(&res, 0, sizeof(res));                                              \
                                                                               \
    MPI_Win_create(&buf, sizeof(c_type), sizeof(c_type),                       \
                    MPI_INFO_NULL, MPI_COMM_WORLD, &win);                      \
                                                                               \
    MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);                            \
                                                                               \
    MPI_Win_fence(MPI_MODE_NOPRECEDE, win);                                    \
    err = MPI_Accumulate(&val, 1, mpi_type,                                    \
                         /*rank=*/0, /*disp=*/0, 1, mpi_type,                  \
                         op, win);                                             \
    MPI_Error_class(err, &err_class);                                          \
    assert(err_class == expected_err);                                         \
                                                                               \
    err = MPI_Get_accumulate(&val,                   1, mpi_type,              \
                             &res,                   1, mpi_type,              \
                             /*rank=*/0, /*disp=*/0, 1, mpi_type,              \
                             op, win);                                         \
    MPI_Error_class(err, &err_class);                                          \
    assert(err_class == expected_err);                                         \
    MPI_Win_fence(MPI_MODE_NOSUCCEED, win);                                    \
                                                                               \
    MPI_Win_free(&win);                                                        \
} while (0)


int main(int argc, char *argv[])
{
    int          rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* some C Integer types */
    ACC_CHECK_TYPE(signed char,         MPI_SIGNED_CHAR,        MPI_SUM, MPI_SUCCESS);
    ACC_CHECK_TYPE(short,               MPI_SHORT,              MPI_SUM, MPI_SUCCESS);
    ACC_CHECK_TYPE(int,                 MPI_INT,                MPI_SUM, MPI_SUCCESS);
    ACC_CHECK_TYPE(long,                MPI_LONG,               MPI_SUM, MPI_SUCCESS);
    ACC_CHECK_TYPE(long long,           MPI_LONG_LONG,          MPI_SUM, MPI_SUCCESS);
    ACC_CHECK_TYPE(unsigned char,       MPI_UNSIGNED_CHAR,      MPI_SUM, MPI_SUCCESS);
    ACC_CHECK_TYPE(unsigned short,      MPI_UNSIGNED_SHORT,     MPI_SUM, MPI_SUCCESS);
    ACC_CHECK_TYPE(unsigned,            MPI_UNSIGNED,           MPI_SUM, MPI_SUCCESS);
    ACC_CHECK_TYPE(unsigned long,       MPI_UNSIGNED_LONG,      MPI_SUM, MPI_SUCCESS);
    ACC_CHECK_TYPE(unsigned long long,  MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_SUCCESS);

    /* Multilanguage Types */
    ACC_CHECK_TYPE(MPI_Aint,            MPI_AINT,               MPI_SUM, MPI_SUCCESS);
    ACC_CHECK_TYPE(MPI_Offset,          MPI_OFFSET,             MPI_SUM, MPI_SUCCESS);
    ACC_CHECK_TYPE(MPI_Count,           MPI_COUNT,              MPI_SUM, MPI_SUCCESS);

    /* Logical */
    ACC_CHECK_TYPE(_Bool,               MPI_C_BOOL,             MPI_LOR, MPI_SUCCESS);

    /* Valid derived datatypes */
    {
        MPI_Datatype my_2int = MPI_DATATYPE_NULL;
        MPI_Type_contiguous(2, MPI_INT, &my_2int);
        MPI_Type_commit(&my_2int);
        ACC_CHECK_TYPE(struct {int a; int b; }, my_2int,        MPI_SUM, MPI_SUCCESS);
        MPI_Type_free(&my_2int);
    }

    /* Floating point */
    ACC_CHECK_TYPE(float,               MPI_FLOAT,              MPI_SUM, MPI_SUCCESS);
    ACC_CHECK_TYPE(double,              MPI_DOUBLE,             MPI_SUM, MPI_SUCCESS);

    /* ERR: floating point with logical/bitwise ops */
    ACC_CHECK_TYPE(double,              MPI_DOUBLE,             MPI_BOR, MPI_ERR_OP);

    /* we would like to treat this as an error too, but MPICH actually allows
     * logical boolean ops with floating point datatypes for some reason... */
    ACC_CHECK_TYPE(double,              MPI_DOUBLE,             MPI_LOR, MPI_SUCCESS);

    /* ERR: Invalid derived datatypes */
    {
        MPI_Datatype my_2int = MPI_DATATYPE_NULL;
        MPI_Type_contiguous(2, MPI_INT, &my_2int);
        MPI_Type_commit(&my_2int);
        ACC_CHECK_TYPE(struct { int a; int b; }, my_2int,           MPI_MAXLOC, MPI_ERR_OP);
        MPI_Type_free(&my_2int);
    }
    {
        MPI_Datatype my_struct = MPI_DATATYPE_NULL;
        struct sif { int x; float y; };
        MPI_Datatype dtypes[2] = { MPI_INT, MPI_FLOAT };
        int blklens[2] = { 1, 1 };
        MPI_Aint displs[2] = { offsetof(struct sif, x), offsetof(struct sif, y) };
        MPI_Type_create_struct(2, blklens, displs, dtypes, &my_struct);
        MPI_Type_commit(&my_struct);
        ACC_CHECK_TYPE(struct sif,      my_struct,     MPI_MAXLOC, MPI_ERR_TYPE);
        MPI_Type_free(&my_struct);
    }

    if (rank == 0) printf(" No Errors\n");
    MPI_Finalize();

    return 0;
}
