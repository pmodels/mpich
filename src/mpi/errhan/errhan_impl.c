/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_ext.h"

int MPIR_Comm_create_errhandler_impl(MPI_Comm_errhandler_function * comm_errhandler_fn,
                                     MPI_Errhandler * errhandler)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errhandler *errhan_ptr;

    errhan_ptr = (MPIR_Errhandler *) MPIR_Handle_obj_alloc(&MPIR_Errhandler_mem);
    MPIR_ERR_CHKANDJUMP(!errhan_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem");

    errhan_ptr->language = MPIR_LANG__C;
    errhan_ptr->kind = MPIR_COMM;
    MPIR_Object_set_ref(errhan_ptr, 1);
    errhan_ptr->errfn.C_Comm_Handler_function = comm_errhandler_fn;

    MPIR_OBJ_PUBLISH_HANDLE(*errhandler, errhan_ptr->handle);
  fn_exit:
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

int MPIR_Win_create_errhandler_impl(MPI_Win_errhandler_function * win_errhandler_fn,
                                    MPI_Errhandler * errhandler)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errhandler *errhan_ptr;

    errhan_ptr = (MPIR_Errhandler *) MPIR_Handle_obj_alloc(&MPIR_Errhandler_mem);
    MPIR_ERR_CHKANDJUMP1(!errhan_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem",
                         "**nomem %s", "MPI_Errhandler");
    errhan_ptr->language = MPIR_LANG__C;
    errhan_ptr->kind = MPIR_WIN;
    MPIR_Object_set_ref(errhan_ptr, 1);
    errhan_ptr->errfn.C_Win_Handler_function = win_errhandler_fn;

    MPIR_OBJ_PUBLISH_HANDLE(*errhandler, errhan_ptr->handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#ifdef MPI_MODE_RDONLY
int MPIR_File_create_errhandler_impl(MPI_File_errhandler_function * file_errhandler_fn,
                                     MPI_Errhandler * errhandler)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errhandler *errhan_ptr;

    errhan_ptr = (MPIR_Errhandler *) MPIR_Handle_obj_alloc(&MPIR_Errhandler_mem);
    MPIR_ERR_CHKANDJUMP(!errhan_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem");
    errhan_ptr->language = MPIR_LANG__C;
    errhan_ptr->kind = MPIR_FILE;
    MPIR_Object_set_ref(errhan_ptr, 1);
    errhan_ptr->errfn.C_File_Handler_function = file_errhandler_fn;

    MPIR_OBJ_PUBLISH_HANDLE(*errhandler, errhan_ptr->handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif

/* MPIR_Comm_get_errhandler_impl
   returning NULL for errhandler_ptr means the default handler, MPI_ERRORS_ARE_FATAL is used */
void MPIR_Comm_get_errhandler_impl(MPIR_Comm * comm_ptr, MPI_Errhandler * errhandler)
{
    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));
    if (comm_ptr->errhandler) {
        *errhandler = comm_ptr->errhandler->handle;
        MPIR_Errhandler_add_ref(comm_ptr->errhandler);
    } else {
        /* Use the default */
        *errhandler = MPI_ERRORS_ARE_FATAL;
    }
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));

    return;
}

void MPIR_Win_get_errhandler_impl(MPIR_Win * win_ptr, MPI_Errhandler * errhandler)
{
    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_WIN_MUTEX(win_ptr));

    if (win_ptr->errhandler) {
        *errhandler = win_ptr->errhandler->handle;
        MPIR_Errhandler_add_ref(win_ptr->errhandler);
    } else {
        /* Use the default */
        *errhandler = MPI_ERRORS_ARE_FATAL;
    }

    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_WIN_MUTEX(win_ptr));
    return;
}

#ifdef MPI_MODE_RDONLY
/* File is handled differently from Comm/Win due to ROMIO abstraction */
void MPIR_File_get_errhandler_impl(MPI_File file, MPI_Errhandler * errhandler)
{
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

    return;
}
#endif

void MPIR_Comm_set_errhandler_impl(MPIR_Comm * comm_ptr, MPIR_Errhandler * errhandler_ptr)
{
    int in_use;

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));

    /* We don't bother with the case where the errhandler is NULL;
     * in this case, the error handler was the original, MPI_ERRORS_ARE_FATAL,
     * which is builtin and can never be freed. */
    if (comm_ptr->errhandler != NULL) {
        MPIR_Errhandler_release_ref(comm_ptr->errhandler, &in_use);
        if (!in_use) {
            MPIR_Errhandler_free(comm_ptr->errhandler);
        }
    }

    MPIR_Errhandler_add_ref(errhandler_ptr);
    comm_ptr->errhandler = errhandler_ptr;

    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));
    return;
}

void MPIR_Win_set_errhandler_impl(MPIR_Win * win_ptr, MPIR_Errhandler * errhandler_ptr)
{
    int in_use;

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_WIN_MUTEX(win_ptr));

    /* We don't bother with the case where the errhandler is NULL;
     * in this case, the error handler was the original, MPI_ERRORS_ARE_FATAL,
     * which is builtin and can never be freed. */
    if (win_ptr->errhandler != NULL) {
        MPIR_Errhandler_release_ref(win_ptr->errhandler, &in_use);
        if (!in_use) {
            MPIR_Errhandler_free(win_ptr->errhandler);
        }
    }

    MPIR_Errhandler_add_ref(errhandler_ptr);
    win_ptr->errhandler = errhandler_ptr;

    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_WIN_MUTEX(win_ptr));
    return;
}

