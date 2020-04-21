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
#define MPIR_LSUM(a,b) ((a)+(b))

void real16_sum_(void *invec, void *inoutvec, int *Len);

void MPIR_SUM(void *invec, void *inoutvec, int *Len, MPI_Datatype * type)
{
    int i, len = *Len;

    switch (*type) {
/* *INDENT-OFF* */
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_, type_name_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPIR_LSUM)
            /* no semicolons by necessity */
            MPIR_OP_TYPE_GROUP(C_INTEGER)
                MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER)
                MPIR_OP_TYPE_GROUP(FLOATING_POINT)
                /* extra types that are not required to be supported by the MPI Standard */
                MPIR_OP_TYPE_GROUP(C_INTEGER_EXTRA)
                MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER_EXTRA)
                MPIR_OP_TYPE_GROUP(FLOATING_POINT_EXTRA)

                /* complex addition is slightly different than scalar addition */
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_, type_name_) \
        case (mpi_type_): {                                \
            c_type_ * restrict a = (c_type_ *)inoutvec;    \
            const c_type_ * restrict b = (c_type_ *)invec; \
            for (i=0; i<len; i++) {                      \
                a[i].re = MPIR_LSUM(a[i].re ,b[i].re);     \
                a[i].im = MPIR_LSUM(a[i].im ,b[i].im);     \
            }                                              \
            break;                                         \
        }
                /* C complex types are just simple sums */
#undef MPIR_OP_C_COMPLEX_TYPE_MACRO
#define MPIR_OP_C_COMPLEX_TYPE_MACRO(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_,c_type_,MPIR_LSUM)
                MPIR_OP_TYPE_GROUP(COMPLEX)
                MPIR_OP_TYPE_GROUP(COMPLEX_EXTRA)
                /* put things back where we found them */
#undef MPIR_OP_TYPE_MACRO
#undef MPIR_OP_C_COMPLEX_TYPE_MACRO
#define MPIR_OP_C_COMPLEX_TYPE_MACRO(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
/* *INDENT-ON* */
        case MPI_REAL16:
            real16_sum_(invec, inoutvec, Len);
            break;
        default:
            MPIR_Assert(0);
            break;
    }
}


int MPIR_SUM_check_dtype(MPI_Datatype type)
{
    switch (type) {
/* *INDENT-OFF* */
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_, type_name_) case (mpi_type_):
            MPIR_OP_TYPE_GROUP(C_INTEGER)
                MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER)
                MPIR_OP_TYPE_GROUP(FLOATING_POINT)
                /* extra types that are not required to be supported by the MPI Standard */
                MPIR_OP_TYPE_GROUP(C_INTEGER_EXTRA)
                MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER_EXTRA)
                MPIR_OP_TYPE_GROUP(FLOATING_POINT_EXTRA)

                MPIR_OP_TYPE_GROUP(COMPLEX)
                MPIR_OP_TYPE_GROUP(COMPLEX_EXTRA)
                case (MPI_REAL16):
#undef MPIR_OP_TYPE_MACRO
                return MPI_SUCCESS;
/* *INDENT-ON* */
            /* --BEGIN ERROR HANDLING-- */
        default:
            return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                        MPI_ERR_OP, "**opundefined", "**opundefined %s", "MPI_SUM");
            /* --END ERROR HANDLING-- */
    }
}
