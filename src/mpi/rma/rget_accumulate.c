/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Rget_accumulate */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Rget_accumulate = PMPI_Rget_accumulate
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Rget_accumulate  MPI_Rget_accumulate
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Rget_accumulate as PMPI_Rget_accumulate
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Rget_accumulate(const void *origin_addr, int origin_count,
                         MPI_Datatype origin_datatype, void *result_addr, int result_count,
                         MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                         int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                         MPI_Request *request)
                         __attribute__((weak,alias("PMPI_Rget_accumulate")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Rget_accumulate
#define MPI_Rget_accumulate PMPI_Rget_accumulate

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Rget_accumulate

/*@
MPI_Rget_accumulate - Perform an atomic, one-sided read-and-accumulate
operation and return a request handle for the operation.


'MPI_Rget_accumulate' is similar to 'MPI_Get_accumulate', except that it allocates
a communication request object and associates it with the request handle (the
argument request) that can be used to wait or test for completion. The
completion of an 'MPI_Rget_accumulate' operation indicates that the data is
available in the result buffer and the origin buffer is free to be updated. It
does not indicate that the operation has been completed at the target window.

Input Parameters:
+ origin_addr - initial address of buffer (choice)
. origin_count - number of entries in buffer (nonnegative integer)
. origin_datatype - datatype of each buffer entry (handle)
. result_addr - initial address of result buffer (choice)
. result_count - number of entries in result buffer (non-negative integer)
. result_datatype - datatype of each entry in result buffer (handle)
. target_rank - rank of target (nonnegative integer)
. target_disp - displacement from start of window to beginning of target
  buffer (nonnegative integer)
. target_count - number of entries in target buffer (nonnegative integer)
. target_datatype - datatype of each entry in target buffer (handle)
. op - predefined reduce operation (handle)
- win - window object (handle)

Output Parameters:
. request - RMA request (handle)

Notes:
This operations is atomic with respect to other "accumulate" operations.

The get and accumulate steps are executed atomically for each basic element in
the datatype (see MPI 3.0 Section 11.7 for details). The predefined operation
'MPI_REPLACE' provides fetch-and-set behavior.

The basic components of both the origin and target datatype must be the same
predefined datatype (e.g., all 'MPI_INT' or all 'MPI_DOUBLE_PRECISION').

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_COUNT
.N MPI_ERR_RANK
.N MPI_ERR_TYPE
.N MPI_ERR_WIN

.seealso: MPI_Get_accumulate MPI_Fetch_and_op
@*/
int MPI_Rget_accumulate(const void *origin_addr, int origin_count,
        MPI_Datatype origin_datatype, void *result_addr, int result_count,
        MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
        int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
        MPI_Request *request)
{
    static const char FCNAME[] = "MPI_Rget_accumulate";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_Request *request_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_RGET_ACCUMULATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_RGET_ACCUMULATE);

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
            MPID_Comm * comm_ptr;
            
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;

            if (op != MPI_NO_OP) {
                MPIR_ERRTEST_COUNT(origin_count, mpi_errno);
                MPIR_ERRTEST_DATATYPE(origin_datatype, "origin_datatype", mpi_errno);
                MPIR_ERRTEST_USERBUFFER(origin_addr, origin_count, origin_datatype, mpi_errno);
            }
            MPIR_ERRTEST_COUNT(result_count, mpi_errno);
            MPIR_ERRTEST_DATATYPE(result_datatype, "result_datatype", mpi_errno);
            MPIR_ERRTEST_USERBUFFER(result_addr, result_count, result_datatype, mpi_errno);
            MPIR_ERRTEST_COUNT(target_count, mpi_errno);
            MPIR_ERRTEST_DATATYPE(target_datatype, "target_datatype", mpi_errno);
            if (win_ptr->create_flavor != MPI_WIN_FLAVOR_DYNAMIC)
                MPIR_ERRTEST_DISP(target_disp, mpi_errno);

            if (op != MPI_NO_OP &&
                HANDLE_GET_KIND(origin_datatype) != HANDLE_KIND_BUILTIN)
            {
                MPID_Datatype *datatype_ptr = NULL;
                
                MPID_Datatype_get_ptr(origin_datatype, datatype_ptr);
                MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            if (HANDLE_GET_KIND(result_datatype) != HANDLE_KIND_BUILTIN)
            {
                MPID_Datatype *datatype_ptr = NULL;
                
                MPID_Datatype_get_ptr(origin_datatype, datatype_ptr);
                MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            if (HANDLE_GET_KIND(target_datatype) != HANDLE_KIND_BUILTIN)
            {
                MPID_Datatype *datatype_ptr = NULL;
                
                MPID_Datatype_get_ptr(target_datatype, datatype_ptr);
                MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            comm_ptr = win_ptr->comm_ptr;
            MPIR_ERRTEST_SEND_RANK(comm_ptr, target_rank, mpi_errno);
            MPIR_ERRTEST_OP_GACC(op, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Rget_accumulate(origin_addr, origin_count,
                                     origin_datatype,
                                     result_addr, result_count,
                                     result_datatype,
                                     target_rank, target_disp, target_count,
                                     target_datatype, op, win_ptr,
                                     &request_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_RGET_ACCUMULATE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_rget_accumulate",
            "**mpi_rget_accumulate %p %d %D %p %d %D %d %d %d %D %O %W %p", origin_addr, origin_count, origin_datatype,
            result_addr, result_count, result_datatype,
            target_rank, target_disp, target_count, target_datatype, op, win, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

