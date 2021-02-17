/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Get_version */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Get_version = PMPI_Get_version
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Get_version  MPI_Get_version
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Get_version as PMPI_Get_version
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Get_version(int *version, int *subversion)
     __attribute__ ((weak, alias("PMPI_Get_version")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Get_version
#define MPI_Get_version PMPI_Get_version

#endif

/*@
   MPI_Get_version - Return the version number of MPI

Output Parameters:
+ version - version number (integer)
- subversion - subversion number (integer)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
@*/

int MPI_Get_version(int *version, int *subversion)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_GET_VERSION);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_GET_VERSION);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(version, "version", mpi_errno);
            MPIR_ERRTEST_ARGNULL(subversion, "subversion", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    *version = MPI_VERSION;
    *subversion = MPI_SUBVERSION;
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_GET_VERSION);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_get_version", "**mpi_get_version %p %p", version,
                                     subversion);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
