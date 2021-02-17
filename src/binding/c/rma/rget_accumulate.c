/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Rget_accumulate */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Rget_accumulate = PMPI_Rget_accumulate
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Rget_accumulate  MPI_Rget_accumulate
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Rget_accumulate as PMPI_Rget_accumulate
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Rget_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                        void *result_addr, int result_count, MPI_Datatype result_datatype,
                        int target_rank, MPI_Aint target_disp, int target_count,
                        MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request *request)
                         __attribute__ ((weak, alias("PMPI_Rget_accumulate")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Rget_accumulate
#define MPI_Rget_accumulate PMPI_Rget_accumulate

#endif

/*@
   MPI_Rget_accumulate - Perform an atomic, one-sided read-and-accumulate

Input Parameters:
+ origin_addr - initial address of buffer (choice)
. origin_count - number of entries in origin buffer (non-negative integer)
. origin_datatype - datatype of each entry in origin buffer (handle)
. result_count - number of entries in result buffer (non-negative integer)
. result_datatype - datatype of entries in result buffer (handle)
. target_rank - rank of target (non-negative integer)
. target_disp - displacement from start of window to beginning of target buffer (non-negative integer)
. target_count - number of entries in target buffer (non-negative integer)
. target_datatype - datatype of each entry in target buffer (handle)
. op - reduce operation (handle)
- win - window object (handle)

Output Parameters:
+ result_addr - initial address of result buffer (choice)
- request - RMA request (handle)

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
.N MPI_ERR_DISP
.N MPI_ERR_RANK
.N MPI_ERR_TYPE
.N MPI_ERR_WIN
.seealso: MPI_Get_accumulate MPI_Fetch_and_op
@*/

int MPI_Rget_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                        void *result_addr, int result_count, MPI_Datatype result_datatype,
                        int target_rank, MPI_Aint target_disp, int target_count,
                        MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_RGET_ACCUMULATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_RGET_ACCUMULATE);

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
            if (op != MPI_NO_OP) {
                MPIR_ERRTEST_COUNT(origin_count, mpi_errno);
                MPIR_ERRTEST_DATATYPE(origin_datatype, "datatype", mpi_errno);
                if (!HANDLE_IS_BUILTIN(origin_datatype)) {
                    MPIR_Datatype *datatype_ptr = NULL;
                    MPIR_Datatype_get_ptr(origin_datatype, datatype_ptr);
                    MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                    if (mpi_errno) {
                        goto fn_fail;
                    }
                    MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                    if (mpi_errno) {
                        goto fn_fail;
                    }
                }
                MPIR_ERRTEST_USERBUFFER(origin_addr, origin_count, origin_datatype, mpi_errno);
            }
            MPIR_ERRTEST_COUNT(result_count, mpi_errno);
            MPIR_ERRTEST_DATATYPE(result_datatype, "datatype", mpi_errno);
            if (!HANDLE_IS_BUILTIN(result_datatype)) {
                MPIR_Datatype *datatype_ptr = NULL;
                MPIR_Datatype_get_ptr(result_datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno) {
                    goto fn_fail;
                }
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno) {
                    goto fn_fail;
                }
            }
            MPIR_ERRTEST_USERBUFFER(result_addr, result_count, result_datatype, mpi_errno);
            MPIR_ERRTEST_SEND_RANK(win_ptr->comm_ptr, target_rank, mpi_errno);
            if (win_ptr->create_flavor != MPI_WIN_FLAVOR_DYNAMIC) {
                MPIR_ERRTEST_DISP(target_disp, mpi_errno);
            }
            MPIR_ERRTEST_COUNT(target_count, mpi_errno);
            MPIR_ERRTEST_DATATYPE(target_datatype, "datatype", mpi_errno);
            if (!HANDLE_IS_BUILTIN(target_datatype)) {
                MPIR_Datatype *datatype_ptr = NULL;
                MPIR_Datatype_get_ptr(target_datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno) {
                    goto fn_fail;
                }
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno) {
                    goto fn_fail;
                }
            }
            MPIR_ERRTEST_OP_GACC(op, mpi_errno);
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Return immediately for dummy process */
    if (unlikely(target_rank == MPI_PROC_NULL)) {
        MPIR_Request *request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RMA);
        MPIR_ERR_CHKANDSTMT(request_ptr == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        *request = request_ptr->handle;
        goto fn_exit;
    }

    /* ... body of routine ... */
    MPIR_Request *request_ptr = NULL;
    *request = MPI_REQUEST_NULL;
    mpi_errno = MPID_Rget_accumulate(origin_addr, origin_count, origin_datatype, result_addr,
                                     result_count, result_datatype, target_rank, target_disp,
                                     target_count, target_datatype, op, win_ptr, &request_ptr);
    if (mpi_errno) {
        goto fn_fail;
    }
    if (request_ptr) {
        MPIR_OBJ_PUBLISH_HANDLE(*request, request_ptr->handle);
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_RGET_ACCUMULATE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_rget_accumulate",
                                     "**mpi_rget_accumulate %p %d %D %p %d %D %d %L %d %D %O %W %p",
                                     origin_addr, origin_count, origin_datatype, result_addr,
                                     result_count, result_datatype, target_rank,
                                     (long long) target_disp, target_count, target_datatype, op, win,
                                     request);
#endif
    mpi_errno = MPIR_Err_return_win(win_ptr, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
