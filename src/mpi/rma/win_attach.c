/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "rma.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Win_attach */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Win_attach = PMPIX_Win_attach
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Win_attach  MPIX_Win_attach
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Win_attach as PMPIX_Win_attach
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Win_attach
#define MPIX_Win_attach PMPIX_Win_attach

#endif

#undef FUNCNAME
#define FUNCNAME MPIX_Win_attach

/*@
   MPIX_Win_attach - Attach memory to a dynamic window

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
@*/
int MPIX_Win_attach(MPI_Win win, void *base, MPI_Aint size)
{
    static const char FCNAME[] = "MPIX_Win_attach";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_WIN_ATTACH);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPIX_WIN_ATTACH);

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

    mpi_errno = MPIU_RMA_CALL(win_ptr,
                              Win_attach(win_ptr, base, size));
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPIX_WIN_ATTACH);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpix_win_attach", "**mpix_win_attach %W %p %d",
            size, base, win);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

