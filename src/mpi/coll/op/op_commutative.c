/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Op_commutative */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Op_commutative = PMPI_Op_commutative
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Op_commutative  MPI_Op_commutative
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Op_commutative as PMPI_Op_commutative
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Op_commutative(MPI_Op op, int *commute)
    __attribute__ ((weak, alias("PMPI_Op_commutative")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Op_commutative
#define MPI_Op_commutative PMPI_Op_commutative

#endif

#undef FUNCNAME
#define FUNCNAME MPIR_Op_is_commutative
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* TODO with a modest amount of work in the handle allocator code we should be
 * able to encode commutativity in the handle value and greatly simplify this
 * routine */
/* returns TRUE iff the given op is commutative */
int MPIR_Op_is_commutative(MPI_Op op)
{
    MPIR_Op *op_ptr;

    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
        return TRUE;
    } else {
        MPIR_Op_get_ptr(op, op_ptr);
        MPIR_Assert(op_ptr != NULL);
        if (op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE)
            return FALSE;
        else
            return TRUE;
    }
}

#undef FUNCNAME
#define FUNCNAME MPIR_Op_commutative
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Op_commutative(MPIR_Op * op_ptr, int *commute)
{
    int mpi_errno = MPI_SUCCESS;

    /* Built-in op */
    if (MPIR_OP_KIND__USER_NONCOMMUTE > op_ptr->kind) {
        *commute = 1;
    } else {
        if (op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE)
            *commute = 0;
        else
            *commute = 1;
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPI_Op_commutative
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
  MPI_Op_commute - Queries an MPI reduction operation for its commutativity.

Input Parameters:
. op - operation (handle)

Output Parameters:
. commute - Flag is true if 'op' is a commutative operation. (logical)

.N NULL

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG

.seealso: MPI_Op_create
@*/
int MPI_Op_commutative(MPI_Op op, int *commute)
{
    MPIR_Op *op_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_OP_COMMUTATIVE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_OP_COMMUTATIVE);

    MPIR_Op_get_ptr(op, op_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Op_valid_ptr(op_ptr, mpi_errno);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    MPIR_Op_commutative(op_ptr, commute);

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_OP_COMMUTATIVE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

#ifdef HAVE_ERROR_CHECKING
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_op_commutative", "**mpi_op_commutative %O %p", op, commute);
    }
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
#endif
}
