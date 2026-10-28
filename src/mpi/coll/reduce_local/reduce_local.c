/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#ifdef MPICH_BUILD_MPI_ABI
#include "mpi_abi_util.h"
#endif

static void call_user_op(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype,
                         MPIR_User_function uop)
{
    /* Take off the global locks before calling user functions */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
#ifndef MPICH_BUILD_MPI_ABI
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
#ifndef MPICH_BUILD_MPI_ABI
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
#ifndef MPICH_BUILD_MPI_ABI
    (uop.c_x_function) ((void *) inbuf, inoutbuf, count, datatype, extra_state);
#else
    ABI_Datatype t = ABI_Datatype_from_mpi(datatype);
    (uop.c_x_function) ((void *) inbuf, inoutbuf, count, t, extra_state);
#endif
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
}

/* The builtin op functions run on the CPU. Swap the device operands, which get here when
 * yaksa can't do the op on the GPU, for host buffers. */
static int reduce_local_on_host(const void *inbuf, void *inoutbuf, MPI_Aint count,
                                MPI_Datatype datatype, MPIR_op_function * uop)
{
    int mpi_errno = MPI_SUCCESS;
    void *host_in = NULL, *host_inout = NULL;
    const void *in = inbuf;
    void *inout = inoutbuf;

    if (MPIR_GPU_query_pointer_is_dev(inbuf)) {
        host_in = MPIR_alloc_buffer(count, datatype);
        mpi_errno = MPIR_Localcopy(inbuf, count, datatype, host_in, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
        in = host_in;
    }
    if (MPIR_GPU_query_pointer_is_dev(inoutbuf)) {
        host_inout = MPIR_alloc_buffer(count, datatype);
        mpi_errno = MPIR_Localcopy(inoutbuf, count, datatype, host_inout, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
        inout = host_inout;
    }

    (*uop) ((void *) in, inout, &count, &datatype);

    if (host_inout) {
        mpi_errno = MPIR_Localcopy(host_inout, count, datatype, inoutbuf, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    if (host_in) {
        MPIR_free_buffer(host_in, count, datatype);
    }
    if (host_inout) {
        MPIR_free_buffer(host_inout, count, datatype);
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Reduce_local(const void *inbuf, void *inoutbuf, MPI_Aint count, MPI_Datatype datatype,
                      MPI_Op op)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Op *op_ptr;

    if (count == 0)
        goto fn_exit;

    if (HANDLE_IS_BUILTIN(op)) {
        MPIR_op_function *uop;
        MPIR_Assert(MPIR_Internal_op_dt_check(op, datatype));

        /* FIXME: ENABLE_GPU is irrelevant here. We should check whether inbuf or inoutbuf is device buffer */
        /* use count=0 since we don't make MPIR_CVAR_YAKSA_REDUCTION_THRESHOLD decision here */
        if (ENABLE_GPU && MPIR_Typerep_reduce_is_supported(op, 0, datatype)) {
            mpi_errno = MPIR_Typerep_reduce(inbuf, inoutbuf, count, datatype, op);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_exit;
        } else {
            /* get the function by indexing into the op table */
            uop = MPIR_OP_HDL_TO_FN(op);
            if (ENABLE_GPU && (MPIR_GPU_query_pointer_is_dev(inbuf) ||
                               MPIR_GPU_query_pointer_is_dev(inoutbuf))) {
                mpi_errno = reduce_local_on_host(inbuf, inoutbuf, count, datatype, uop);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_exit;
            } else {
                (*uop) ((void *) inbuf, inoutbuf, &count, &datatype);
            }
        }
    } else {
        MPIR_Op_get_ptr(op, op_ptr);

        if (HANDLE_IS_BUILTIN(datatype)) {
            datatype = MPIR_DATATYPE_GET_ORIG_BUILTIN(datatype);
            MPIR_Assert(datatype != MPI_DATATYPE_NULL);
        }
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
