/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#ifndef BUILD_MPI_ABI
#include "mpir_ext.h"
#define get_file_errhand MPIR_ROMIO_Get_file_errhand
#define set_file_errhand MPIR_ROMIO_Set_file_errhand

#else
#include "mpi_abi_util.h"
int MPIR_ROMIO_Get_file_errhand(MPI_File, ABI_Errhandler *);
int MPIR_ROMIO_Set_file_errhand(MPI_File, ABI_Errhandler);

static int get_file_errhand(MPI_File file, MPI_Errhandler * eh_ptr)
{
    ABI_Errhandler eh_abi;
    int ret = MPIR_ROMIO_Get_file_errhand(file, &eh_abi);
    if (!eh_abi) {
        *eh_ptr = 0;
    } else {
        *eh_ptr = ABI_Errhandler_to_mpi(eh_abi);
    }
}

static int set_file_errhand(MPI_File file, MPI_Errhandler eh)
{
    return MPIR_ROMIO_Set_file_errhand(file, ABI_Errhandler_from_mpi(eh));
}

int MPIR_Call_file_errhandler(ABI_Errhandler e, int errorcode, MPI_File f);
#endif

int MPIR_File_create_errhandler_impl(MPI_File_errhandler_function * file_errhandler_fn,
                                     MPIR_Errhandler ** errhandler_ptr)
{
#ifdef HAVE_ROMIO
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errhandler *errhan_ptr;

    errhan_ptr = (MPIR_Errhandler *) MPIR_Handle_obj_alloc(&MPIR_Errhandler_mem);
    MPIR_ERR_CHKANDJUMP(!errhan_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem");
    errhan_ptr->language = MPIR_LANG__C;
    errhan_ptr->kind = MPIR_FILE;
    MPIR_Object_set_ref(errhan_ptr, 1);
    errhan_ptr->errfn.C_File_Handler_function = file_errhandler_fn;

    *errhandler_ptr = errhan_ptr;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#else
    return MPI_ERR_INTERN;
#endif
}

/* File is handled differently from Comm/Win due to ROMIO abstraction */
int MPIR_File_get_errhandler_impl(MPI_File file, MPI_Errhandler * errhandler)
{
#ifdef HAVE_ROMIO
    MPI_Errhandler eh;
    MPIR_Errhandler *e;

    get_file_errhand(file, &eh);
    if (!eh) {
        MPIR_Errhandler_get_ptr(MPI_ERRORS_RETURN, e);
    } else {
        MPIR_Errhandler_get_ptr(eh, e);
    }

    MPIR_Errhandler_add_ref(e);
    *errhandler = e->handle;

    return MPI_SUCCESS;
#else
    return MPI_ERR_INTERN;
#endif
}

int MPIR_File_set_errhandler_impl(MPI_File file, MPIR_Errhandler * errhan_ptr)
{
#ifdef HAVE_ROMIO
    MPIR_Errhandler *old_errhandler_ptr;
    MPI_Errhandler old_errhandler;

    get_file_errhand(file, &old_errhandler);
    if (!old_errhandler) {
        /* MPI_File objects default to the errhandler set on MPI_FILE_NULL
         * at file open time, or MPI_ERRORS_RETURN if no errhandler is set
         * on MPI_FILE_NULL. (MPI-2.2, sec 13.7) */
        MPIR_Errhandler_get_ptr(MPI_ERRORS_RETURN, old_errhandler_ptr);
    } else {
        MPIR_Errhandler_get_ptr(old_errhandler, old_errhandler_ptr);
    }

    if (old_errhandler_ptr) {
        MPIR_Errhandler_free_impl(old_errhandler_ptr);
    }

    MPIR_Errhandler_add_ref(errhan_ptr);
    set_file_errhand(file, errhan_ptr->handle);
    return MPI_SUCCESS;
#else
    return MPI_ERR_INTERN;
#endif
}

int MPIR_File_call_errhandler_impl(MPI_File fh, int errorcode)
{
#ifdef HAVE_ROMIO
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errhandler *e;
    MPI_Errhandler eh;

    get_file_errhand(fh, &eh);
    /* Check for the special case of errors-throw-exception.  In this case
     * return the error code; the C++ wrapper will cause an exception to
     * be thrown.
     */
#ifdef HAVE_CXX_BINDING
    if (eh == MPIR_ERRORS_THROW_EXCEPTIONS) {
        mpi_errno = errorcode;
        goto fn_exit;
    }
#endif

    if (!eh) {
        MPIR_Errhandler_get_ptr(MPI_ERRORS_RETURN, e);
    } else {
        MPIR_Errhandler_get_ptr(eh, e);
    }

    /* Note that, unlike the rest of MPICH, MPI_File objects are pointers,
     * not integers.  */

    if (e->handle == MPI_ERRORS_RETURN) {
        goto fn_exit;
    }

    if (e->handle == MPI_ERRORS_ARE_FATAL || e->handle == MPI_ERRORS_ABORT) {
        MPIR_Handle_fatal_error(NULL, "MPI_File_call_errhandler", errorcode);
    }

    MPIR_handle h;
    h.kind = MPIR_FILE;
    h.u.fh = fh;
    mpi_errno = MPIR_call_errhandler(e, errorcode, h);

  fn_exit:
    return mpi_errno;
#else /* HAVE_ROMIO */
    /* Dummy in case ROMIO is not defined */
    return MPI_ERR_INTERN;
#endif /* HAVE_ROMIO */
}

static int call_file_errhandler(MPI_Errhandler e, int errorcode, MPI_File f)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Errhandler *e_ptr = NULL;
    MPIR_Errhandler_get_ptr(e, e_ptr);

    if (!e_ptr) {
        mpi_errno = errorcode;
    } else {
        MPIR_handle h;
        h.kind = MPIR_FILE;
        h.u.fh = f;
        mpi_errno = MPIR_call_errhandler(e_ptr, errorcode, h);
    }

    return mpi_errno;
}

#ifndef BUILD_MPI_ABI
int MPIR_Call_file_errhandler(MPI_Errhandler e, int errorcode, MPI_File f)
{
    return call_file_errhandler(e, errorcode, f);
}
#else
int MPIR_Call_file_errhandler(ABI_Errhandler e, int errorcode, MPI_File f)
{
    return call_file_errhandler(ABI_Errhandler_to_mpi(e), errorcode, f);
}
#endif
