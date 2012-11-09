/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <mpichconf.h>
#include <mpiimpl.h>
#include <mpid_dataloop.h>


typedef struct external32_basic_size
{
    MPI_Datatype el_type;
    MPI_Aint el_size;
} external32_basic_size_t;

static external32_basic_size_t external32_basic_size_array[] =
{
    { MPI_PACKED, 1 },
    { MPI_BYTE, 1 },
    { MPI_CHAR, 1 },
    { MPI_UNSIGNED_CHAR, 1 },
    { MPI_SIGNED_CHAR, 1 },
    { MPI_WCHAR, 2 },
    { MPI_SHORT, 2 },
    { MPI_UNSIGNED_SHORT, 2 },
    { MPI_INT, 4 },
    { MPI_UNSIGNED, 4 },
    { MPI_LONG, 4 },
    { MPI_UNSIGNED_LONG, 4 },
    { MPI_LONG_LONG, 8 },
    { MPI_LONG_LONG_INT, 8 },
    { MPI_UNSIGNED_LONG_LONG, 8 },
    { MPI_FLOAT, 4 },
    { MPI_DOUBLE, 8 },
    { MPI_LONG_DOUBLE, 16 },

    { MPI_C_BOOL, 1 },
    { MPI_INT8_T, 1 },
    { MPI_INT16_T, 2 },
    { MPI_INT32_T, 4 },
    { MPI_INT64_T, 8 },
    { MPI_UINT8_T, 1 },
    { MPI_UINT16_T, 2 },
    { MPI_UINT32_T, 4 },
    { MPI_UINT64_T, 8 },
    { MPI_AINT, 8 },
    { MPI_COUNT, 8 },
    { MPI_OFFSET, 8 },
    { MPI_C_COMPLEX, 2*4 },
    { MPI_C_FLOAT_COMPLEX, 2*4 },
    { MPI_C_DOUBLE_COMPLEX, 2*8 },
    { MPI_C_LONG_DOUBLE_COMPLEX, 2*16 },

    { MPI_CHARACTER, 1 },
    { MPI_LOGICAL, 4 },
    { MPI_INTEGER, 4 },
    { MPI_REAL, 4 },
    { MPI_DOUBLE_PRECISION, 8 },
    { MPI_COMPLEX, 8 },
    { MPI_DOUBLE_COMPLEX, 16 },

    { MPI_INTEGER1, 1 },
    { MPI_INTEGER2, 2 },
    { MPI_INTEGER4, 4 },
    { MPI_INTEGER8, 8 },
    { MPI_INTEGER16, 16 },

    { MPI_REAL4, 4 },
    { MPI_REAL8, 8 },
    { MPI_REAL16, 16 },

    { MPI_COMPLEX8, 2*4 },
    { MPI_COMPLEX16, 2*8 },
    { MPI_COMPLEX32, 2*16 },

    { MPI_CXX_BOOL, 1 },
    { MPI_CXX_FLOAT_COMPLEX, 2*4 },
    { MPI_CXX_DOUBLE_COMPLEX, 2*8 },
    { MPI_CXX_LONG_DOUBLE_COMPLEX, 2*16 }
};

MPI_Aint MPIDI_Datatype_get_basic_size_external32(MPI_Datatype el_type)
{
    MPI_Aint ret = (MPI_Aint) 0;
    unsigned int i = 0;
    for(i = 0; i < (sizeof(external32_basic_size_array) /
                    sizeof(external32_basic_size_t)); i++)
    {
        if (external32_basic_size_array[i].el_type == el_type)
        {
            ret = external32_basic_size_array[i].el_size;
            break;
        }
    }
    return ret;
}

MPI_Aint MPID_Datatype_size_external32(MPI_Datatype type)
{
    if (HANDLE_GET_KIND(type) == HANDLE_KIND_BUILTIN) {
	return MPIDI_Datatype_get_basic_size_external32(type);
    }
    else {
	MPID_Dataloop *dlp = NULL;

	MPID_Datatype_get_loopptr_macro(type, dlp, MPID_DATALOOP_HETEROGENEOUS);

	return MPID_Dataloop_stream_size(dlp,
					 MPIDI_Datatype_get_basic_size_external32);
    }
}

