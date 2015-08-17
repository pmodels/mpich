/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Accumulate */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Accumulate = PMPI_Accumulate
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Accumulate  MPI_Accumulate
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Accumulate as PMPI_Accumulate
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                   int target_rank, MPI_Aint target_disp, int target_count,
                   MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
                   __attribute__((weak,alias("PMPI_Accumulate")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Accumulate
#define MPI_Accumulate PMPI_Accumulate

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Accumulate

/*@
   MPI_Accumulate - Accumulate data into the target process using remote 
   memory access 

Input Parameters:
+ origin_addr - initial address of buffer (choice) 
. origin_count - number of entries in buffer (nonnegative integer) 
. origin_datatype - datatype of each buffer entry (handle) 
. target_rank - rank of target (nonnegative integer) 
. target_disp - displacement from start of window to beginning of target 
  buffer (nonnegative integer)  
. target_count - number of entries in target buffer (nonnegative integer) 
. target_datatype - datatype of each entry in target buffer (handle) 
. op - predefined reduce operation (handle) 
- win - window object (handle) 

   Notes:
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

.seealso: MPI_Raccumulate
@*/
int MPI_Accumulate(const void *origin_addr, int origin_count, MPI_Datatype
                   origin_datatype, int target_rank, MPI_Aint
                   target_disp, int target_count, MPI_Datatype
                   target_datatype, MPI_Op op, MPI_Win win) 
{
    static const char FCNAME[] = "MPI_Accumulate";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ACCUMULATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_ACCUMULATE);

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
            MPIR_ERRTEST_OP_ACC(op, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Accumulate(origin_addr, origin_count,
                                origin_datatype,
                                target_rank, target_disp, target_count,
                                target_datatype, op, win_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_ACCUMULATE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_accumulate",
	    "**mpi_accumulate %p %d %D %d %d %d %D %O %W", origin_addr, origin_count, origin_datatype,
	    target_rank, target_disp, target_count, target_datatype, op, win);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

