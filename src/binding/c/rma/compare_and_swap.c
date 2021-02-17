/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Compare_and_swap */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Compare_and_swap = PMPI_Compare_and_swap
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Compare_and_swap  MPI_Compare_and_swap
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Compare_and_swap as PMPI_Compare_and_swap
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Compare_and_swap(const void *origin_addr, const void *compare_addr, void *result_addr,
                         MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Win win)
                          __attribute__ ((weak, alias("PMPI_Compare_and_swap")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Compare_and_swap
#define MPI_Compare_and_swap PMPI_Compare_and_swap

#endif

/*@
   MPI_Compare_and_swap - Perform one-sided atomic compare-and-swap.

Input Parameters:
+ origin_addr - initial address of buffer (choice)
. compare_addr - initial address of compare buffer (choice)
. datatype - datatype of the element in all buffers (handle)
. target_rank - rank of target (non-negative integer)
. target_disp - displacement from start of window to beginning of target buffer (non-negative integer)
- win - window object (handle)

Output Parameters:
. result_addr - initial address of result buffer (choice)

Notes:
This operation is atomic with respect to other "accumulate" operations.

The parameter datatype must belong to one of the following categories of
predefined datatypes: C integer, Fortran integer, Logical, Multi-language
types, or Byte as specified in Section 5.9.2 on page 176. The origin and result
buffers (origin_addr and result_addr) must be disjoint.

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
.N MPI_ERR_DISP
.N MPI_ERR_RANK
.N MPI_ERR_WIN
@*/

int MPI_Compare_and_swap(const void *origin_addr, const void *compare_addr, void *result_addr,
                         MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Win win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMPARE_AND_SWAP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMPARE_AND_SWAP);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_WIN(win, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    MPIR_Win_get_ptr(win, win_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Win_valid_ptr(win_ptr, mpi_errno);
            if (mpi_errno) {
                goto fn_fail;
            }
            MPIR_ERRTEST_ARGNULL(origin_addr, "origin_addr", mpi_errno);
            MPIR_ERRTEST_ARGNULL(compare_addr, "compare_addr", mpi_errno);
            MPIR_ERRTEST_ARGNULL(result_addr, "result_addr", mpi_errno);
            MPIR_ERRTEST_SEND_RANK(win_ptr->comm_ptr, target_rank, mpi_errno);
            if (win_ptr->create_flavor != MPI_WIN_FLAVOR_DYNAMIC) {
                MPIR_ERRTEST_DISP(target_disp, mpi_errno);
            }
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            /* Check if datatype is a C integer, Fortran Integer,
             * logical, or byte, per the classes given on page 165. */
            MPIR_ERRTEST_TYPE_RMA_ATOMIC(datatype, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Return immediately for dummy process */
    if (unlikely(target_rank == MPI_PROC_NULL)) {
        goto fn_exit;
    }

    /* ... body of routine ... */
    mpi_errno = MPID_Compare_and_swap(origin_addr, compare_addr, result_addr, datatype, target_rank,
                                      target_disp, win_ptr);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMPARE_AND_SWAP);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_compare_and_swap",
                                     "**mpi_compare_and_swap %p %p %p %D %d %L %W", origin_addr,
                                     compare_addr, result_addr, datatype, target_rank,
                                     (long long) target_disp, win);
#endif
    mpi_errno = MPIR_Err_return_win(win_ptr, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
