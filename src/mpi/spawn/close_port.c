/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Close_port */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Close_port = PMPI_Close_port
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Close_port  MPI_Close_port
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Close_port as PMPI_Close_port
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Close_port(const char *port_name) __attribute__((weak,alias("PMPI_Close_port")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Close_port
#define MPI_Close_port PMPI_Close_port

int MPIR_Close_port_impl(const char *port_name)
{
    return MPID_Close_port(port_name);
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Close_port
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Close_port - close port

Input Parameters:
.  port_name - a port name (string)

.N NotThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Close_port(const char *port_name)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_CLOSE_PORT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_CLOSE_PORT);

    /* ... body of routine ...  */
    
    mpi_errno = MPIR_Close_port_impl(port_name);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CLOSE_PORT);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_close_port",
	    "**mpi_close_port %s", port_name);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
