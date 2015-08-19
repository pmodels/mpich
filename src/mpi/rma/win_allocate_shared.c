/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_allocate_shared */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_allocate_shared = PMPI_Win_allocate_shared
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_allocate_shared  MPI_Win_allocate_shared
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_allocate_shared as PMPI_Win_allocate_shared
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm,
                            void *baseptr, MPI_Win *win) __attribute__((weak,alias("PMPI_Win_allocate_shared")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_allocate_shared
#define MPI_Win_allocate_shared PMPI_Win_allocate_shared

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_allocate_shared
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/*@
MPI_Win_allocate_shared - Create an MPI Window object for one-sided
communication and shared memory access, and allocate memory at each process.

This is a collective call executed by all processes in the group of comm. On
each process i, it allocates memory of at least size bytes that is shared among
all processes in comm, and returns a pointer to the locally allocated segment
in baseptr that can be used for load/store accesses on the calling process. The
locally allocated memory can be the target of load/store accesses by remote
processes; the base pointers for other processes can be queried using the
function 'MPI_Win_shared_query'.

The call also returns a window object that can be used by all processes in comm
to perform RMA operations. The size argument may be different at each process
and size = 0 is valid. It is the user''s responsibility to ensure that the
communicator comm represents a group of processes that can create a shared
memory segment that can be accessed by all processes in the group. The
allocated memory is contiguous across process ranks unless the info key
alloc_shared_noncontig is specified. Contiguous across process ranks means that
the first address in the memory segment of process i is consecutive with the
last address in the memory segment of process i − 1.  This may enable the user
to calculate remote address offsets with local information only.

Input Parameters:
. size - size of window in bytes (nonnegative integer)
. disp_unit - local unit size for displacements, in bytes (positive integer)
. info - info argument (handle)
- comm - communicator (handle)

Output Parameters:
. baseptr - initial address of window (choice)
- win - window object returned by the call (handle)

.N ThreadSafe
.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_COMM
.N MPI_ERR_INFO
.N MPI_ERR_OTHER
.N MPI_ERR_SIZE

.seealso: MPI_Win_allocate MPI_Win_create MPI_Win_create_dynamic MPI_Win_free MPI_Win_shared_query
@*/
int MPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm,
                             void *baseptr, MPI_Win *win)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_Comm *comm_ptr = NULL;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_ALLOCATE_SHARED);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_ALLOCATE_SHARED);

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

            MPIR_ERR_CHKANDJUMP1(disp_unit <= 0, mpi_errno, MPI_ERR_ARG,
                                 "**arg", "**arg %s", "disp_unit must be positive");

            MPIR_ERR_CHKANDJUMP1(size < 0, mpi_errno, MPI_ERR_SIZE,
                                 "**rmasize", "**rmasize %d", size);

            MPIR_ERR_CHKANDJUMP1(size > 0 && baseptr == NULL, mpi_errno, MPI_ERR_ARG,
                                 "**nullptr", "**nullptr %s",
                                 "NULL base pointer is invalid when size is nonzero");

            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Win_allocate_shared(size, disp_unit, info_ptr, comm_ptr, baseptr, &win_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* Initialize a few fields that have specific defaults */
    win_ptr->name[0]    = 0;
    win_ptr->errhandler = 0;

    /* return the handle of the window object to the user */
    MPID_OBJ_PUBLISH_HANDLE(*win, win_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_ALLOCATE_SHARED);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_win_allocate_shared",
	    "**mpi_win_allocate_shared %d %I %C %p %p", size, info, comm, baseptr, win);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
