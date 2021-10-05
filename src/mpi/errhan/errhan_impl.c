/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_ext.h"

int MPIR_Comm_create_errhandler_impl(MPI_Comm_errhandler_function * comm_errhandler_fn,
                                     MPIR_Errhandler ** errhandler_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errhandler *errhan_ptr;

    errhan_ptr = (MPIR_Errhandler *) MPIR_Handle_obj_alloc(&MPIR_Errhandler_mem);
    MPIR_ERR_CHKANDJUMP(!errhan_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem");

    errhan_ptr->language = MPIR_LANG__C;
    errhan_ptr->kind = MPIR_COMM;
    MPIR_Object_set_ref(errhan_ptr, 1);
    errhan_ptr->errfn.C_Comm_Handler_function = comm_errhandler_fn;

    *errhandler_ptr = errhan_ptr;
  fn_exit:
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

int MPIR_Win_create_errhandler_impl(MPI_Win_errhandler_function * win_errhandler_fn,
                                    MPIR_Errhandler ** errhandler_ptr)
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

    *errhandler_ptr = errhan_ptr;
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Session_create_errhandler_impl(MPI_Session_errhandler_function * session_errhandler_fn,
                                        MPIR_Errhandler ** errhandler_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errhandler *errhan_ptr;

    errhan_ptr = (MPIR_Errhandler *) MPIR_Handle_obj_alloc(&MPIR_Errhandler_mem);
    MPIR_ERR_CHKANDJUMP1(!errhan_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem",
                         "**nomem %s", "MPI_Errhandler");
    errhan_ptr->language = MPIR_LANG__C;
    errhan_ptr->kind = MPIR_SESSION;
    MPIR_Object_set_ref(errhan_ptr, 1);
    errhan_ptr->errfn.C_Session_Handler_function = session_errhandler_fn;

    *errhandler_ptr = errhan_ptr;
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

/* MPIR_Comm_get_errhandler_impl
   returning NULL for errhandler_ptr means the default handler, MPI_ERRORS_ARE_FATAL is used */
int MPIR_Comm_get_errhandler_impl(MPIR_Comm * comm_ptr, MPI_Errhandler * errhandler)
{
    MPID_THREAD_CS_ENTER(POBJ, comm_ptr->mutex);
    MPID_THREAD_CS_ENTER(VCI, comm_ptr->mutex);
    if (comm_ptr->errhandler) {
        *errhandler = comm_ptr->errhandler->handle;
        MPIR_Errhandler_add_ref(comm_ptr->errhandler);
    } else {
        /* Use the default */
        *errhandler = MPI_ERRORS_ARE_FATAL;
    }
    MPID_THREAD_CS_EXIT(POBJ, comm_ptr->mutex);
    MPID_THREAD_CS_EXIT(VCI, comm_ptr->mutex);

    return MPI_SUCCESS;
}

int MPIR_Win_get_errhandler_impl(MPIR_Win * win_ptr, MPI_Errhandler * errhandler)
{
    MPID_THREAD_CS_ENTER(POBJ, win_ptr->mutex);
    MPID_THREAD_CS_ENTER(VCI, win_ptr->mutex);

    if (win_ptr->errhandler) {
        *errhandler = win_ptr->errhandler->handle;
        MPIR_Errhandler_add_ref(win_ptr->errhandler);
    } else {
        /* Use the default */
        *errhandler = MPI_ERRORS_ARE_FATAL;
    }

    MPID_THREAD_CS_EXIT(POBJ, win_ptr->mutex);
    MPID_THREAD_CS_EXIT(VCI, win_ptr->mutex);
    return MPI_SUCCESS;
}

int MPIR_Session_get_errhandler_impl(MPIR_Session * session_ptr, MPI_Errhandler * errhandler)
{
    if (session_ptr->errhandler) {
        *errhandler = session_ptr->errhandler->handle;
        MPIR_Errhandler_add_ref(session_ptr->errhandler);
    } else {
        /* Use the default */
        *errhandler = MPI_ERRORS_ARE_FATAL;
    }

    return MPI_SUCCESS;
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

int MPIR_Comm_set_errhandler_impl(MPIR_Comm * comm_ptr, MPIR_Errhandler * errhandler_ptr)
{
    MPID_THREAD_CS_ENTER(POBJ, comm_ptr->mutex);
    MPID_THREAD_CS_ENTER(VCI, comm_ptr->mutex);

    /* We don't bother with the case where the errhandler is NULL;
     * in this case, the error handler was the original, MPI_ERRORS_ARE_FATAL,
     * which is builtin and can never be freed. */
    if (comm_ptr->errhandler != NULL) {
        MPIR_Errhandler_free_impl(comm_ptr->errhandler);
    }

    MPIR_Errhandler_add_ref(errhandler_ptr);
    comm_ptr->errhandler = errhandler_ptr;

    MPID_THREAD_CS_EXIT(POBJ, comm_ptr->mutex);
    MPID_THREAD_CS_EXIT(VCI, comm_ptr->mutex);
    return MPI_SUCCESS;
}

int MPIR_Win_set_errhandler_impl(MPIR_Win * win_ptr, MPIR_Errhandler * errhandler_ptr)
{
    MPID_THREAD_CS_ENTER(POBJ, win_ptr->mutex);
    MPID_THREAD_CS_ENTER(VCI, win_ptr->mutex);

    /* We don't bother with the case where the errhandler is NULL;
     * in this case, the error handler was the original, MPI_ERRORS_ARE_FATAL,
     * which is builtin and can never be freed. */
    if (win_ptr->errhandler != NULL) {
        MPIR_Errhandler_free_impl(win_ptr->errhandler);
    }

    MPIR_Errhandler_add_ref(errhandler_ptr);
    win_ptr->errhandler = errhandler_ptr;

    MPID_THREAD_CS_EXIT(POBJ, win_ptr->mutex);
    MPID_THREAD_CS_EXIT(VCI, win_ptr->mutex);
    return MPI_SUCCESS;
}

int MPIR_Session_set_errhandler_impl(MPIR_Session * session_ptr, MPIR_Errhandler * errhandler_ptr)
{
    /* We don't bother with the case where the errhandler is NULL;
     * in this case, the error handler was the original, MPI_ERRORS_ARE_FATAL,
     * which is builtin and can never be freed. */
    if (session_ptr->errhandler != NULL) {
        MPIR_Errhandler_free_impl(session_ptr->errhandler);
    }

    MPIR_Errhandler_add_ref(errhandler_ptr);
    session_ptr->errhandler = errhandler_ptr;

    return MPI_SUCCESS;
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

static int call_errhandler(MPIR_Errhandler * errhandler, int errorcode, int handle)
{
    int mpi_errno = MPI_SUCCESS;

    int kind = HANDLE_GET_MPI_KIND(handle);
    int cxx_kind = 0;

    /* Check for predefined error handlers */
    if (!errhandler || errhandler->handle == MPI_ERRORS_ARE_FATAL ||
        errhandler->handle == MPI_ERRORS_ABORT) {
        const char *fcname = NULL;
        if (kind == MPIR_COMM) {
            fcname = "MPI_Comm_call_errhandler";
        } else if (kind == MPIR_WIN) {
            fcname = "MPI_Win_call_errhandler";
        } else if (kind == MPIR_SESSION) {
            fcname = "MPI_Session_call_errhandler";
        }
        MPIR_Handle_fatal_error(NULL, fcname, errorcode);
        /* not expected to return */
        goto fn_exit;
    } else if (errhandler->handle == MPI_ERRORS_RETURN) {
        /* MPI_ERRORS_RETURN should always return MPI_SUCCESS */
        goto fn_exit;
    }

    /* Check for the special case of errors-throw-exception.  In this case
     * return the error code; the C++ wrapper will cause an exception to
     * be thrown.
     */
#ifdef HAVE_CXX_BINDING
    if (errhandler->handle == MPIR_ERRORS_THROW_EXCEPTIONS) {
        mpi_errno = errorcode;
        goto fn_exit;
    }
#endif

    /* Process any user-defined error handling function */
    switch (errhandler->language) {
        case MPIR_LANG__C:
            (*errhandler->errfn.C_Comm_Handler_function) (&handle, &errorcode);
            break;
#ifdef HAVE_CXX_BINDING
        case MPIR_LANG__CXX:
            if (kind == MPIR_COMM) {
                cxx_kind = 0;
            } else if (kind == MPIR_WIN) {
                cxx_kind = 2;
            } else {
                MPIR_Assert_error("kind not supported");
            }
            MPIR_Process.cxx_call_errfn(cxx_kind, &handle, &errorcode,
                                        (void (*)(void)) errhandler->errfn.C_Comm_Handler_function);
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
                MPI_Fint commhandle = handle;
                (*errhandler->errfn.F77_Handler_function) (&commhandle, &ferr);
            }
            break;
#endif
    }

  fn_exit:
    return mpi_errno;
}

int MPIR_Comm_call_errhandler_impl(MPIR_Comm * comm_ptr, int errorcode)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(POBJ, comm_ptr->mutex);
    MPID_THREAD_CS_ENTER(VCI, comm_ptr->mutex);

    mpi_errno = call_errhandler(comm_ptr->errhandler, errorcode, comm_ptr->handle);

    MPID_THREAD_CS_EXIT(POBJ, comm_ptr->mutex);
    MPID_THREAD_CS_EXIT(VCI, comm_ptr->mutex);
    return mpi_errno;
}

int MPIR_Win_call_errhandler_impl(MPIR_Win * win_ptr, int errorcode)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(POBJ, win_ptr->mutex);
    MPID_THREAD_CS_ENTER(VCI, win_ptr->mutex);

    mpi_errno = call_errhandler(win_ptr->errhandler, errorcode, win_ptr->handle);

    MPID_THREAD_CS_EXIT(POBJ, win_ptr->mutex);
    MPID_THREAD_CS_EXIT(VCI, win_ptr->mutex);
    return mpi_errno;
}

int MPIR_Session_call_errhandler_impl(MPIR_Session * session_ptr, int errorcode)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = call_errhandler(session_ptr->errhandler, errorcode, session_ptr->handle);

    return mpi_errno;
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

int MPIR_Errhandler_free_impl(MPIR_Errhandler * errhan_ptr)
{
    int in_use;
    MPIR_Errhandler_release_ref(errhan_ptr, &in_use);
    if (!in_use) {
        MPIR_Handle_obj_free(&MPIR_Errhandler_mem, errhan_ptr);
    }
    return MPI_SUCCESS;
}
