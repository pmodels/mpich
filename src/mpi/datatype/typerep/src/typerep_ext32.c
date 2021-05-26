/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpiimpl.h>
#include "typerep_util.h"

#include <stdio.h>
#include <stdlib.h>

/* "external32" format defined by MPI specification.
 * The type is defined to be platform-independent and data is packed in big-endian order.
 */

typedef struct external32_basic_size {
    MPI_Datatype el_type;
    MPI_Aint el_size;
} external32_basic_size_t;

static external32_basic_size_t external32_basic_size_array[] = {
    {MPI_PACKED, 1},
    {MPI_BYTE, 1},
    {MPI_CHAR, 1},
    {MPI_UNSIGNED_CHAR, 1},
    {MPI_SIGNED_CHAR, 1},
    {MPI_WCHAR, 2},
    {MPI_SHORT, 2},
    {MPI_UNSIGNED_SHORT, 2},
    {MPI_INT, 4},
    {MPI_UNSIGNED, 4},
    {MPI_LONG, 4},
    {MPI_UNSIGNED_LONG, 4},
    {MPI_LONG_LONG, 8},
    {MPI_LONG_LONG_INT, 8},
    {MPI_UNSIGNED_LONG_LONG, 8},
    {MPI_FLOAT, 4},
    {MPI_DOUBLE, 8},
    {MPI_LONG_DOUBLE, 16},

    {MPI_C_BOOL, 1},
    {MPI_INT8_T, 1},
    {MPI_INT16_T, 2},
    {MPI_INT32_T, 4},
    {MPI_INT64_T, 8},
    {MPI_UINT8_T, 1},
    {MPI_UINT16_T, 2},
    {MPI_UINT32_T, 4},
    {MPI_UINT64_T, 8},
    {MPI_AINT, 8},
    {MPI_COUNT, 8},
    {MPI_OFFSET, 8},
    {MPI_C_COMPLEX, 2 * 4},
    {MPI_C_FLOAT_COMPLEX, 2 * 4},
    {MPI_C_DOUBLE_COMPLEX, 2 * 8},
    {MPI_C_LONG_DOUBLE_COMPLEX, 2 * 16},

    {MPI_CHARACTER, 1},
    {MPI_LOGICAL, 4},
    {MPI_INTEGER, 4},
    {MPI_REAL, 4},
    {MPI_DOUBLE_PRECISION, 8},
    {MPI_COMPLEX, 8},
    {MPI_DOUBLE_COMPLEX, 16},

    {MPI_INTEGER1, 1},
    {MPI_INTEGER2, 2},
    {MPI_INTEGER4, 4},
    {MPI_INTEGER8, 8},
    {MPI_INTEGER16, 16},

    {MPI_REAL4, 4},
    {MPI_REAL8, 8},
    {MPI_REAL16, 16},

    {MPI_COMPLEX8, 2 * 4},
    {MPI_COMPLEX16, 2 * 8},
    {MPI_COMPLEX32, 2 * 16},

    {MPI_CXX_BOOL, 1},
    {MPI_CXX_FLOAT_COMPLEX, 2 * 4},
    {MPI_CXX_DOUBLE_COMPLEX, 2 * 8},
    {MPI_CXX_LONG_DOUBLE_COMPLEX, 2 * 16}
};

#define COUNT_OF(array) \
    (sizeof(array) / sizeof(array[0]))

MPI_Aint MPII_Typerep_get_basic_size_external32(MPI_Datatype el_type)
{
    for (int i = 0; i < COUNT_OF(external32_basic_size_array); i++) {
        if (external32_basic_size_array[i].el_type == el_type) {
            return external32_basic_size_array[i].el_size;
        }
    }
    return 0;
}

bool MPII_Typerep_basic_type_is_complex(MPI_Datatype el_type)
{
    switch (el_type) {
        case MPI_C_COMPLEX:
        case MPI_C_DOUBLE_COMPLEX:
        case MPI_C_LONG_DOUBLE_COMPLEX:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_COMPLEX8:
        case MPI_COMPLEX16:
        case MPI_COMPLEX32:
#endif
#ifdef HAVE_CXX_BINDING
        case MPI_CXX_FLOAT_COMPLEX:
        case MPI_CXX_DOUBLE_COMPLEX:
        case MPI_CXX_LONG_DOUBLE_COMPLEX:
#endif
            return true;
        default:
            return false;
    }
}

bool MPII_Typerep_basic_type_is_unsigned(MPI_Datatype el_type)
{
    switch (el_type) {
        case MPI_PACKED:
        case MPI_BYTE:
        case MPI_WCHAR:
        case MPI_UNSIGNED_CHAR:
        case MPI_UNSIGNED_SHORT:
        case MPI_UNSIGNED:
        case MPI_UNSIGNED_LONG:
        case MPI_UNSIGNED_LONG_LONG:
        case MPI_UINT8_T:
        case MPI_UINT16_T:
        case MPI_UINT32_T:
        case MPI_UINT64_T:
            return true;
        default:
            return false;
    }
}
