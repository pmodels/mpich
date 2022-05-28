/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

static void call_user_op(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype,
                         MPIR_User_function uop)
{
    /* Take off the global locks before calling user functions */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    (uop.c_function) ((void *) inbuf, inoutbuf, &count, &datatype);
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
}

static void call_user_op_large(const void *inbuf, void *inoutbuf, MPI_Count count,
                               MPI_Datatype datatype, MPIR_User_function uop)
{
    /* Take off the global locks before calling user functions */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    (uop.c_large_function) ((void *) inbuf, inoutbuf, &count, &datatype);
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

#if defined(HAVE_FORTRAN_BINDING) && !defined(HAVE_FINT_IS_INT)
static void call_user_op_f77(const void *inbuf, void *inoutbuf, MPI_Fint count, MPI_Fint datatype,
                             MPIR_User_function uop)
{
    /* Take off the global locks before calling user functions */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    (uop.f77_function) ((void *) inbuf, inoutbuf, &count, &datatype);
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
        if (ENABLE_GPU && MPIR_Typerep_reduce_is_supported(op, datatype)) {
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
            MPIR_Assert(op_ptr->kind == MPIR_OP_KIND__USER ||
                        op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE);
            call_user_op_cxx(inbuf, inoutbuf, (int) count, datatype,
                             (MPI_User_function *) op_ptr->function.c_function);
            goto fn_exit;
        }
#endif
#if defined(HAVE_FORTRAN_BINDING) && !defined(HAVE_FINT_IS_INT)
        if (op_ptr->language == MPIR_LANG__FORTRAN) {
            /* large count not supported */
            MPIR_Assert(op_ptr->kind == MPIR_OP_KIND__USER ||
                        op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE);
            call_user_op_f77(inbuf, inoutbuf, (MPI_Fint) count, (MPI_Fint) datatype,
                             op_ptr->function);
            goto fn_exit;
        }
#endif
        if (op_ptr->kind == MPIR_OP_KIND__USER_LARGE ||
            op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE_LARGE) {
            call_user_op_large(inbuf, inoutbuf, (MPI_Count) count, datatype, op_ptr->function);
        } else {
            MPIR_Assert(count <= INT_MAX);
            call_user_op(inbuf, inoutbuf, (int) count, datatype, op_ptr->function);
        }
    }

  fn_exit:
    return mpi_errno;
}
