/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
int MPI_Lookup_name(const char *service_name, MPI_Info info, char *port_name)
    __attribute__ ((weak, alias("PMPI_Lookup_name")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Lookup_name
#define MPI_Lookup_name PMPI_Lookup_name
#endif

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
    int mpi_errno = MPI_SUCCESS;
    MPIR_Info *info_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_LOOKUP_NAME);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_LOOKUP_NAME);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INFO_OR_NULL(info, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Info_get_ptr(info, info_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate info_ptr (only if not null) */
            if (info_ptr)
                MPIR_Info_valid_ptr(info_ptr, mpi_errno);
            /* Validate character pointers */
            MPIR_ERRTEST_ARGNULL(service_name, "service_name", mpi_errno);
            MPIR_ERRTEST_ARGNULL(port_name, "port_name", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Lookup_name_impl(service_name, info_ptr, port_name);
    if (mpi_errno) {
        goto fn_fail;
    }

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_LOOKUP_NAME);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_lookup_name", "**mpi_lookup_name %s %I", service_name,
                                 info);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
