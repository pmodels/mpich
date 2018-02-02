/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpir_op_util.h"

/*
 * In MPI-2.1, this operation is valid only for C integer, Fortran integer,
 * and floating point types (5.9.2 Predefined reduce operations)
 */
#undef FUNCNAME
#define FUNCNAME MPIR_MINF
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_MINF(void *invec, void *inoutvec, int *Len, MPI_Datatype * type)
{
    int i, len = *Len;

    switch (*type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_, type_name_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPL_MIN)
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
        default:{
                {
                    MPIR_Per_thread_t *per_thread = NULL;
                    int err = 0;

                    MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                                 MPIR_Per_thread, per_thread, &err);
                    MPIR_Assert(err == 0);
                    per_thread->op_errno =
                        MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                             MPI_ERR_OP, "**opundefined", "**opundefined %s",
                                             "MPI_MIN");
                }
                break;
            }
            /* --END ERROR HANDLING-- */
    }
}


#undef FUNCNAME
#define FUNCNAME MPIR_MINF_check_dtype
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_MINF_check_dtype(MPI_Datatype type)
{
    switch (type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_, type_name_) case (mpi_type_):
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
            return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                        MPI_ERR_OP, "**opundefined", "**opundefined %s", "MPI_MIN");
            /* --END ERROR HANDLING-- */
    }
}
