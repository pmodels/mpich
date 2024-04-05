/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#ifdef BUILD_MPI_ABI
#include "mpi_abi_util.h"
#endif

static void call_user_op(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype,
                         MPIR_User_function uop)
{
    /* Take off the global locks before calling user functions */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
#ifndef BUILD_MPI_ABI
    (uop.c_function) ((void *) inbuf, inoutbuf, &count, &datatype);
#else
    ABI_Datatype t = ABI_Datatype_from_mpi(datatype);
    (uop.c_function) ((void *) inbuf, inoutbuf, &count, &t);
#endif
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
}

static void call_user_op_large(const void *inbuf, void *inoutbuf, MPI_Count count,
                               MPI_Datatype datatype, MPIR_User_function uop)
{
    /* Take off the global locks before calling user functions */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
#ifndef BUILD_MPI_ABI
    (uop.c_large_function) ((void *) inbuf, inoutbuf, &count, &datatype);
#else
    ABI_Datatype t = ABI_Datatype_from_mpi(datatype);
    (uop.c_large_function) ((void *) inbuf, inoutbuf, &count, &t);
#endif
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
}

static void call_user_op_x(const void *inbuf, void *inoutbuf, MPI_Count count,
                           MPI_Datatype datatype, MPIR_User_function uop, void *extra_state)
{
    /* Take off the global locks before calling user functions */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
#ifndef BUILD_MPI_ABI
    (uop.c_x_function) ((void *) inbuf, inoutbuf, count, datatype, extra_state);
#else
    ABI_Datatype t = ABI_Datatype_from_mpi(datatype);
    (uop.c_x_function) ((void *) inbuf, inoutbuf, count, t, extra_state);
#endif
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
}

#ifdef HAVE_CXX_BINDING
static void call_user_op_cxx(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype,
                             MPI_User_function * uop)
{
    /* Take off the global locks before calling user functions */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    (*MPIR_Process.cxx_call_op_fn) (inbuf, inoutbuf, count, datatype, uop);
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
}
#endif

int MPIR_Reduce_local(const void *inbuf, void *inoutbuf, MPI_Aint count, MPI_Datatype datatype,
                      MPI_Op op)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Op *op_ptr;

    if (count == 0)
        goto fn_exit;

    if (HANDLE_IS_BUILTIN(op)) {
        MPIR_op_function *uop;
        /* --BEGIN ERROR HANDLING-- */
        mpi_errno = (*MPIR_OP_HDL_TO_DTYPE_FN(op)) (datatype);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_exit;
        /* --END ERROR HANDLING-- */
        /* FIXME: ENABLE_GPU is irrelevant here. We should check whether inbuf or inoutbuf is device buffer */
        /* use count=0 since we don't make MPIR_CVAR_YAKSA_REDUCTION_THRESHOLD decision here */
        if (ENABLE_GPU && MPIR_Typerep_reduce_is_supported(op, 0, datatype)) {
            mpi_errno = MPIR_Typerep_reduce(inbuf, inoutbuf, count, datatype, op);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_exit;
        } else {
            /* get the function by indexing into the op table */
            uop = MPIR_OP_HDL_TO_FN(op);
            (*uop) ((void *) inbuf, inoutbuf, &count, &datatype);
        }
    } else {
        MPIR_Op_get_ptr(op, op_ptr);

#ifdef HAVE_CXX_BINDING
        if (op_ptr->language == MPIR_LANG__CXX) {
            /* large count not supported */
            MPIR_Assert(count <= INT_MAX);
            MPIR_Assert(op_ptr->kind == MPIR_OP_KIND__USER);
            call_user_op_cxx(inbuf, inoutbuf, (int) count, datatype,
                             (MPI_User_function *) op_ptr->function.c_function);
            goto fn_exit;
        }
#endif
        if (op_ptr->kind == MPIR_OP_KIND__USER_X) {
            call_user_op_x(inbuf, inoutbuf, count, datatype, op_ptr->function, op_ptr->extra_state);
        } else if (op_ptr->kind == MPIR_OP_KIND__USER_LARGE) {
            call_user_op_large(inbuf, inoutbuf, (MPI_Count) count, datatype, op_ptr->function);
        } else {
            MPIR_Assert(count <= INT_MAX);
            call_user_op(inbuf, inoutbuf, (int) count, datatype, op_ptr->function);
        }
    }

  fn_exit:
    return mpi_errno;
}
