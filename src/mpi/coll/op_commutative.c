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
int MPI_Op_commutative(MPI_Op op, int *commute) __attribute__((weak,alias("PMPI_Op_commutative")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Op_commutative
#define MPI_Op_commutative PMPI_Op_commutative

#endif

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
    MPID_Op *op_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_OP_COMMUTATIVE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_OP_COMMUTATIVE);

    MPID_Op_get_ptr( op, op_ptr );

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Op_valid_ptr( op_ptr, mpi_errno );
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
        *commute = 1;
    }
    else {
        if (op_ptr->kind == MPID_OP_USER_NONCOMMUTE)
            *commute = 0;
        else
            *commute = 1;
    }

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_OP_COMMUTATIVE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;
fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_op_commutative",
            "**mpi_op_commutative %O %p", op, commute);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

