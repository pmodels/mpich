/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Get_library_version */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Get_library_version = PMPI_Get_library_version
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Get_library_version  MPI_Get_library_version
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Get_library_version as PMPI_Get_library_version
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Get_library_version(char *version, int *resultlen) __attribute__((weak,alias("PMPI_Get_library_version")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Get_library_version
#define MPI_Get_library_version PMPI_Get_library_version

#endif

/*@
   MPI_Get_library_version - Return the version number of MPI library

Output Parameters:
+  version - Version of MPI
-  resultlen - Length of the MPI library version string

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
#undef FUNCNAME
#define FUNCNAME MPI_Get_library_version
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPI_Get_library_version(char *version, int *resultlen)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GET_LIBRARY_VERSION);

    /* Note that this routine may be called before MPI_Init */

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GET_LIBRARY_VERSION);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(version, "version", mpi_errno);
            MPIR_ERRTEST_ARGNULL(resultlen, "resultlen", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    MPL_snprintf(version, MPI_MAX_LIBRARY_VERSION_STRING,
                 "MPICH Version:\t%s\n"
                 "MPICH Release date:\t%s\n"
                 "MPICH Device:\t%s\n"
                 "MPICH configure:\t%s\n"
                 "MPICH CC:\t%s\n"
                 "MPICH CXX:\t%s\n"
                 "MPICH F77:\t%s\n"
                 "MPICH FC:\t%s\n",
                 MPIR_Version_string, MPIR_Version_date, MPIR_Version_device,
                 MPIR_Version_configure, MPIR_Version_CC, MPIR_Version_CXX,
                 MPIR_Version_F77, MPIR_Version_FC);

    *resultlen = (int)strlen(version);

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_LIBRARY_VERSION);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                 MPI_ERR_OTHER, "**mpi_get_library_version",
                                 "**mpi_get_library_version %p %p", version, resultlen);
    }
    mpi_errno = MPIR_Err_return_comm(0, FCNAME, mpi_errno);
    goto fn_exit;
#endif
    /* --END ERROR HANDLING-- */
}
