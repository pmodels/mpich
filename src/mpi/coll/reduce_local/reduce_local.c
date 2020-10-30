/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* any utility functions should go here, usually prefixed with PMPI_LOCAL to
 * correctly handle weak symbols and the profiling interface */

int MPIR_Reduce_local(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype,
                      MPI_Op op)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Op *op_ptr;
    MPI_User_function *uop;
#ifdef HAVE_CXX_BINDING
    int is_cxx_uop = 0;
#endif
#if defined(HAVE_FORTRAN_BINDING) && !defined(HAVE_FINT_IS_INT)
    int is_f77_uop = 0;
#endif

    if (count == 0)
        goto fn_exit;

    if (HANDLE_IS_BUILTIN(op)) {
        /* --BEGIN ERROR HANDLING-- */
        mpi_errno = (*MPIR_OP_HDL_TO_DTYPE_FN(op)) (datatype);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_exit;
        /* --END ERROR HANDLING-- */
        /* get the function by indexing into the op table */
        uop = MPIR_OP_HDL_TO_FN(op);
    } else {
        MPIR_Op_get_ptr(op, op_ptr);

#ifdef HAVE_CXX_BINDING
        if (op_ptr->language == MPIR_LANG__CXX) {
            uop = (MPI_User_function *) op_ptr->function.c_function;
            is_cxx_uop = 1;
        } else
#endif
        {
            if (op_ptr->language == MPIR_LANG__C) {
                uop = (MPI_User_function *) op_ptr->function.c_function;
            } else {
                uop = (MPI_User_function *) op_ptr->function.f77_function;
#if defined(HAVE_FORTRAN_BINDING) && !defined(HAVE_FINT_IS_INT)
                is_f77_uop = 1;
#endif
            }
        }
    }

    /* actually perform the reduction */
#ifdef HAVE_CXX_BINDING
    if (is_cxx_uop) {
        (*MPIR_Process.cxx_call_op_fn) (inbuf, inoutbuf, count, datatype, uop);
    } else
#endif
    {
#if defined(HAVE_FORTRAN_BINDING) && !defined(HAVE_FINT_IS_INT)
        if (is_f77_uop) {
            MPI_Fint lcount = (MPI_Fint) count;
            MPI_Fint ldtype = (MPI_Fint) datatype;
            MPII_F77_User_function *uop_f77 = (MPII_F77_User_function *) uop;

            (*uop_f77) ((void *) inbuf, inoutbuf, &lcount, &ldtype);
        } else {
            (*uop) ((void *) inbuf, inoutbuf, &count, &datatype);
        }
#else
        (*uop) ((void *) inbuf, inoutbuf, &count, &datatype);
#endif
    }

  fn_exit:
    return mpi_errno;
}
