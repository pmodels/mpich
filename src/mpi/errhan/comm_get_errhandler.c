/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_get_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_get_errhandler = PMPI_Comm_get_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_get_errhandler  MPI_Comm_get_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_get_errhandler as PMPI_Comm_get_errhandler
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler) __attribute__((weak,alias("PMPI_Comm_get_errhandler")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_get_errhandler
#define MPI_Comm_get_errhandler PMPI_Comm_get_errhandler

/* MPIR_Comm_get_errhandler_impl
   returning NULL for errhandler_ptr means the default handler, MPI_ERRORS_ARE_FATAL is used */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_get_errhandler_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_Comm_get_errhandler_impl(MPID_Comm *comm_ptr, MPID_Errhandler **errhandler_ptr)
{
    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));
    *errhandler_ptr = comm_ptr->errhandler;
    if (comm_ptr->errhandler)
	MPIR_Errhandler_add_ref(comm_ptr->errhandler);
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));

    return;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_get_errhandler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Comm_get_errhandler - Get the error handler attached to a communicator

Input Parameters:
. comm - communicator (handle) 

Output Parameters:
. errhandler - handler currently associated with communicator (handle) 

.N ThreadSafeNoUpdate

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
@*/
int MPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Errhandler *errhandler_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_GET_ERRHANDLER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_GET_ERRHANDLER);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr; if comm_ptr is not valid, it will be reset to null  */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, TRUE );
            if (mpi_errno) goto fn_fail;
	    MPIR_ERRTEST_ARGNULL(errhandler,"errhandler",mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    MPIR_Comm_get_errhandler_impl(comm_ptr, &errhandler_ptr);
    if (errhandler_ptr)
        *errhandler = errhandler_ptr->handle;
    else
        *errhandler = MPI_ERRORS_ARE_FATAL;
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_GET_ERRHANDLER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_comm_get_errhandler",
	    "**mpi_comm_get_errhandler %C %p", comm, errhandler);
    }
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}

