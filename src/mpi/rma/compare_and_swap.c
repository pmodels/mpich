/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Compare_and_swap */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Compare_and_swap = PMPI_Compare_and_swap
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Compare_and_swap  MPI_Compare_and_swap
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Compare_and_swap as PMPI_Compare_and_swap
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Compare_and_swap(const void *origin_addr, const void *compare_addr,
                          void *result_addr, MPI_Datatype datatype, int target_rank,
                          MPI_Aint target_disp, MPI_Win win)
                          __attribute__((weak,alias("PMPI_Compare_and_swap")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Compare_and_swap
#define MPI_Compare_and_swap PMPI_Compare_and_swap

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Compare_and_swap

/*@
MPI_Compare_and_swap - Perform one-sided atomic compare-and-swap.


This function compares one element of type datatype in the compare buffer
compare_addr with the buffer at offset target_disp in the target window
specified by target_rank and win and replaces the value at the target with the
value in the origin buffer origin_addr if the compare buffer and the target
buffer are identical. The original value at the target is returned in the
buffer result_addr.

Input Parameters:
+ origin_addr - initial address of buffer (choice)
. compare_addr - initial address of compare buffer (choice)
. result_addr - initial address of result buffer (choice)
. datatype - datatype of the entry in origin, result, and target buffers (handle)
. target_rank - rank of target (nonnegative integer)
. target_disp - displacement from start of window to beginning of target buffer (non-negative integer)
- win - window object (handle)

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
.N MPI_ERR_COUNT
.N MPI_ERR_OP
.N MPI_ERR_RANK
.N MPI_ERR_TYPE
.N MPI_ERR_WIN
@*/
int MPI_Compare_and_swap(const void *origin_addr, const void *compare_addr,
        void *result_addr, MPI_Datatype datatype, int target_rank,
        MPI_Aint target_disp, MPI_Win win)
{
    static const char FCNAME[] = "MPI_Compare_and_swap";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMPARE_AND_SWAP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_COMPARE_AND_SWAP);

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

            MPIR_ERRTEST_ARGNULL(origin_addr, "origin_addr", mpi_errno);
            MPIR_ERRTEST_ARGNULL(compare_addr, "compare_addr", mpi_errno);
            MPIR_ERRTEST_ARGNULL(result_addr, "result_addr", mpi_errno);

            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

            /* Check if datatype is a C integer, Fortran Integer,
               logical, or byte, per the classes given on page 165. */
            MPIR_ERRTEST_TYPE_RMA_ATOMIC(datatype, mpi_errno);

            if (win_ptr->create_flavor != MPI_WIN_FLAVOR_DYNAMIC)
                MPIR_ERRTEST_DISP(target_disp, mpi_errno);

            comm_ptr = win_ptr->comm_ptr;
            MPIR_ERRTEST_SEND_RANK(comm_ptr, target_rank, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Compare_and_swap(origin_addr,
                                      compare_addr, result_addr,
                                      datatype, target_rank,
                                      target_disp, win_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_COMPARE_AND_SWAP);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_compare_and_swap",
            "**mpi_compare_and_swap %p %p %p %D %d %d %W", origin_addr, compare_addr,
            result_addr, datatype, target_rank, target_disp, win);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

