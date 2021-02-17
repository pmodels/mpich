/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_allocate */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_allocate = PMPI_Win_allocate
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_allocate  MPI_Win_allocate
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_allocate as PMPI_Win_allocate
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr,
                     MPI_Win *win)  __attribute__ ((weak, alias("PMPI_Win_allocate")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_allocate
#define MPI_Win_allocate PMPI_Win_allocate

#endif

/*@
   MPI_Win_allocate - Create and allocate an MPI Window object for one-sided communication

Input Parameters:
+ size - size of window in bytes (non-negative integer)
. disp_unit - local unit size for displacements, in bytes (positive integer)
. info - info argument (handle)
- comm - intra-communicator (handle)

Output Parameters:
+ baseptr - initial address of window (choice)
- win - window object returned by call (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
.N MPI_ERR_COMM
.N MPI_ERR_DISP
.N MPI_ERR_INFO
.N MPI_ERR_SIZE
.seealso: MPI_Win_allocate_shared MPI_Win_create MPI_Win_create_dynamic MPI_Win_free
@*/

int MPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr,
                     MPI_Win *win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Info *info_ptr = NULL;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WIN_ALLOCATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_WIN_ALLOCATE);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INFO_OR_NULL(info, mpi_errno);
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    if (info != MPI_INFO_NULL) {
        MPIR_Info_get_ptr(info, info_ptr);
    }
    MPIR_Comm_get_ptr(comm, comm_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (info != MPI_INFO_NULL) {
                MPIR_Info_valid_ptr(info_ptr, mpi_errno);
                if (mpi_errno) {
                    goto fn_fail;
                }
            }
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno) {
                goto fn_fail;
            }
            MPIR_ERRTEST_WIN_SIZE(size, mpi_errno);
            MPIR_ERRTEST_WIN_DISPUNIT(disp_unit, mpi_errno);
            MPIR_ERRTEST_ARGNULL(baseptr, "baseptr", mpi_errno);
            MPIR_ERRTEST_ARGNULL(win, "win", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    MPIR_Win *win_ptr = NULL;
    *win = MPI_WIN_NULL;
    mpi_errno = MPID_Win_allocate(size, disp_unit, info_ptr, comm_ptr, baseptr, &win_ptr);
    if (mpi_errno) {
        goto fn_fail;
    }
    if (win_ptr) {
        /* Initialize a few fields that have specific defaults */
        win_ptr->name[0] = 0;
        win_ptr->errhandler = 0;
        MPIR_OBJ_PUBLISH_HANDLE(*win, win_ptr->handle);
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_WIN_ALLOCATE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_win_allocate", "**mpi_win_allocate %L %d %I %C %p %p",
                                     (long long) size, disp_unit, info, comm, baseptr, win);
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
