/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_create_dynamic */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_create_dynamic = PMPI_Win_create_dynamic
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_create_dynamic  MPI_Win_create_dynamic
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_create_dynamic as PMPI_Win_create_dynamic
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win) __attribute__((weak,alias("PMPI_Win_create_dynamic")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_create_dynamic
#define MPI_Win_create_dynamic PMPI_Win_create_dynamic

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_create_dynamic

/*@
MPI_Win_create_dynamic - Create an MPI Window object for one-sided
communication.  This window allows memory to be dynamically exposed and
un-exposed for RMA operations.


This is a collective call executed by all processes in the group of comm. It
returns a window win without memory attached. Existing process memory can be
attached as described below. This routine returns a window object that can be
used by these processes to perform RMA operations on attached memory. Because
this window has special properties, it will sometimes be referred to as a
dynamic window.  The info argument can be used to specify hints similar to the
info argument for 'MPI_Win_create'.

In the case of a window created with 'MPI_Win_create_dynamic', the target_disp
for all RMA functions is the address at the target; i.e., the effective
window_base is 'MPI_BOTTOM' and the disp_unit is one. For dynamic windows, the
target_disp argument to RMA communication operations is not restricted to
non-negative values. Users should use 'MPI_Get_address' at the target process to
determine the address of a target memory location and communicate this address
to the origin process.

Input Parameters:
+ info - info argument (handle)
- comm - communicator (handle)

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
.N MPI_ERR_OTHER
.N MPI_ERR_SIZE

.seealso: MPI_Win_attach MPI_Win_detach MPI_Win_allocate MPI_Win_allocate_shared MPI_Win_create MPI_Win_free
@*/
int MPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
    static const char FCNAME[] = "MPI_Win_create_dynamic";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_Comm *comm_ptr = NULL;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_CREATE_DYNAMIC);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_CREATE_DYNAMIC);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
            MPIR_ERRTEST_INFO_OR_NULL(info, mpi_errno);
            MPIR_ERRTEST_ARGNULL(win, "win", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );
    MPID_Info_get_ptr( info, info_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate pointers */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Win_create_dynamic(info_ptr, comm_ptr, &win_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* Initialize a few fields that have specific defaults */
    win_ptr->name[0]    = 0;
    win_ptr->errhandler = 0;

    /* return the handle of the window object to the user */
    MPID_OBJ_PUBLISH_HANDLE(*win, win_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_CREATE_DYNAMIC);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_win_create_dynamic",
            "**mpi_win_create_dynamic %I %C %p", info, comm, win);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
