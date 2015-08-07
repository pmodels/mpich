/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This file contains functions that support the RMA code but use some "private"
 * headers from the oputil.h in the "coll" directory.  The alternative is to put
 * this file in src/mpi/rma instead and add -I${top_srcdir}/src/mpi/coll to the
 * AM_CPPFLAGS.  That option is less preferable because the usage of "oputil.h"
 * can bleed out of this directory and it clutters the CPPFLAGS further. */

#include "mpiimpl.h"
#include "oputil.h"

/* Returns true iff the given type is valid for use in MPI-3 RMA atomics, such
 * as MPI_Compare_and_swap or MPI_Fetch_and_op.  Does NOT return MPICH error
 * codes. */
#undef FUNCNAME
#define FUNCNAME MPIR_Sched_cb_free_buf
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Type_is_rma_atomic(MPI_Datatype type)
{
    switch (type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_, type_name_) case mpi_type_:
        MPIR_OP_TYPE_GROUP(C_INTEGER)
        MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER)
        MPIR_OP_TYPE_GROUP(LOGICAL)
        MPIR_OP_TYPE_GROUP(BYTE)
        MPIR_OP_TYPE_GROUP(C_INTEGER_EXTRA)
        MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER_EXTRA)
        MPIR_OP_TYPE_GROUP(LOGICAL_EXTRA)
        MPIR_OP_TYPE_GROUP(BYTE_EXTRA)
            return TRUE;
            break;
#undef MPIR_OP_TYPE_MACRO
        default:
            return FALSE;
            break;
    }
}


/* Returns true if (a == b) when interepreted using the given datatype.
 * Currently, this is only defined for RMA atomic types.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_COMPARE_EQUAL
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Compare_equal(const void *a, const void *b, MPI_Datatype type)
{
    switch (type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_, type_name_) \
        case mpi_type_:                         \
            if (*(c_type_ *)a == *(c_type_ *)b) \
                return TRUE;                    \
                break;
        MPIR_OP_TYPE_GROUP(C_INTEGER)
        MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER)
        MPIR_OP_TYPE_GROUP(LOGICAL)
        MPIR_OP_TYPE_GROUP(BYTE)
        MPIR_OP_TYPE_GROUP(C_INTEGER_EXTRA)
        MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER_EXTRA)
        MPIR_OP_TYPE_GROUP(LOGICAL_EXTRA)
#undef MPIR_OP_TYPE_MACRO
        default:
            return FALSE;
            break;
    }

    return FALSE;
}


