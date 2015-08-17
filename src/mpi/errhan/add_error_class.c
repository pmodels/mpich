/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "errcodes.h"

/* -- Begin Profiling Symbol Block for routine MPI_Add_error_class */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Add_error_class = PMPI_Add_error_class
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Add_error_class  MPI_Add_error_class
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Add_error_class as PMPI_Add_error_class
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Add_error_class(int *errorclass) __attribute__((weak,alias("PMPI_Add_error_class")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Add_error_class
#define MPI_Add_error_class PMPI_Add_error_class

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Add_error_class

/*@
   MPI_Add_error_class - Add an MPI error class to the known classes

Output Parameters:
.  errorclass - New error class

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
@*/
int MPI_Add_error_class(int *errorclass)
{
    static const char FCNAME[] = "MPI_Add_error_class";
    int mpi_errno = MPI_SUCCESS;
    int new_class;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ADD_ERROR_CLASS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ADD_ERROR_CLASS);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(errorclass, "errorclass", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* ... body of routine ...  */
    
    new_class = MPIR_Err_add_class( );
    MPIR_ERR_CHKANDJUMP(new_class<0,mpi_errno,MPI_ERR_OTHER,"**noerrclasses");

    *errorclass = ERROR_DYN_MASK | new_class;

    /* FIXME why isn't this done in MPIR_Err_add_class? */
    if (*errorclass > MPIR_Process.attrs.lastusedcode) {
       MPIR_Process.attrs.lastusedcode = *errorclass;
    }

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ADD_ERROR_CLASS);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_add_error_class",
	    "**mpi_add_error_class %p", errorclass);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

