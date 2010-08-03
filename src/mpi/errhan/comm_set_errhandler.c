/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_set_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_set_errhandler = PMPI_Comm_set_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_set_errhandler  MPI_Comm_set_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_set_errhandler as PMPI_Comm_set_errhandler
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_set_errhandler
#define MPI_Comm_set_errhandler PMPI_Comm_set_errhandler

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_set_errhandler_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
void MPIR_Comm_set_errhandler_impl(MPID_Comm *comm_ptr, MPID_Errhandler *errhandler_ptr)
{
    int in_use;

    MPIU_THREAD_CS_ENTER(MPI_OBJ, comm_ptr);

    /* We don't bother with the case where the errhandler is NULL;
       in this case, the error handler was the original, MPI_ERRORS_ARE_FATAL,
       which is builtin and can never be freed. */
    if (comm_ptr->errhandler != NULL) {
        MPIR_Errhandler_release_ref(comm_ptr->errhandler, &in_use);
        if (!in_use) {
            MPID_Errhandler_free( comm_ptr->errhandler );
        }
    }

    MPIR_Errhandler_add_ref(errhandler_ptr);
    comm_ptr->errhandler = errhandler_ptr;

    MPIU_THREAD_CS_EXIT(MPI_OBJ, comm_ptr);
    return;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_set_errhandler
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)

/*@
   MPI_Comm_set_errhandler - Set the error handler for a communicator

   Input Parameters:
+ comm - communicator (handle) 
- errhandler - new error handler for communicator (handle) 

.N ThreadSafeNoUpdate

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_OTHER

.seealso MPI_Comm_get_errhandler, MPI_Comm_call_errhandler
@*/
int MPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Errhandler *errhan_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_SET_ERRHANDLER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_SET_ERRHANDLER);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );
    MPID_Errhandler_get_ptr( errhandler, errhan_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr; if comm_ptr is not valid, it will be reset to null */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );

	    if (HANDLE_GET_KIND(errhandler) != HANDLE_KIND_BUILTIN) {
		MPID_Errhandler_valid_ptr( errhan_ptr, mpi_errno );
	    }
	    MPIR_ERRTEST_ERRHANDLER(errhandler, mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    MPIR_Comm_set_errhandler_impl(comm_ptr, errhan_ptr);
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SET_ERRHANDLER);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;
    
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_comm_set_errhandler",
	    "**mpi_comm_set_errhandler %C %E", comm, errhandler);
    }
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}

