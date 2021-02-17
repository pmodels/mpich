/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_shared_query */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_shared_query = PMPI_Win_shared_query
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_shared_query  MPI_Win_shared_query
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_shared_query as PMPI_Win_shared_query
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_shared_query(MPI_Win win, int rank, MPI_Aint *size, int *disp_unit, void *baseptr)
     __attribute__ ((weak, alias("PMPI_Win_shared_query")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_shared_query
#define MPI_Win_shared_query PMPI_Win_shared_query

#endif

/*@
   MPI_Win_shared_query - Query the size and base pointer for a patch of a shared memory window

Input Parameters:
+ win - shared memory window object (handle)
- rank - rank in the group of window win or MPI_PROC_NULL (non-negative integer)

Output Parameters:
+ size - size of the window segment (non-negative integer)
. disp_unit - local unit size for displacements, in bytes (positive integer)
- baseptr - address for load/store access to window segment (choice)

Notes:
The returned baseptr points to the calling process' address space of the
shared segment belonging to the target rank.

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
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WIN_SHARED_QUERY);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_WIN_SHARED_QUERY);

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
            MPIR_ERRTEST_WIN_FLAVOR(win_ptr, MPI_WIN_FLAVOR_SHARED, mpi_errno);
            MPIR_ERRTEST_SEND_RANK(win_ptr->comm_ptr, rank, mpi_errno);
            MPIR_ERRTEST_ARGNULL(size, "size", mpi_errno);
            MPIR_ERRTEST_ARGNULL(disp_unit, "disp_unit", mpi_errno);
            MPIR_ERRTEST_ARGNULL(baseptr, "baseptr", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPID_Win_shared_query(win_ptr, rank, size, disp_unit, baseptr);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_WIN_SHARED_QUERY);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_win_shared_query", "**mpi_win_shared_query %W %d %p %p %p",
                                     win, rank, size, disp_unit, baseptr);
#endif
    mpi_errno = MPIR_Err_return_win(win_ptr, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
