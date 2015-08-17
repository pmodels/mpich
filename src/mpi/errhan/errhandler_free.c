/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Errhandler_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Errhandler_free = PMPI_Errhandler_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Errhandler_free  MPI_Errhandler_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Errhandler_free as PMPI_Errhandler_free
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Errhandler_free(MPI_Errhandler *errhandler) __attribute__((weak,alias("PMPI_Errhandler_free")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Errhandler_free
#define MPI_Errhandler_free PMPI_Errhandler_free

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Errhandler_free

/*@
  MPI_Errhandler_free - Frees an MPI-style errorhandler

Input Parameters:
. errhandler - MPI error handler (handle).  Set to 'MPI_ERRHANDLER_NULL' on 
exit.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Errhandler_free(MPI_Errhandler *errhandler)
{
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPI_Errhandler_free";
#endif
    int mpi_errno = MPI_SUCCESS;
    MPID_Errhandler *errhan_ptr = NULL;
    int in_use;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ERRHANDLER_FREE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ERRHANDLER_FREE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(*errhandler, "errhandler", mpi_errno);
	    MPIR_ERRTEST_ERRHANDLER(*errhandler, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Errhandler_get_ptr( *errhandler, errhan_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Errhandler_valid_ptr( errhan_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    MPIR_Errhandler_release_ref( errhan_ptr,&in_use);
    if (!in_use) {
	MPIU_Handle_obj_free( &MPID_Errhandler_mem, errhan_ptr );
    }
    *errhandler = MPI_ERRHANDLER_NULL;
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERRHANDLER_FREE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_errhandler_free",
	    "**mpi_errhandler_free %p", errhandler);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}

