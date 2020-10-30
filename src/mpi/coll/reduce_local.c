/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Reduce_local */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Reduce_local = PMPI_Reduce_local
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Reduce_local  MPI_Reduce_local
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Reduce_local as PMPI_Reduce_local
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Reduce_local(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op)
    __attribute__ ((weak, alias("PMPI_Reduce_local")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Reduce_local
#define MPI_Reduce_local PMPI_Reduce_local
#endif

/*@
MPI_Reduce_local - Applies a reduction operator to local arguments.

Input Parameters:
+ inbuf - address of the input buffer (choice)
. count - number of elements in each buffer (integer)
. datatype - data type of elements in the buffers (handle)
- op - reduction operation (handle)

Output Parameters:
. inoutbuf - address of input-output buffer (choice)

.N ThreadSafe

.N Fortran

.N collops

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
.N MPI_ERR_BUFFER_ALIAS
@*/
int MPI_Reduce_local(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_REDUCE_LOCAL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_REDUCE_LOCAL);

    /* Validate parameters */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_OP(op, mpi_errno);

            if (!HANDLE_IS_BUILTIN(op)) {
                MPIR_Op *op_ptr;
                MPIR_Op_get_ptr(op, op_ptr);
                MPIR_Op_valid_ptr(op_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            } else {
                mpi_errno = (*MPIR_OP_HDL_TO_DTYPE_FN(op)) (datatype);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }
            if (count != 0) {
                MPIR_ERRTEST_ALIAS_COLL(inbuf, inoutbuf, mpi_errno);
            }
            MPIR_ERRTEST_NAMED_BUF_INPLACE(inbuf, "inbuf", count, mpi_errno);
            MPIR_ERRTEST_NAMED_BUF_INPLACE(inoutbuf, "inoutbuf", count, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */


    /* ... body of routine ...  */

    mpi_errno = MPIR_Reduce_local(inbuf, inoutbuf, count, datatype, op);
    MPIR_ERR_CHECK(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_REDUCE_LOCAL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_reduce_local", "**mpi_reduce_local %p %p %d %D %O", inbuf,
                                 inoutbuf, count, datatype, op);
    }
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
