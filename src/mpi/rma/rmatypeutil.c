/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This file contains functions that support the RMA code but use some "private"
 * headers from the mpir_op_util.h in the "coll" directory.  The alternative is to put
 * this file in src/mpi/rma instead and add -I${top_srcdir}/src/mpi/coll to the
 * AM_CPPFLAGS.  That option is less preferable because the usage of "mpir_op_util.h"
 * can bleed out of this directory and it clutters the CPPFLAGS further. */

#include "mpiimpl.h"
#include "mpir_op_util.h"

/* Returns true iff the given type is valid for use in MPI-3 RMA atomics, such
 * as MPI_Compare_and_swap or MPI_Fetch_and_op.  Does NOT return MPICH error
 * codes. */
int MPIR_Type_is_rma_atomic(MPI_Datatype type)
{
    MPIR_DATATYPE_REPLACE_BUILTIN(type);
    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) case mpi_type_:
            MPIR_OP_TYPE_GROUP(INTEGER)
                return TRUE;
#undef MPIR_OP_TYPE_MACRO
        default:
            return FALSE;
    }
}


/* Returns true if (a == b) when interpreted using the given datatype.
 * Currently, this is only defined for RMA atomic types.
 */
int MPIR_Compare_equal(const void *a, const void *b, MPI_Datatype type)
{
    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) \
        case mpi_type_:                         \
            if (*(c_type_ *)a == *(c_type_ *)b) \
                return TRUE;                    \
                                                \
            break;
            MPIR_OP_TYPE_GROUP(INTEGER)
#undef MPIR_OP_TYPE_MACRO
        default:
            return FALSE;
    }

    return FALSE;
}
