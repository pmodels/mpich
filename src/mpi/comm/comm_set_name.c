/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_set_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_set_name = PMPI_Comm_set_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_set_name  MPI_Comm_set_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_set_name as PMPI_Comm_set_name
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_set_name(MPI_Comm comm, const char *comm_name) __attribute__((weak,alias("PMPI_Comm_set_name")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_set_name
#define MPI_Comm_set_name PMPI_Comm_set_name

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_set_name
#undef FCNAME
#define FCNAME "MPI_Comm_set_name"

/*@
   MPI_Comm_set_name - Sets the print name for a communicator

Input Parameters:
+  comm - communicator to name (handle)
-  comm_name - Name for communicator

.N ThreadSafeNoUpdate

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
@*/
int MPI_Comm_set_name(MPI_Comm comm, const char *comm_name)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_SET_NAME);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_SET_NAME);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, TRUE );
            if (mpi_errno) goto fn_fail;
	    MPIR_ERRTEST_ARGNULL( comm_name, "comm_name", mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));
    MPIU_Strncpy( comm_ptr->name, comm_name, MPI_MAX_OBJECT_NAME );
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SET_NAME);
    return mpi_errno;
    
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_comm_set_name",
	    "**mpi_comm_set_name %C %s", comm, comm_name);
    }
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}

