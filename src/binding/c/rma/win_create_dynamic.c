/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_create_dynamic */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_create_dynamic = PMPI_Win_create_dynamic
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_create_dynamic  MPI_Win_create_dynamic
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_create_dynamic as PMPI_Win_create_dynamic
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win)
     __attribute__ ((weak, alias("PMPI_Win_create_dynamic")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_create_dynamic
#define MPI_Win_create_dynamic PMPI_Win_create_dynamic

#endif

/*@
   MPI_Win_create_dynamic - Create an MPI Window object for one-sided communication

Input Parameters:
+ info - info argument (handle)
- comm - intra-communicator (handle)

Output Parameters:
. win - window object returned by the call (handle)

Notes:

Users are cautioned that displacement arithmetic can overflow in variables of
type 'MPI_Aint' and result in unexpected values on some platforms. This issue may
be addressed in a future version of MPI.

Memory in this window may not be used as the target of one-sided accesses in
this window until it is attached using the function 'MPI_Win_attach'. That is, in
addition to using 'MPI_Win_create_dynamic' to create an MPI window, the user must
use 'MPI_Win_attach' before any local memory may be the target of an MPI RMA
operation. Only memory that is currently accessible may be attached.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
.N MPI_ERR_COMM
.N MPI_ERR_INFO
.seealso: MPI_Win_attach MPI_Win_detach MPI_Win_allocate MPI_Win_allocate_shared MPI_Win_create MPI_Win_free
@*/

int MPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Info *info_ptr = NULL;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WIN_CREATE_DYNAMIC);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_WIN_CREATE_DYNAMIC);

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
            MPIR_ERRTEST_ARGNULL(win, "win", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    MPIR_Win *win_ptr = NULL;
    *win = MPI_WIN_NULL;
    mpi_errno = MPID_Win_create_dynamic(info_ptr, comm_ptr, &win_ptr);
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
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_WIN_CREATE_DYNAMIC);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_win_create_dynamic", "**mpi_win_create_dynamic %I %C %p",
                                     info, comm, win);
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
