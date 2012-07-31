/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <assert.h>

/* MPI-3 is not yet standardized -- allow MPI-3 routines to be switched off.
 */

#if !defined(USE_STRICT_MPI) && defined(MPICH2)
#  define TEST_MPI3_ROUTINES 1
#endif

#define CAS_CHECK_TYPE(c_type, mpi_type, expected_err)  \
do {                                                    \
    int            err, err_class;                      \
    c_type         val, cmp_val;                        \
    c_type         buf, res;                            \
    MPI_Win        win;                                 \
                                                        \
    val = cmp_val = 0;                                  \
                                                        \
    MPI_Win_create( &buf, sizeof(c_type), sizeof(c_type),       \
                    MPI_INFO_NULL, MPI_COMM_WORLD, &win );      \
                                                        \
    MPI_Win_set_errhandler( win, MPI_ERRORS_RETURN );   \
                                                        \
    MPI_Win_fence( MPI_MODE_NOPRECEDE, win );           \
    err = MPIX_Compare_and_swap( &val, &cmp_val, &res,  \
                                 mpi_type, 0, 0, win ); \
    MPI_Error_class( err, &err_class );                 \
    assert( err_class == expected_err );                \
    MPI_Win_fence( MPI_MODE_NOSUCCEED, win);            \
                                                        \
    MPI_Win_free( &win );                               \
} while (0);


/* TODO: Compare_and_swap currently returns a notimpl error */
#define MY_ERR_SUCCESS MPI_ERR_OTHER

int main( int argc, char *argv[] )
{
    int          rank;
    MPI_Datatype my_int;

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

#ifdef TEST_MPI3_ROUTINES

    /* C Integer */
    CAS_CHECK_TYPE(signed char,         MPI_SIGNED_CHAR,        MY_ERR_SUCCESS);
    CAS_CHECK_TYPE(short,               MPI_SHORT,              MY_ERR_SUCCESS);
    CAS_CHECK_TYPE(int,                 MPI_INT,                MY_ERR_SUCCESS);
    CAS_CHECK_TYPE(long,                MPI_LONG,               MY_ERR_SUCCESS);
    CAS_CHECK_TYPE(long long,           MPI_LONG_LONG,          MY_ERR_SUCCESS);
    CAS_CHECK_TYPE(unsigned char,       MPI_UNSIGNED_CHAR,      MY_ERR_SUCCESS);
    CAS_CHECK_TYPE(unsigned short,      MPI_UNSIGNED_SHORT,     MY_ERR_SUCCESS);
    CAS_CHECK_TYPE(unsigned,            MPI_UNSIGNED,           MY_ERR_SUCCESS);
    CAS_CHECK_TYPE(unsigned long,       MPI_UNSIGNED_LONG,      MY_ERR_SUCCESS);
    CAS_CHECK_TYPE(unsigned long long,  MPI_UNSIGNED_LONG_LONG, MY_ERR_SUCCESS);

    /* Multilanguage Types */
    CAS_CHECK_TYPE(MPI_Aint,            MPI_AINT,               MY_ERR_SUCCESS);
    CAS_CHECK_TYPE(MPI_Offset,          MPI_OFFSET,             MY_ERR_SUCCESS);
    /* TODO: MPI_Count support needed */
    /*
    CAS_CHECK_TYPE(MPI_Count,           MPI_COUNT,              MY_ERR_SUCCESS);
    */

    /* Byte */
    CAS_CHECK_TYPE(char,                MPI_BYTE,               MY_ERR_SUCCESS);

    /* Logical */
    CAS_CHECK_TYPE(char,                MPI_C_BOOL,             MY_ERR_SUCCESS);

    /* ERR: Derived datatypes */
    MPI_Type_contiguous(sizeof(int), MPI_BYTE, &my_int);
    MPI_Type_commit(&my_int);
    CAS_CHECK_TYPE(int,                 my_int,                 MPI_ERR_TYPE);
    MPI_Type_free(&my_int);

    /* ERR: Character types */
    CAS_CHECK_TYPE(char,                MPI_CHAR,               MPI_ERR_TYPE);

    /* ERR: Floating point */
    CAS_CHECK_TYPE(float,               MPI_FLOAT,              MPI_ERR_TYPE);
    CAS_CHECK_TYPE(double,              MPI_DOUBLE,             MPI_ERR_TYPE);
    CAS_CHECK_TYPE(long double,         MPI_LONG_DOUBLE,        MPI_ERR_TYPE);

#endif /* TEST_MPI3_ROUTINES */

    if (rank == 0) printf(" No Errors\n");
    MPI_Finalize();

    return 0;
}
