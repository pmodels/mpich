/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_ext.h"

int MPIR_File_create_errhandler_impl(MPI_File_errhandler_function * file_errhandler_fn,
                                     MPIR_Errhandler ** errhandler_ptr)
{
#ifdef MPI_MODE_RDONLY
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
#ifdef MPI_MODE_RDONLY
    MPI_Errhandler eh;
    MPIR_Errhandler *e;

    MPIR_ROMIO_Get_file_errhand(file, &eh);
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
#ifdef MPI_MODE_RDONLY
    MPIR_Errhandler *old_errhandler_ptr;
    MPI_Errhandler old_errhandler;

    MPIR_ROMIO_Get_file_errhand(file, &old_errhandler);
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
    MPIR_ROMIO_Set_file_errhand(file, errhan_ptr->handle);
    return MPI_SUCCESS;
#else
    return MPI_ERR_INTERN;
#endif
}

int MPIR_File_call_errhandler_impl(MPI_File fh, int errorcode)
{
#ifdef MPI_MODE_RDONLY
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errhandler *e;
    MPI_Errhandler eh;

    MPIR_ROMIO_Get_file_errhand(fh, &eh);
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

    switch (e->language) {
        case MPIR_LANG__C:
            (*e->errfn.C_File_Handler_function) (&fh, &errorcode);
            break;
#ifdef HAVE_CXX_BINDING
        case MPIR_LANG__CXX:
            /* See HAVE_LANGUAGE_FORTRAN below for an explanation */
            {
                void *fh1 = (void *) &fh;
                (*MPIR_Process.cxx_call_errfn) (1, fh1, &errorcode,
                                                (void (*)(void)) *e->errfn.C_File_Handler_function);
            }
            break;
#endif
#ifdef HAVE_FORTRAN_BINDING
        case MPIR_LANG__FORTRAN90:
        case MPIR_LANG__FORTRAN:
            /* The assignemt to a local variable prevents the compiler
             * from generating a warning about a type-punned pointer.  Since
             * the value is really const (but MPI didn't define error handlers
             * with const), this preserves the intent */
            {
                void *fh1 = (void *) &fh;
                MPI_Fint ferr = errorcode;      /* Needed if MPI_Fint and int aren't
                                                 * the same size */
                (*e->errfn.F77_Handler_function) (fh1, &ferr);
            }
            break;
#endif
    }

  fn_exit:
    return mpi_errno;
#else /* MPI_MODE_RDONLY */
    /* Dummy in case ROMIO is not defined */
    return MPI_ERR_INTERN;
#endif /* MPI_MODE_RDONLY */
}

/* Export this routine only once (if we need to compile this file twice
   to get the PMPI and MPI versions without weak symbols */
void MPIR_Get_file_error_routine(MPI_Errhandler e, void (**c) (MPI_File *, int *, ...), int *kind)
{
    MPIR_Errhandler *e_ptr = 0;
    int mpi_errno = MPI_SUCCESS;

    /* Convert the MPI_Errhandler into an MPIR_Errhandler */

    if (!e) {
        *c = 0;
        *kind = 1;      /* Use errors return as the default */
    } else {
        MPIR_ERRTEST_ERRHANDLER(e, mpi_errno);
        MPIR_Errhandler_get_ptr(e, e_ptr);
        if (!e_ptr) {
            /* FIXME: We need an error return */
            *c = 0;
            *kind = 1;
            return;
        }
        if (e_ptr->handle == MPI_ERRORS_RETURN) {
            *c = 0;
            *kind = 1;
        } else if (e_ptr->handle == MPI_ERRORS_ARE_FATAL || e_ptr->handle == MPI_ERRORS_ABORT) {
            *c = 0;
            *kind = 0;
        } else {
            *c = e_ptr->errfn.C_File_Handler_function;
            *kind = 2;
            /* If the language is C++, we need to use a special call
             * interface.  This is MPIR_File_call_cxx_errhandler.
             * See file_call_errhandler.c */
#ifdef HAVE_CXX_BINDING
            if (e_ptr->language == MPIR_LANG__CXX)
                *kind = 3;
#endif
        }
    }
  fn_fail:
    return;
}

/* This is a glue routine that can be used by ROMIO
   (see mpi-io/glue/mpich/mpio_err.c) to properly invoke the C++
   error handler */
int MPIR_File_call_cxx_errhandler(MPI_File * fh, int *errorcode,
                                  void (*c_errhandler) (MPI_File *, int *, ...))
{
    /* ROMIO will contain a reference to this routine, so if there is
     * no C++ support, it will never be called but it must be available. */
#ifdef HAVE_CXX_BINDING
    void *fh1 = (void *) fh;
    (*MPIR_Process.cxx_call_errfn) (1, fh1, errorcode, (void (*)(void)) c_errhandler);
    /* The C++ code throws an exception if the error handler
     * returns something other than MPI_SUCCESS. There is no "return"
     * of an error code. This code mirrors that in errutil.c */
    *errorcode = MPI_SUCCESS;
#endif
    return *errorcode;
}
