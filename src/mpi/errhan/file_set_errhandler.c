/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_ext.h"

/* -- Begin Profiling Symbol Block for routine MPI_File_set_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_set_errhandler = PMPI_File_set_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_set_errhandler  MPI_File_set_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_set_errhandler as PMPI_File_set_errhandler
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_set_errhandler(MPI_File file, MPI_Errhandler errhandler)
    __attribute__ ((weak, alias("PMPI_File_set_errhandler")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_File_set_errhandler
#define MPI_File_set_errhandler PMPI_File_set_errhandler

#endif

/*@
   MPI_File_set_errhandler - Set the error handler for an MPI file

Input Parameters:
+ file - MPI file (handle)
- errhandler - new error handler for file (handle)

.N ThreadSafeNoUpdate

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_File_set_errhandler(MPI_File file, MPI_Errhandler errhandler)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_FILE_SET_ERRHANDLER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_FILE_SET_ERRHANDLER);

#ifdef MPI_MODE_RDONLY

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* FIXME: check for a valid file handle (fh) before converting to
             * a pointer */
            MPIR_ERRTEST_ERRHANDLER(errhandler, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    MPIR_Errhandler *errhan_ptr;
    MPIR_Errhandler_get_ptr(errhandler, errhan_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (!HANDLE_IS_BUILTIN(errhandler)) {
                MPIR_Errhandler_valid_ptr(errhan_ptr, mpi_errno);
                /* Also check for a valid errhandler kind */
                if (!mpi_errno) {
                    if (errhan_ptr->kind != MPIR_FILE) {
                        mpi_errno =
                            MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__,
                                                 __LINE__, MPI_ERR_ARG, "**errhandnotfile", NULL);
                    }
                }
            }
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    MPIR_File_set_errhandler_impl(file, errhan_ptr);
#else
    /* Dummy in case ROMIO is not defined */
    mpi_errno = MPI_ERR_INTERN;
#ifdef HAVE_ERROR_CHECKING
    if (0)
        goto fn_fail;   /* quiet compiler warning about unused label */
#endif
#endif

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_FILE_SET_ERRHANDLER);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_file_set_errhandler", "**mpi_file_set_errhandler %F %E",
                                 file, errhandler);
    }
    /* FIXME: Is this obsolete now? */
#ifdef MPI_MODE_RDONLY
    mpi_errno = MPIO_Err_return_file(file, mpi_errno);
#endif
    goto fn_exit;
#endif
    /* --END ERROR HANDLING-- */
}
