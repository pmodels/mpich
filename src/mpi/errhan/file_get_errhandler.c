/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpir_ext.h"

/* -- Begin Profiling Symbol Block for routine MPI_File_get_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_get_errhandler = PMPI_File_get_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_get_errhandler  MPI_File_get_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_get_errhandler as PMPI_File_get_errhandler
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_get_errhandler(MPI_File file, MPI_Errhandler * errhandler)
    __attribute__ ((weak, alias("PMPI_File_get_errhandler")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_File_get_errhandler
#define MPI_File_get_errhandler PMPI_File_get_errhandler

#endif

#undef FUNCNAME
#define FUNCNAME MPI_File_get_errhandler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_File_get_errhandler - Get the error handler attached to a file

Input Parameters:
. file - MPI file (handle)

Output Parameters:
. errhandler - handler currently associated with file (handle)

.N ThreadSafeNoUpdate

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_File_get_errhandler(MPI_File file, MPI_Errhandler * errhandler)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef MPI_MODE_RDONLY
    MPI_Errhandler eh;
    MPIR_Errhandler *e;
#endif
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_FILE_GET_ERRHANDLER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_FILE_GET_ERRHANDLER);

#ifdef MPI_MODE_RDONLY
    /* Validate parameters, especially handles needing to be converted */
    /* FIXME: check for a valid file handle (fh) before converting to a pointer */

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(errhandler, "errhandler", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    MPIR_ROMIO_Get_file_errhand(file, &eh);
    if (!eh) {
        MPIR_Errhandler_get_ptr(MPI_ERRORS_RETURN, e);
    } else {
        MPIR_Errhandler_get_ptr(eh, e);
    }
    MPIR_Errhandler_add_ref(e);
    *errhandler = e->handle;

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
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_FILE_GET_ERRHANDLER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_file_get_errhandler", "**mpi_file_get_errhandler %F %p",
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
