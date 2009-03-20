/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "oputil.h"

/*
 * In MPI-2.1, this operation is valid only for C integer, Fortran integer,
 * and floating point types (5.9.2 Predefined reduce operations)
 */

#undef FUNCNAME
#define FUNCNAME MPIR_MAXF
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
void MPIR_MAXF(
    void *invec,
    void *inoutvec,
    int *Len,
    MPI_Datatype *type )
{
    int i, len = *Len;

    switch (*type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPIR_MAX)
        /* no semicolons by necessity */
        MPIR_OP_TYPE_GROUP(C_INTEGER)
        MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER)
        MPIR_OP_TYPE_GROUP(FLOATING_POINT)
        /* extra types that are not required to be supported by the MPI Standard */
        MPIR_OP_TYPE_GROUP(C_INTEGER_EXTRA)
        MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER_EXTRA)
        MPIR_OP_TYPE_GROUP(FLOATING_POINT_EXTRA)
#undef MPIR_OP_TYPE_MACRO
        /* --BEGIN ERROR HANDLING-- */
        default: {
            MPIU_THREADPRIV_DECL;
            MPIU_THREADPRIV_GET;
            MPIU_THREADPRIV_FIELD(op_errno) = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP, "**opundefined","**opundefined %s", "MPI_MAX" );
            break;
        }
        /* --END ERROR HANDLING-- */
    }
}


#undef FUNCNAME
#define FUNCNAME MPIR_MAXF_check_dtype
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_MAXF_check_dtype( MPI_Datatype type )
{
    switch (type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) case (mpi_type_):
        MPIR_OP_TYPE_GROUP(C_INTEGER)
        MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER)
        MPIR_OP_TYPE_GROUP(FLOATING_POINT)
        /* extra types that are not required to be supported by the MPI Standard */
        MPIR_OP_TYPE_GROUP(C_INTEGER_EXTRA)
        MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER_EXTRA)
        MPIR_OP_TYPE_GROUP(FLOATING_POINT_EXTRA)
#undef MPIR_OP_TYPE_MACRO
            return MPI_SUCCESS;
        /* --BEGIN ERROR HANDLING-- */
        default:
            return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP, "**opundefined","**opundefined %s", "MPI_MAX" );
        /* --END ERROR HANDLING-- */
    }
}
