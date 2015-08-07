/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "namepub.h"

/* -- Begin Profiling Symbol Block for routine MPI_Lookup_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Lookup_name = PMPI_Lookup_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Lookup_name  MPI_Lookup_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Lookup_name as PMPI_Lookup_name
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Lookup_name(const char *service_name, MPI_Info info, char *port_name) __attribute__((weak,alias("PMPI_Lookup_name")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Lookup_name
#define MPI_Lookup_name PMPI_Lookup_name

/* One of these routines needs to define the global handle.  Since
   Most routines will use lookup (if they use any of the name publishing
   interface at all), we place this in MPI_Lookup_name. 
*/
MPID_NS_Handle MPIR_Namepub = 0;

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Lookup_name

/*@
   MPI_Lookup_name - Lookup a port given a service name

Input Parameters:
+ service_name - a service name (string) 
- info - implementation-specific information (handle) 


Output Parameters:
.  port_name - a port name (string) 

Notes:
If the 'service_name' is found, MPI copies the associated value into 
'port_name'.  The maximum size string that may be supplied by the system is 
'MPI_MAX_PORT_NAME'. 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_INFO
.N MPI_ERR_OTHER
.N MPI_ERR_ARG
@*/
int MPI_Lookup_name(const char *service_name, MPI_Info info, char *port_name)
{
    static const char FCNAME[] = "MPI_Lookup_name";
    int mpi_errno = MPI_SUCCESS;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_LOOKUP_NAME);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_LOOKUP_NAME);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
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
            /* Validate info_ptr (only if not null) */
	    if (info_ptr) 
		MPID_Info_valid_ptr( info_ptr, mpi_errno );
	    /* Validate character pointers */
	    MPIR_ERRTEST_ARGNULL( service_name, "service_name", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( port_name, "port_name", mpi_errno );
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
#   ifdef HAVE_NAMEPUB_SERVICE
    {
	if (!MPIR_Namepub)
	{
	    mpi_errno = MPID_NS_Create( info_ptr, &MPIR_Namepub );
	    /* FIXME: change **fail to something more meaningful */
	    MPIR_ERR_CHKANDJUMP((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**fail");
	    MPIR_Add_finalize( (int (*)(void*))MPID_NS_Free, &MPIR_Namepub, 9 );
	}

	mpi_errno = MPID_NS_Lookup( MPIR_Namepub, info_ptr,
				    (const char *)service_name, port_name );
	/* FIXME: change **fail to something more meaningful */
	/* Note: Jump on *any* error, not just errors other than MPI_ERR_NAME.
	   The usual MPI rules on errors apply - the error handler on the
	   communicator (file etc.) is invoked; MPI_COMM_WORLD is used
	   if there is no obvious communicator. A previous version of 
	   this routine erroneously did not invoke the error handler
	   when the error was of class MPI_ERR_NAME. */
	MPIR_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS,
			    mpi_errno, MPI_ERR_OTHER, "**fail");
    }
#   else
    {
	/* No name publishing service available */
	MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nonamepub");
    }
#   endif
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_LOOKUP_NAME);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_lookup_name",
	    "**mpi_lookup_name %s %I %p", service_name, info, port_name);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
