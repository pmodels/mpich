/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Fetch_and_op */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Fetch_and_op = PMPI_Fetch_and_op
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Fetch_and_op  MPI_Fetch_and_op
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Fetch_and_op as PMPI_Fetch_and_op
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Fetch_and_op(const void *origin_addr, void *result_addr,
                      MPI_Datatype datatype, int target_rank, MPI_Aint target_disp,
                      MPI_Op op, MPI_Win win)
                      __attribute__((weak,alias("PMPI_Fetch_and_op")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Fetch_and_op
#define MPI_Fetch_and_op PMPI_Fetch_and_op

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Fetch_and_op

/*@
MPI_Fetch_and_op - Perform one-sided read-modify-write.


Accumulate one element of type datatype from the origin buffer (origin_addr) to
the buffer at offset target_disp, in the target window specified by target_rank
and win, using the operation op and return in the result buffer result_addr the
content of the target buffer before the accumulation.

Input Parameters:
+ origin_addr - initial address of buffer (choice)
. result_addr - initial address of result buffer (choice)
. datatype - datatype of the entry in origin, result, and target buffers (handle)
. target_rank - rank of target (nonnegative integer)
. target_disp - displacement from start of window to beginning of target buffer (non-negative integer)
. op - reduce operation (handle)
- win - window object (handle)

Notes:
This operations is atomic with respect to other "accumulate" operations.

The generic functionality of 'MPI_Get_accumulate' might limit the performance of
fetch-and-increment or fetch-and-add calls that might be supported by special
hardware operations. 'MPI_Fetch_and_op' thus allows for a fast implementation
of a commonly used subset of the functionality of 'MPI_Get_accumulate'.

The origin and result buffers (origin_addr and result_addr) must be disjoint.
Any of the predefined operations for 'MPI_Reduce', as well as 'MPI_NO_OP' or
'MPI_REPLACE', can be specified as op; user-defined functions cannot be used. The
datatype argument must be a predefined datatype.

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_COUNT
.N MPI_ERR_OP
.N MPI_ERR_RANK
.N MPI_ERR_TYPE
.N MPI_ERR_WIN

.seealso: MPI_Get_accumulate
@*/
int MPI_Fetch_and_op(const void *origin_addr, void *result_addr,
        MPI_Datatype datatype, int target_rank, MPI_Aint target_disp,
        MPI_Op op, MPI_Win win)
{
    static const char FCNAME[] = "MPI_Fetch_and_op";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_FETCH_AND_OP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_FETCH_AND_OP);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_WIN(win, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI object handles to object pointers */
    MPID_Win_get_ptr( win, win_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Comm *comm_ptr;
            
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;

            if (op != MPI_NO_OP) {
                /* NOTE: when op is MPI_NO_OP, origin_addr is allowed to be NULL.
                 * In such case, MPI_Fetch_and_op equals to an atomic GET. */
                MPIR_ERRTEST_ARGNULL(origin_addr, "origin_addr", mpi_errno);
            }

            MPIR_ERRTEST_ARGNULL(result_addr, "result_addr", mpi_errno);
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

            if (!MPIR_DATATYPE_IS_PREDEFINED(datatype))
            {
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_TYPE, "**typenotpredefined");
            }

            if (win_ptr->create_flavor != MPI_WIN_FLAVOR_DYNAMIC)
                MPIR_ERRTEST_DISP(target_disp, mpi_errno);

            comm_ptr = win_ptr->comm_ptr;
            MPIR_ERRTEST_SEND_RANK(comm_ptr, target_rank, mpi_errno);

            MPIR_ERRTEST_OP_GACC(op, mpi_errno);

            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN)
            {
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OP, "**opnotpredefined");
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Fetch_and_op(origin_addr,
                                  result_addr, datatype,
                                  target_rank, target_disp,
                                  op, win_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_FETCH_AND_OP);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_fetch_and_op",
            "**mpi_fetch_and_op %p %p %D %d %d %O %W", origin_addr,
            result_addr, datatype, target_rank, target_disp, op, win);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

