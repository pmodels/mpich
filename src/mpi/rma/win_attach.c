/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_attach */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_attach = PMPI_Win_attach
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_attach  MPI_Win_attach
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_attach as PMPI_Win_attach
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_attach(MPI_Win win, void *base, MPI_Aint size) __attribute__((weak,alias("PMPI_Win_attach")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_attach
#define MPI_Win_attach PMPI_Win_attach

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_attach

/*@
MPI_Win_attach - Attach memory to a dynamic window.


Attaches a local memory region beginning at base for remote access within the
given window. The memory region specified must not contain any part that is
already attached to the window win, that is, attaching overlapping memory
concurrently within the same window is erroneous. The argument win must be a
window that was created with 'MPI_Win_create_dynamic'. Multiple (but
non-overlapping) memory regions may be attached to the same window.

Input Parameters:
+ size - size of memory to be attached in bytes
. base - initial address of memory to be attached
- win - window object used for communication (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_COUNT
.N MPI_ERR_RANK
.N MPI_ERR_TYPE
.N MPI_ERR_WIN

.seealso: MPI_Win_create_dynamic MPI_Win_detach
@*/
int MPI_Win_attach(MPI_Win win, void *base, MPI_Aint size)
{
    static const char FCNAME[] = "MPI_Win_attach";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_ATTACH);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_ATTACH);

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
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;

            if (size < 0)
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS,
                                                  MPIR_ERR_RECOVERABLE,
                                                  FCNAME, __LINE__,
                                                  MPI_ERR_SIZE,
                                                  "**rmasize",
                                                  "**rmasize %d", size);
            if (size > 0 && base == NULL)
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
                                                  MPIR_ERR_RECOVERABLE, 
                                                  FCNAME, __LINE__, 
                                                  MPI_ERR_ARG,
                                                  "**nullptr",
                                                  "**nullptr %s",
                                                  "NULL base pointer is invalid when size is nonzero");  
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
   
    if (size == 0) goto fn_exit;

    mpi_errno = MPID_Win_attach(win_ptr, base, size);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_ATTACH);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_win_attach", "**mpi_win_attach %W %p %d",
            size, base, win);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

