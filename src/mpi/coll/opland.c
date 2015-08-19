/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "oputil.h"
#ifdef HAVE_FORTRAN_BINDING
#include "mpi_fortlogical.h"
#endif

/*
 * In MPI-2.1, this operation is valid only for C integer and Logical
 * types (5.9.2 Predefined reduce operations)
 */
#ifndef MPIR_LLAND
#define MPIR_LLAND(a,b) ((a)&&(b))
#endif

#undef FUNCNAME
#define FUNCNAME MPIR_LAND
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_LAND (
    void *invec,
    void *inoutvec,
    int *Len,
    MPI_Datatype *type )
{
    int i, len = *Len;

    switch (*type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_, type_name_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPIR_LLAND)
        /* no semicolons by necessity */
        MPIR_OP_TYPE_GROUP(C_INTEGER)

        /* MPI_LOGICAL requires special handling (MPIR_{TO,FROM}_FLOG) */
#if defined(HAVE_FORTRAN_BINDING)
#  undef MPIR_OP_TYPE_MACRO_HAVE_FORTRAN
#  define MPIR_OP_TYPE_MACRO_HAVE_FORTRAN(mpi_type_, c_type_, type_name_) \
    case (mpi_type_): {                                                \
            c_type_ * restrict a = (c_type_ *)inoutvec;                \
            c_type_ * restrict b = (c_type_ *)invec;                   \
            for (i=0; i<len; i++)                                      \
                a[i] = MPIR_TO_FLOG(MPIR_LLAND(MPIR_FROM_FLOG(a[i]),   \
                                               MPIR_FROM_FLOG(b[i]))); \
            break;                                                     \
    }
        /* expand logicals (which may include MPI_C_BOOL, a non-Fortran type) */
        MPIR_OP_TYPE_GROUP(LOGICAL)
        MPIR_OP_TYPE_GROUP(LOGICAL_EXTRA)
        /* now revert _HAVE_FORTRAN macro to default */
#  undef MPIR_OP_TYPE_MACRO_HAVE_FORTRAN
#  define MPIR_OP_TYPE_MACRO_HAVE_FORTRAN(mpi_type_, c_type_, type_name_) MPIR_OP_TYPE_MACRO(mpi_type_, c_type_, type_name_)
#else
        /* if we don't have Fortran support then we don't have to jump through
           any hoops, simply expand the group */
        MPIR_OP_TYPE_GROUP(LOGICAL)
        MPIR_OP_TYPE_GROUP(LOGICAL_EXTRA)
#endif

        /* extra types that are not required to be supported by the MPI Standard */
        MPIR_OP_TYPE_GROUP(C_INTEGER_EXTRA)
        MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER)
        MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER_EXTRA)
        /* We previously supported floating point types, although I question
           their utility in logical boolean ops [goodell@ 2009-03-16] */
        MPIR_OP_TYPE_GROUP(FLOATING_POINT)
        MPIR_OP_TYPE_GROUP(FLOATING_POINT_EXTRA)
#undef MPIR_OP_TYPE_MACRO
        /* --BEGIN ERROR HANDLING-- */
        default: {
            MPID_THREADPRIV_DECL;
            MPID_THREADPRIV_GET;
            MPID_THREADPRIV_FIELD(op_errno) = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP, "**opundefined","**opundefined %s", "MPI_LAND" );
            break;
        }
        /* --END ERROR HANDLING-- */
    }
}


#undef FUNCNAME
#define FUNCNAME MPIR_LAND
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_LAND_check_dtype ( MPI_Datatype type )
{
    switch (type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_, type_name_) case (mpi_type_):
        MPIR_OP_TYPE_GROUP(C_INTEGER)
        MPIR_OP_TYPE_GROUP(LOGICAL) /* no special handling needed in check_dtype code */
        MPIR_OP_TYPE_GROUP(LOGICAL_EXTRA)

        /* extra types that are not required to be supported by the MPI Standard */
        MPIR_OP_TYPE_GROUP(C_INTEGER_EXTRA)
        MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER)
        MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER_EXTRA)
        /* We previously supported floating point types, although I question
           their utility in logical boolean ops [goodell@ 2009-03-16] */
        MPIR_OP_TYPE_GROUP(FLOATING_POINT)
        MPIR_OP_TYPE_GROUP(FLOATING_POINT_EXTRA)
#undef MPIR_OP_TYPE_MACRO
            return MPI_SUCCESS;
        /* --BEGIN ERROR HANDLING-- */
        default:
            return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP, "**opundefined","**opundefined %s", "MPI_LAND" );
        /* --END ERROR HANDLING-- */
    }
}

