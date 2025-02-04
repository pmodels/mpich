/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_op_util.h"

/*
 * In MPI-2.1, this operation is valid only for C integer, Fortran integer,
 * and byte data items (5.9.2 Predefined reduce operations)
 */
#ifndef MPIR_LBXOR
#define MPIR_LBXOR(a,b) ((a)^(b))
#endif

void MPIR_BXOR(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPI_Aint i, len = *Len;

    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(*type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPIR_LBXOR)
            /* no semicolons by necessity */
            MPIR_OP_TYPE_GROUP(INTEGER)
#undef MPIR_OP_TYPE_MACRO
        default:
            MPIR_Assert(0);
            break;
    }
}
