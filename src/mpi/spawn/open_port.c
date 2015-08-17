/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Open_port */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Open_port = PMPI_Open_port
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Open_port  MPI_Open_port
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Open_port as PMPI_Open_port
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Open_port(MPI_Info info, char *port_name) __attribute__((weak,alias("PMPI_Open_port")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Open_port
#define MPI_Open_port PMPI_Open_port

int MPIR_Open_port_impl(MPID_Info *info_ptr, char *port_name)
{
    return MPID_Open_port(info_ptr, port_name);
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Open_port
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Open_port - Establish an address that can be used to establish 
   connections between groups of MPI processes

Input Parameters:
. info - implementation-specific information on how to establish a 
   port for 'MPI_Comm_accept' (handle) 

Output Parameters:
. port_name - newly established port (string) 

Notes:
MPI copies a system-supplied port name into 'port_name'. 'port_name' identifies
the newly opened port and can be used by a client to contact the server. 
The maximum size string that may be supplied by the system is 
'MPI_MAX_PORT_NAME'. 

 Reserved Info Key Values:
+ ip_port - Value contains IP port number at which to establish a port. 
- ip_address - Value contains IP address at which to establish a port.
 If the address is not a valid IP address of the host on which the
 'MPI_OPEN_PORT' call is made, the results are undefined. 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Open_port(MPI_Info info, char *port_name)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_OPEN_PORT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_OPEN_PORT);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Note that a NULL info is allowed */
	    MPIR_ERRTEST_INFO_OR_NULL(info, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Info_get_ptr( info, info_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* FIXME: If info_ptr is non-null, we should validate it */
	    MPIR_ERRTEST_ARGNULL(port_name,"port_name",mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPIR_Open_port_impl(info_ptr, port_name);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_OPEN_PORT);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_open_port",
	    "**mpi_open_port %I %p", info, port_name);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
