/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_File_get_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_get_errhandler = PMPI_File_get_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_get_errhandler  MPI_File_get_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_get_errhandler as PMPI_File_get_errhandler
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_get_errhandler(MPI_File file, MPI_Errhandler *errhandler)
     __attribute__ ((weak, alias("PMPI_File_get_errhandler")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_File_get_errhandler
#define MPI_File_get_errhandler PMPI_File_get_errhandler

#endif

/*@
   MPI_File_get_errhandler - Get the error handler attached to a file

Input Parameters:
. file - file (handle)

Output Parameters:
. errhandler - error handler currently associated with file (handle)

.N ThreadSafeNoUpdate

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
@*/

int MPI_File_get_errhandler(MPI_File file, MPI_Errhandler *errhandler)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_FILE_GET_ERRHANDLER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_FILE_GET_ERRHANDLER);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(errhandler, "errhandler", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPIR_File_get_errhandler_impl(file, errhandler);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_FILE_GET_ERRHANDLER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_file_get_errhandler", "**mpi_file_get_errhandler %F %p",
                                     file, errhandler);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