#ifdef MPI_MODE_RDONLY
void MPIR_File_set_errhandler_impl(MPI_File file, MPIR_Errhandler * errhan_ptr)
{
    int in_use;
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
        MPIR_Errhandler_release_ref(old_errhandler_ptr, &in_use);
        if (!in_use) {
            MPIR_Errhandler_free(old_errhandler_ptr);
        }
    }

    MPIR_Errhandler_add_ref(errhan_ptr);
    MPIR_ROMIO_Set_file_errhand(file, errhan_ptr->handle);
}
#endif

int MPIR_Comm_call_errhandler_impl(MPIR_Comm * comm_ptr, int errorcode)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));  /* protect access to comm_ptr->errhandler */

    /* Check for predefined error handlers */
    if (!comm_ptr->errhandler || comm_ptr->errhandler->handle == MPI_ERRORS_ARE_FATAL) {
        mpi_errno = MPIR_Err_return_comm(comm_ptr, "MPI_Comm_call_errhandler", errorcode);
        goto fn_exit;
    }

    if (comm_ptr->errhandler->handle == MPI_ERRORS_RETURN) {
        /* MPI_ERRORS_RETURN should always return MPI_SUCCESS */
        goto fn_exit;
    }

    /* Check for the special case of errors-throw-exception.  In this case
     * return the error code; the C++ wrapper will cause an exception to
     * be thrown.
     */
#ifdef HAVE_CXX_BINDING
    if (comm_ptr->errhandler->handle == MPIR_ERRORS_THROW_EXCEPTIONS) {
        mpi_errno = errorcode;
        goto fn_exit;
    }
#endif

    /* Process any user-defined error handling function */
    switch (comm_ptr->errhandler->language) {
        case MPIR_LANG__C:
            (*comm_ptr->errhandler->errfn.C_Comm_Handler_function) (&comm_ptr->handle, &errorcode);
            break;
#ifdef HAVE_CXX_BINDING
        case MPIR_LANG__CXX:
            MPIR_Process.cxx_call_errfn(0, &comm_ptr->handle,
                                        &errorcode,
                                        (void (*)(void)) comm_ptr->errhandler->
                                        errfn.C_Comm_Handler_function);
            break;
#endif
#ifdef HAVE_FORTRAN_BINDING
        case MPIR_LANG__FORTRAN90:
        case MPIR_LANG__FORTRAN:
            {
                /* If int and MPI_Fint aren't the same size, we need to
                 * convert.  As this is not performance critical, we
                 * do this even if MPI_Fint and int are the same size. */
                MPI_Fint ferr = errorcode;
                MPI_Fint commhandle = comm_ptr->handle;
                (*comm_ptr->errhandler->errfn.F77_Handler_function) (&commhandle, &ferr);
            }
            break;
#endif
    }
  fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));
    return mpi_errno;
}

int MPIR_Win_call_errhandler_impl(MPIR_Win * win_ptr, int errorcode)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_WIN_MUTEX(win_ptr));    /* protect access to win_ptr->errhandler */

    if (!win_ptr->errhandler || win_ptr->errhandler->handle == MPI_ERRORS_ARE_FATAL) {
        mpi_errno = MPIR_Err_return_win(win_ptr, "MPI_Win_call_errhandler", errorcode);
        goto fn_exit;
    }

    if (win_ptr->errhandler->handle == MPI_ERRORS_RETURN) {
        /* MPI_ERRORS_RETURN should always return MPI_SUCCESS */
        goto fn_exit;
    }

    /* Check for the special case of errors-throw-exception.  In this case
     * return the error code; the C++ wrapper will cause an exception to
     * be thrown.
     */
#ifdef HAVE_CXX_BINDING
    if (win_ptr->errhandler->handle == MPIR_ERRORS_THROW_EXCEPTIONS) {
        mpi_errno = errorcode;
        goto fn_exit;
    }
#endif
    switch (win_ptr->errhandler->language) {
        case MPIR_LANG__C:
            (*win_ptr->errhandler->errfn.C_Win_Handler_function) (&win_ptr->handle, &errorcode);
            break;
#ifdef HAVE_CXX_BINDING
        case MPIR_LANG__CXX:
            MPIR_Process.cxx_call_errfn(2, &win_ptr->handle,
                                        &errorcode,
                                        (void (*)(void)) win_ptr->errhandler->
                                        errfn.C_Win_Handler_function);
            break;
#endif
#ifdef HAVE_FORTRAN_BINDING
        case MPIR_LANG__FORTRAN90:
        case MPIR_LANG__FORTRAN:
            {
                /* If int and MPI_Fint aren't the same size, we need to
                 * convert.  As this is not performance critical, we
                 * do this even if MPI_Fint and int are the same size. */
                MPI_Fint ferr = errorcode;
                MPI_Fint winhandle = win_ptr->handle;
                (*win_ptr->errhandler->errfn.F77_Handler_function) (&winhandle, &ferr);
            }
            break;
#endif
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_WIN_MUTEX(win_ptr));
    return mpi_errno;
}

#ifdef MPI_MODE_RDONLY
int MPIR_File_call_errhandler_impl(MPI_File fh, int errorcode)
{
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

    if (e->handle == MPI_ERRORS_ARE_FATAL) {
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
}
#endif

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
        } else if (e_ptr->handle == MPI_ERRORS_ARE_FATAL) {
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
     * no C++ support, it will never be called but it must be availavle. */
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
