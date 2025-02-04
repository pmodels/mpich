/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_op_util.h"
#include "mpii_fortlogical.h"

/*
 * In MPI-2.1, this operation is valid only for C integer and Logical
 * types (5.9.2 Predefined reduce operations)
 */
#ifndef MPIR_LLAND
#define MPIR_LLAND(a,b) ((a)&&(b))
#endif

void MPIR_LAND(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPI_Aint i, len = *Len;

    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(*type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPIR_LLAND)
            /* no semicolons by necessity */
            MPIR_OP_TYPE_GROUP(INTEGER)
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) \
        case (mpi_type_): { \
            c_type_ * restrict a = (c_type_ *)inoutvec; \
            c_type_ * restrict b = (c_type_ *)invec; \
            for (i=0; i<len; i++) { \
                a[i] = MPII_TO_FLOG(MPIR_LLAND(MPII_FROM_FLOG(a[i]), MPII_FROM_FLOG(b[i]))); \
            } \
            break; \
        }
                MPIR_OP_TYPE_GROUP(FORTRAN_LOGICAL)
        default:
            MPIR_Assert(0);
            break;
    }
}
