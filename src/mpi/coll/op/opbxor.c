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

    switch (*type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_, type_name_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPIR_LBXOR)
            /* no semicolons by necessity */
            MPIR_OP_TYPE_GROUP(C_INTEGER)
                MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER)
                MPIR_OP_TYPE_GROUP(BYTE)
                /* extra types that are not required to be supported by the MPI Standard */
                MPIR_OP_TYPE_GROUP(C_INTEGER_EXTRA)
                MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER_EXTRA)
                MPIR_OP_TYPE_GROUP(BYTE_EXTRA)
#undef MPIR_OP_TYPE_MACRO
        default:
            MPIR_Assert(0);
            break;
    }
}


int MPIR_BXOR_check_dtype(MPI_Datatype type)
{
    switch (type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_, type_name_) case (mpi_type_):
            MPIR_OP_TYPE_GROUP(C_INTEGER)
                MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER)
                MPIR_OP_TYPE_GROUP(BYTE)
                /* extra types that are not required to be supported by the MPI Standard */
                MPIR_OP_TYPE_GROUP(C_INTEGER_EXTRA)
                MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER_EXTRA)
                MPIR_OP_TYPE_GROUP(BYTE_EXTRA)
#undef MPIR_OP_TYPE_MACRO
                return MPI_SUCCESS;
            /* --BEGIN ERROR HANDLING-- */
        default:
            return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                        MPI_ERR_OP, "**opundefined", "**opundefined %s",
                                        "MPI_BXOR");
            /* --END ERROR HANDLING-- */
    }
}
