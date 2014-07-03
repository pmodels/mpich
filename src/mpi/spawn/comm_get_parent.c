/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_get_parent */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_get_parent = PMPI_Comm_get_parent
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_get_parent  MPI_Comm_get_parent
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_get_parent as PMPI_Comm_get_parent
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_get_parent(MPI_Comm *parent) __attribute__((weak,alias("PMPI_Comm_get_parent")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_get_parent
#define MPI_Comm_get_parent PMPI_Comm_get_parent

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_get_parent

/*@
   MPI_Comm_get_parent - Return the parent communicator for this process

Output Parameters:
. parent - the parent communicator (handle) 

   Notes:

 If a process was started with 'MPI_Comm_spawn' or 'MPI_Comm_spawn_multiple', 
 'MPI_Comm_get_parent' returns the parent intercommunicator of the current 
  process. This parent intercommunicator is created implicitly inside of 
 'MPI_Init' and is the same intercommunicator returned by 'MPI_Comm_spawn'
  in the parents. 

  If the process was not spawned, 'MPI_Comm_get_parent' returns 
  'MPI_COMM_NULL'.

  After the parent communicator is freed or disconnected, 'MPI_Comm_get_parent'
  returns 'MPI_COMM_NULL'. 

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Comm_get_parent(MPI_Comm *parent)
{
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPI_Comm_get_parent";
#endif
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_GET_PARENT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_GET_PARENT);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(parent, "parent", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* Note that MPIU_DBG_OpenFile also uses this code (so as to avoid
       calling an MPI routine while logging it */
    *parent = (MPIR_Process.comm_parent == NULL) ? MPI_COMM_NULL :
               (MPIR_Process.comm_parent)->handle;  

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_GET_PARENT);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_comm_get_parent", "**mpi_comm_get_parent %p", parent);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
