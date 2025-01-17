/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_op_util.h"

/*
 * In MPI-2.1, this operation is valid only for C integer, Fortran integer,
 * Floating point, and Complex types (5.9.2 Predefined reduce operations)
 */
#define MPIR_LPROD(a,b) ((a)*(b))

void MPIR_PROD(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPI_Aint i, len = *Len;

    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(*type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPIR_LPROD)
            /* no semicolons by necessity */
            MPIR_OP_TYPE_GROUP(INTEGER)
                MPIR_OP_TYPE_GROUP(FLOATING_POINT)
                MPIR_OP_TYPE_GROUP(C_COMPLEX)
                /* complex multiplication is slightly different than scalar multiplication */
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) \
        case (mpi_type_): {                             \
            c_type_ * restrict a = (c_type_ *)inoutvec; \
            c_type_ * restrict b = (c_type_ *)invec;    \
            for (i=0; i<len; i++) {                   \
                c_type_ c;                              \
                c.re = a[i].re; c.im = a[i].im;         \
                a[i].re = c.re*b[i].re - c.im*b[i].im;  \
                a[i].im = c.im*b[i].re + c.re*b[i].im;  \
            }                                           \
            break;                                      \
        }
                MPIR_OP_TYPE_GROUP(COMPLEX)
        default:
            MPIR_Assert(0);
            break;
    }
}


int MPIR_PROD_check_dtype(MPI_Datatype type)
{
    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) case (mpi_type_):
            MPIR_OP_TYPE_GROUP(INTEGER)
                MPIR_OP_TYPE_GROUP(FLOATING_POINT)
                MPIR_OP_TYPE_GROUP(C_COMPLEX)
                MPIR_OP_TYPE_GROUP(COMPLEX)
#undef MPIR_OP_TYPE_MACRO
                return MPI_SUCCESS;
            /* --BEGIN ERROR HANDLING-- */
        default:
            return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                        MPI_ERR_OP, "**opundefined", "**opundefined %s",
                                        "MPI_PROD");
            /* --END ERROR HANDLING-- */
    }
}
