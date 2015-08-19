/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_create = PMPI_Win_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_create  MPI_Win_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_create as PMPI_Win_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm,
                   MPI_Win *win) __attribute__((weak,alias("PMPI_Win_create")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_create
#define MPI_Win_create PMPI_Win_create

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_create

/*@
MPI_Win_create - Create an MPI Window object for one-sided communication


This is a collective call executed by all processes in the group of comm. It
returns a window object that can be used by these processes to perform RMA
operations. Each process specifies a window of existing memory that it exposes
to RMA accesses by the processes in the group of comm. The window consists of
size bytes, starting at address base. In C, base is the starting address of a
memory region. In Fortran, one can pass the first element of a memory region or
a whole array, which must be ''simply contiguous'' (for ''simply contiguous'', see
also MPI 3.0, Section 17.1.12 on page 626). A process may elect to expose no
memory by specifying size = 0.

Input Parameters:
+ base - initial address of window (choice)
. size - size of window in bytes (nonnegative integer)
. disp_unit - local unit size for displacements, in bytes (positive integer)
. info - info argument (handle)
- comm - communicator (handle)

Output Parameters:
. win - window object returned by the call (handle)

Notes:

The displacement unit argument is provided to facilitate address arithmetic in
RMA operations: the target displacement argument of an RMA operation is scaled
by the factor disp_unit specified by the target process, at window creation.

The info argument provides optimization hints to the runtime about the expected
usage pattern of the window. The following info keys are predefined.

. no_locks - If set to true, then the implementation may assume that passive
    target synchronization (i.e., 'MPI_Win_lock', 'MPI_Win_lock_all') will not be used on
    the given window. This implies that this window is not used for 3-party
    communication, and RMA can be implemented with no (less) asynchronous agent
    activity at this process.

. accumulate_ordering - Controls the ordering of accumulate operations at the
    target.  The argument string should contain a comma-separated list of the
    following read/write ordering rules, where e.g. "raw" means read-after-write:
    "rar,raw,war,waw".

. accumulate_ops - If set to same_op, the implementation will assume that all
    concurrent accumulate calls to the same target address will use the same
    operation. If set to same_op_no_op, then the implementation will assume that
    all concurrent accumulate calls to the same target address will use the same
    operation or 'MPI_NO_OP'. This can eliminate the need to protect access for
    certain operation types where the hardware can guarantee atomicity. The default
    is same_op_no_op.

.N ThreadSafe
.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_COMM
.N MPI_ERR_INFO
.N MPI_ERR_OTHER
.N MPI_ERR_SIZE

.seealso: MPI_Win_allocate MPI_Win_allocate_shared MPI_Win_create_dynamic MPI_Win_free
@*/
int MPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, 
		   MPI_Comm comm, MPI_Win *win)
{
    static const char FCNAME[] = "MPI_Win_create";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_Comm *comm_ptr = NULL;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_CREATE);

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
            if (size < 0)
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
						  MPIR_ERR_RECOVERABLE, 
						  FCNAME, __LINE__, 
						  MPI_ERR_SIZE,
                                                  "**rmasize",
                                                  "**rmasize %d", size);  
            if (disp_unit <= 0)
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
						  MPIR_ERR_RECOVERABLE, 
						  FCNAME, __LINE__, 
						  MPI_ERR_ARG,
                                                 "**arg", "**arg %s", 
                                                 "disp_unit must be positive");
            if (size > 0 && base == NULL)
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
                                                  MPIR_ERR_RECOVERABLE, 
                                                  FCNAME, __LINE__, 
                                                  MPI_ERR_ARG,
                                                  "**nullptr",
                                                  "**nullptr %s",
                                                  "NULL base pointer is invalid when size is nonzero");  
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Win_create(base, size, disp_unit, info_ptr, 
				comm_ptr, &win_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* Initialize a few fields that have specific defaults */
    win_ptr->name[0]    = 0;
    win_ptr->errhandler = 0;

    /* return the handle of the window object to the user */
    MPID_OBJ_PUBLISH_HANDLE(*win, win_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_CREATE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_win_create", 
	    "**mpi_win_create %p %d %d %I %C %p", base, size, disp_unit, info, comm, win);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
