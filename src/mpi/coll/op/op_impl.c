/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

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
    op_ptr->is_commute = commute;
    op_ptr->kind = MPIR_OP_KIND__USER;
#ifndef BUILD_MPI_ABI
    op_ptr->function.c_function = (void (*)(const void *, void *,
                                            const int *, const MPI_Datatype *)) user_fn;
#else
    op_ptr->function.c_function = (void (*)(const void *, void *,
                                            const int *, const ABI_Datatype *)) user_fn;
#endif
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
    int mpi_errno = MPIR_Op_create_impl(NULL, commute, p_op_ptr);
    if (mpi_errno == MPI_SUCCESS) {
        (*p_op_ptr)->is_commute = commute;
        (*p_op_ptr)->kind = MPIR_OP_KIND__USER_LARGE;
#ifndef BUILD_MPI_ABI
        (*p_op_ptr)->function.c_large_function = (void (*)(const void *, void *,
                                                           const MPI_Count *,
                                                           const MPI_Datatype *)) user_fn;
#else
        (*p_op_ptr)->function.c_large_function = (void (*)(const void *, void *,
                                                           const MPI_Count *,
                                                           const ABI_Datatype *)) user_fn;
#endif
    }
    return mpi_errno;
}

int MPIR_Op_free_impl(MPIR_Op * op_ptr)
{
    int in_use;

    MPIR_Op_ptr_release_ref(op_ptr, &in_use);
    if (!in_use) {
        if (op_ptr->kind == MPIR_OP_KIND__USER_X && op_ptr->destructor_fn) {
            op_ptr->destructor_fn(op_ptr->extra_state);
        }
        MPIR_Handle_obj_free(&MPIR_Op_mem, op_ptr);
        MPID_Op_free_hook(op_ptr);
    }

    return MPI_SUCCESS;
}

int MPIR_Op_create_x_impl(MPIX_User_function_x * user_fn,
                          MPIX_Destructor_function * destructor_fn,
                          int commute, void *extra_state, MPIR_Op ** p_op_ptr)
{
    int mpi_errno = MPIR_Op_create_impl(NULL, commute, p_op_ptr);
    if (mpi_errno == MPI_SUCCESS) {
        (*p_op_ptr)->kind = MPIR_OP_KIND__USER_X;
        (*p_op_ptr)->is_commute = commute;
        (*p_op_ptr)->extra_state = extra_state;
        (*p_op_ptr)->function.c_x_function = user_fn;
        (*p_op_ptr)->destructor_fn = destructor_fn;
    }
    return mpi_errno;
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
        if (!op_ptr->is_commute) {
            return FALSE;
        } else {
            return TRUE;
        }
    }
}

int MPIR_Op_commutative_impl(MPI_Op op, int *commute)
{
    int mpi_errno = MPI_SUCCESS;

    *commute = MPIR_Op_is_commutative(op);

    return mpi_errno;
}
