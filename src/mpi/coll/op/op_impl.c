/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#ifndef MPIR_OP_PREALLOC
#define MPIR_OP_PREALLOC 16
#endif

/* Preallocated op objects */
MPIR_Op MPIR_Op_builtin[MPIR_OP_N_BUILTIN];
MPIR_Op MPIR_Op_direct[MPIR_OP_PREALLOC];

MPIR_Object_alloc_t MPIR_Op_mem = { 0, 0, 0, 0, 0, 0, MPIR_OP,
    sizeof(MPIR_Op),
    MPIR_Op_direct,
    MPIR_OP_PREALLOC,
    NULL, {0}
};

#ifdef HAVE_CXX_BINDING
void MPII_Op_set_cxx(MPI_Op op, void (*opcall) (void))
{
    MPIR_Op *op_ptr;

    MPIR_Op_get_ptr(op, op_ptr);
    op_ptr->language = MPIR_LANG__CXX;
    MPIR_Process.cxx_call_op_fn = (void (*)(const void *, void *, int,
                                            MPI_Datatype, MPI_User_function *)) opcall;
}
#endif
#if defined(HAVE_FORTRAN_BINDING) && !defined(HAVE_FINT_IS_INT)
/* Normally, the C and Fortran versions are the same, by design in the
   MPI Standard.  However, if MPI_Fint and int are not the same size (e.g.,
   MPI_Fint was made 8 bytes but int is 4 bytes), then the C and Fortran
   versions must be distinquished. */
void MPII_Op_set_fc(MPI_Op op)
{
    MPIR_Op *op_ptr;

    MPIR_Op_get_ptr(op, op_ptr);
    op_ptr->language = MPIR_LANG__FORTRAN;
}
#endif

int MPIR_Op_create_impl(MPI_User_function * user_fn, int commute, MPIR_Op ** p_op_ptr)
{
    MPIR_Op *op_ptr;
    int mpi_errno = MPI_SUCCESS;

    op_ptr = (MPIR_Op *) MPIR_Handle_obj_alloc(&MPIR_Op_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!op_ptr) {
        mpi_errno =
            MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPI_Op");
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    op_ptr->language = MPIR_LANG__C;
    op_ptr->kind = commute ? MPIR_OP_KIND__USER : MPIR_OP_KIND__USER_NONCOMMUTE;
    op_ptr->function.c_function = (void (*)(const void *, void *,
                                            const int *, const MPI_Datatype *)) user_fn;
    MPIR_Object_set_ref(op_ptr, 1);

    MPID_Op_commit_hook(op_ptr);

    *p_op_ptr = op_ptr;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Op_create_large_impl(MPI_User_function_c * user_fn, int commute, MPIR_Op ** p_op_ptr)
{
    return MPIR_Op_create_impl((void *) user_fn, commute, p_op_ptr);
}

int MPIR_Op_free_impl(MPIR_Op * op_ptr)
{
    int in_use;

    MPIR_Op_ptr_release_ref(op_ptr, &in_use);
    if (!in_use) {
        MPIR_Handle_obj_free(&MPIR_Op_mem, op_ptr);
        MPID_Op_free_hook(op_ptr);
    }

    return MPI_SUCCESS;
}

/* TODO with a modest amount of work in the handle allocator code we should be
 * able to encode commutativity in the handle value and greatly simplify this
 * routine */
/* returns TRUE iff the given op is commutative */
int MPIR_Op_is_commutative(MPI_Op op)
{
    MPIR_Op *op_ptr;

    if (HANDLE_IS_BUILTIN(op)) {
        if (op == MPI_NO_OP || op == MPI_REPLACE) {
            return FALSE;
        } else {
            return TRUE;
        }
    } else {
        MPIR_Op_get_ptr(op, op_ptr);
        MPIR_Assert(op_ptr != NULL);
        if (op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE)
            return FALSE;
        else
            return TRUE;
    }
}

int MPIR_Op_commutative_impl(MPI_Op op, int *commute)
{
    int mpi_errno = MPI_SUCCESS;

    *commute = MPIR_Op_is_commutative(op);

    return mpi_errno;
}
