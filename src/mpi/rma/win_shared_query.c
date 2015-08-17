/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_shared_query */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_shared_query = PMPI_Win_shared_query
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_shared_query  MPI_Win_shared_query
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_shared_query as PMPI_Win_shared_query
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_shared_query(MPI_Win win, int rank, MPI_Aint *size, int *disp_unit, void *baseptr) __attribute__((weak,alias("PMPI_Win_shared_query")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_shared_query
#define MPI_Win_shared_query PMPI_Win_shared_query

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_shared_query

/*@
MPI_Win_shared_query - Query the size and base pointer for a patch of a shared
memory window.


This function queries the process-local address for remote memory segments
created with 'MPI_Win_allocate_shared'. This function can return different
process-local addresses for the same physical memory on different processes.

The returned memory can be used for load/store accesses subject to the
constraints defined in MPI 3.0, Section 11.7. This function can only be called
with windows of type 'MPI_Win_flavor_shared'. If the passed window is not of
flavor 'MPI_Win_flavor_shared', the error 'MPI_ERR_RMA_FLAVOR' is raised. When rank
is 'MPI_PROC_NULL', the pointer, disp_unit, and size returned are the pointer,
disp_unit, and size of the memory segment belonging the lowest rank that
specified size > 0. If all processes in the group attached to the window
specified size = 0, then the call returns size = 0 and a baseptr as if
'MPI_Alloc_mem' was called with size = 0.

Input Parameters:
+ win - window object used for communication (handle)
- rank - target rank

Output Parameters:
+ size - size of the segment at the given rank
. disp_unit - local unit size for displacements, in bytes (positive integer)
- baseptr - base pointer in the calling process'' address space of the shared
  segment belonging to the target rank.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_RANK
.N MPI_ERR_WIN

.seealso: MPI_Win_allocate_shared
@*/
int MPI_Win_shared_query(MPI_Win win, int rank, MPI_Aint *size, int *disp_unit, void *baseptr)
{
    static const char FCNAME[] = "MPI_Win_shared_query";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_SHARED_QUERY);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_SHARED_QUERY);

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

            MPIR_ERRTEST_ARGNULL(size, "size", mpi_errno);
            MPIR_ERRTEST_ARGNULL(disp_unit, "disp_unit", mpi_errno);
            MPIR_ERRTEST_ARGNULL(baseptr, "baseptr", mpi_errno);

            comm_ptr = win_ptr->comm_ptr;
            MPIR_ERRTEST_SEND_RANK(comm_ptr, rank, mpi_errno);
            MPIR_ERRTEST_WIN_FLAVOR(win_ptr, MPI_WIN_FLAVOR_SHARED, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Win_shared_query(win_ptr, rank, size, disp_unit, baseptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_SHARED_QUERY);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_win_shared_query", "**mpi_win_shared_query %W %d %p %p",
            win, rank, size, baseptr);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

