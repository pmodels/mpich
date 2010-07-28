/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Errhandler_set */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Errhandler_set = PMPI_Errhandler_set
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Errhandler_set  MPI_Errhandler_set
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Errhandler_set as PMPI_Errhandler_set
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Errhandler_set
#define MPI_Errhandler_set PMPI_Errhandler_set

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Errhandler_set
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/*@
  MPI_Errhandler_set - Sets the error handler for a communicator

Input Parameters:
+ comm - communicator to set the error handler for (handle) 
- errhandler - new MPI error handler for communicator (handle) 

.N ThreadSafe

.N Deprecated
The replacement for this routine is 'MPI_Comm_set_errhandler'.

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_ARG

.seealso: MPI_Comm_set_errhandler, MPI_Errhandler_create, MPI_Comm_create_errhandler
@*/
int MPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Errhandler *errhan_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ERRHANDLER_SET);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ERRHANDLER_SET);

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
            /* Validate comm_ptr; if comm_ptr is not value, it will be reset to null */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    MPIR_ERRTEST_ERRHANDLER(errhandler, mpi_errno);
            if (mpi_errno) goto fn_fail;

	    if (HANDLE_GET_KIND(errhandler) != HANDLE_KIND_BUILTIN) {
                MPID_Errhandler_valid_ptr( errhan_ptr, mpi_errno );
                if (mpi_errno) goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    MPIR_Comm_set_errhandler_impl(comm_ptr, errhan_ptr);
    
    /* ... end of body of routine ... */

#   ifdef HAVE_ERROR_CHECKING
  fn_exit:
#   endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERRHANDLER_SET);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_errhandler_set",
	    "**mpi_errhandler_set %C %E", comm, errhandler);
    }
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
#   endif
}

