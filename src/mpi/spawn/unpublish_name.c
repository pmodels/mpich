/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "namepub.h"

/* -- Begin Profiling Symbol Block for routine MPI_Unpublish_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Unpublish_name = PMPI_Unpublish_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Unpublish_name  MPI_Unpublish_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Unpublish_name as PMPI_Unpublish_name
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Unpublish_name(const char *service_name, MPI_Info info, const char *port_name) __attribute__((weak,alias("PMPI_Unpublish_name")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Unpublish_name
#define MPI_Unpublish_name PMPI_Unpublish_name

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Unpublish_name

/*@
   MPI_Unpublish_name - Unpublish a service name published with 
   MPI_Publish_name

Input Parameters:
+ service_name - a service name (string) 
. info - implementation-specific information (handle) 
- port_name - a port name (string) 

.N ThreadSafeNoUpdate

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_INFO
.N MPI_ERR_ARG
.N MPI_ERR_OTHER
@*/
int MPI_Unpublish_name(const char *service_name, MPI_Info info, const char *port_name)
{
    static const char FCNAME[] = "MPI_Unpublish_name";
    int mpi_errno = MPI_SUCCESS;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_UNPUBLISH_NAME);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);  
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_UNPUBLISH_NAME);

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
	    MPIR_ERRTEST_ARGNULL( service_name, "service_name", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( port_name, "port_name", mpi_errno );
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
#   ifdef HAVE_NAMEPUB_SERVICE
    {
	/* The standard leaves explicitly undefined what happens if the code 
	   attempts to unpublish a name that is not published.  In this case, 
	   MPI_Unpublish_name could be called before a name service structure
	   is allocated. */
	if (!MPIR_Namepub)
	{
	    mpi_errno = MPID_NS_Create( info_ptr, &MPIR_Namepub );
	    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	    MPIR_Add_finalize( (int (*)(void*))MPID_NS_Free, &MPIR_Namepub, 9 );
	}

	mpi_errno = MPID_NS_Unpublish( MPIR_Namepub, info_ptr, 
				       (const char *)service_name );
	if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    }
#   else
    {
	/* No name publishing service available */
	MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nonamepub");
    }
#   endif

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_UNPUBLISH_NAME);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_unpublish_name",
	    "**mpi_unpublish_name %s %I %s", service_name, info, port_name);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
