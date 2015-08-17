/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Rget */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Rget = PMPI_Rget
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Rget  MPI_Rget
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Rget as PMPI_Rget
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Rget(void *origin_addr, int origin_count,
              MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
              int target_count, MPI_Datatype target_datatype, MPI_Win win,
              MPI_Request *request)
              __attribute__((weak,alias("PMPI_Rget")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Rget
#define MPI_Rget PMPI_Rget

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Rget

/*@
MPI_Rget - Get data from a memory window on a remote process


'MPI_Rget' is similar to 'MPI_Get', except that it allocates a communication
request object and associates it with the request handle (the argument request)
that can be used to wait or test for completion. The completion of an 'MPI_Rget'
operation indicates that the data is available in the origin buffer. If
origin_addr points to memory attached to a window, then the data becomes
available in the private copy of this window.

Input Parameters:
+ origin_addr - Address of the buffer in which to receive the data
. origin_count - number of entries in origin buffer (nonnegative integer)
. origin_datatype - datatype of each entry in origin buffer (handle)
. target_rank - rank of target (nonnegative integer)
. target_disp - displacement from window start to the beginning of the
  target buffer (nonnegative integer)
. target_count - number of entries in target buffer (nonnegative integer)
. target_datatype - datatype of each entry in target buffer (handle)
- win - window object used for communication (handle)

Output Parameters:
. request - RMA request (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_COUNT
.N MPI_ERR_RANK
.N MPI_ERR_TYPE
.N MPI_ERR_WIN

.seealso: MPI_Get
@*/
int MPI_Rget(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPI_Win
            win, MPI_Request *request)
{
    static const char FCNAME[] = "MPI_Rget";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_Request *request_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_RGET);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_RGET);

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

            MPIR_ERRTEST_COUNT(origin_count, mpi_errno);
            MPIR_ERRTEST_DATATYPE(origin_datatype, "origin_datatype", mpi_errno);
            MPIR_ERRTEST_USERBUFFER(origin_addr, origin_count, origin_datatype, mpi_errno);
            MPIR_ERRTEST_COUNT(target_count, mpi_errno);
            MPIR_ERRTEST_DATATYPE(target_datatype, "target_datatype", mpi_errno);
            if (win_ptr->create_flavor != MPI_WIN_FLAVOR_DYNAMIC)
                MPIR_ERRTEST_DISP(target_disp, mpi_errno);

            if (HANDLE_GET_KIND(origin_datatype) != HANDLE_KIND_BUILTIN)
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
            MPIR_ERRTEST_ARGNULL(request,"request",mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Rget(origin_addr, origin_count, origin_datatype,
                          target_rank, target_disp, target_count,
                          target_datatype, win_ptr, &request_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_RGET);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_rget", "**mpi_rget %p %d %D %d %d %d %D %W %p",
            origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

