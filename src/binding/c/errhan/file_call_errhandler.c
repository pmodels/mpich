/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_File_call_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_call_errhandler = PMPI_File_call_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_call_errhandler  MPI_File_call_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_call_errhandler as PMPI_File_call_errhandler
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_call_errhandler(MPI_File fh, int errorcode)
     __attribute__ ((weak, alias("PMPI_File_call_errhandler")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_File_call_errhandler
#define MPI_File_call_errhandler PMPI_File_call_errhandler

#endif

/*@
   MPI_File_call_errhandler - Call the error handler installed on a file

Input Parameters:
+ fh - file with error handler (handle)
- errorcode - error code (integer)

.N ThreadSafeNoUpdate

.N Fortran

.N Errors
.N MPI_SUCCESS

@*/

int MPI_File_call_errhandler(MPI_File fh, int errorcode)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);

    /* ... body of routine ... */
    mpi_errno = MPIR_File_call_errhandler_impl(fh, errorcode);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_file_call_errhandler", "**mpi_file_call_errhandler %F %d",
                                     fh, errorcode);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
