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
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Type_is_rma_atomic(MPI_Datatype type)
{
    switch (type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) case mpi_type_:
        MPIR_OP_TYPE_GROUP(C_INTEGER)
        MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER)
        MPIR_OP_TYPE_GROUP(LOGICAL)
        case MPI_BYTE:
        /* extra types that are not required to be supported by the MPI
        MPIR_OP_TYPE_GROUP(C_INTEGER_EXTRA)
        MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER_EXTRA)
        MPIR_OP_TYPE_GROUP(LOGICAL_EXTRA)
        */
            return TRUE;
            break;
#undef MPIR_OP_TYPE_MACRO
        default:
            return FALSE;
            break;
    }
}

