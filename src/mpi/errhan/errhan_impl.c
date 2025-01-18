/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#ifdef BUILD_MPI_ABI
#include "mpi_abi_util.h"
#endif

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

int MPIR_Comm_create_errhandler_x_impl(MPIX_Comm_errhandler_function_x * errhandler_fn_x,
                                       MPIX_Destructor_function * destructor_fn,
                                       void *extra_state, MPIR_Errhandler ** errhandler_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errhandler *errhan_ptr;

    errhan_ptr = (MPIR_Errhandler *) MPIR_Handle_obj_alloc(&MPIR_Errhandler_mem);
    MPIR_ERR_CHKANDJUMP(!errhan_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem");

    errhan_ptr->language = MPIR_LANG__X;
    errhan_ptr->kind = MPIR_COMM;
    errhan_ptr->extra_state = extra_state;
    errhan_ptr->destructor_fn = destructor_fn;
    MPIR_Object_set_ref(errhan_ptr, 1);
    errhan_ptr->errfn.X_Comm_Handler_function = errhandler_fn_x;

    *errhandler_ptr = errhan_ptr;
  fn_exit:
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

int MPIR_Win_create_errhandler_x_impl(MPIX_Win_errhandler_function_x * errhandler_fn_x,
                                      MPIX_Destructor_function * destructor_fn,
                                      void *extra_state, MPIR_Errhandler ** errhandler_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errhandler *errhan_ptr;

    errhan_ptr = (MPIR_Errhandler *) MPIR_Handle_obj_alloc(&MPIR_Errhandler_mem);
    MPIR_ERR_CHKANDJUMP1(!errhan_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem",
                         "**nomem %s", "MPI_Errhandler");
    errhan_ptr->language = MPIR_LANG__X;
    errhan_ptr->kind = MPIR_WIN;
    errhan_ptr->extra_state = extra_state;
    errhan_ptr->destructor_fn = destructor_fn;
    MPIR_Object_set_ref(errhan_ptr, 1);
    errhan_ptr->errfn.X_Win_Handler_function = errhandler_fn_x;

    *errhandler_ptr = errhan_ptr;
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_create_errhandler_x_impl(MPIX_File_errhandler_function_x * errhandler_fn_x,
                                       MPIX_Destructor_function * destructor_fn,
                                       void *extra_state, MPIR_Errhandler ** errhandler_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errhandler *errhan_ptr;

    errhan_ptr = (MPIR_Errhandler *) MPIR_Handle_obj_alloc(&MPIR_Errhandler_mem);
    MPIR_ERR_CHKANDJUMP(!errhan_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem");
    errhan_ptr->language = MPIR_LANG__X;
    errhan_ptr->kind = MPIR_FILE;
    errhan_ptr->extra_state = extra_state;
    errhan_ptr->destructor_fn = destructor_fn;
    MPIR_Object_set_ref(errhan_ptr, 1);
    errhan_ptr->errfn.X_File_Handler_function = errhandler_fn_x;

    *errhandler_ptr = errhan_ptr;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Session_create_errhandler_x_impl(MPIX_Session_errhandler_function_x * errhandler_fn_x,
                                          MPIX_Destructor_function * destructor_fn,
                                          void *extra_state, MPIR_Errhandler ** errhandler_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errhandler *errhan_ptr;

    errhan_ptr = (MPIR_Errhandler *) MPIR_Handle_obj_alloc(&MPIR_Errhandler_mem);
    MPIR_ERR_CHKANDJUMP1(!errhan_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem",
                         "**nomem %s", "MPI_Errhandler");
    errhan_ptr->language = MPIR_LANG__X;
    errhan_ptr->kind = MPIR_SESSION;
    errhan_ptr->extra_state = extra_state;
    errhan_ptr->destructor_fn = destructor_fn;
    MPIR_Object_set_ref(errhan_ptr, 1);
    errhan_ptr->errfn.X_Session_Handler_function = errhandler_fn_x;

    *errhandler_ptr = errhan_ptr;
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIR_Comm_get_errhandler_impl
   returning NULL for errhandler_ptr means the default handler, MPI_ERRORS_ARE_FATAL is used */
int MPIR_Comm_get_errhandler_impl(MPIR_Comm * comm_ptr, MPI_Errhandler * errhandler)
{
    MPID_THREAD_CS_ENTER(VCI, comm_ptr->mutex);
    if (comm_ptr->errhandler) {
        *errhandler = comm_ptr->errhandler->handle;
        MPIR_Errhandler_add_ref(comm_ptr->errhandler);
    } else {
        /* Use the default */
        *errhandler = MPI_ERRORS_ARE_FATAL;
    }
    MPID_THREAD_CS_EXIT(VCI, comm_ptr->mutex);

    return MPI_SUCCESS;
}

int MPIR_Win_get_errhandler_impl(MPIR_Win * win_ptr, MPI_Errhandler * errhandler)
{
    MPID_THREAD_CS_ENTER(VCI, win_ptr->mutex);

    if (win_ptr->errhandler) {
        *errhandler = win_ptr->errhandler->handle;
        MPIR_Errhandler_add_ref(win_ptr->errhandler);
    } else {
        /* Use the default */
        *errhandler = MPI_ERRORS_ARE_FATAL;
    }

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

int MPIR_Comm_set_errhandler_impl(MPIR_Comm * comm_ptr, MPIR_Errhandler * errhandler_ptr)
{
    MPID_THREAD_CS_ENTER(VCI, comm_ptr->mutex);

    /* We don't bother with the case where the errhandler is NULL;
     * in this case, the error handler was the original, MPI_ERRORS_ARE_FATAL,
     * which is builtin and can never be freed. */
    if (comm_ptr->errhandler != NULL) {
        MPIR_Errhandler_free_impl(comm_ptr->errhandler);
    }

    MPIR_Errhandler_add_ref(errhandler_ptr);
    comm_ptr->errhandler = errhandler_ptr;

    MPID_THREAD_CS_EXIT(VCI, comm_ptr->mutex);
    return MPI_SUCCESS;
}

int MPIR_Win_set_errhandler_impl(MPIR_Win * win_ptr, MPIR_Errhandler * errhandler_ptr)
{
    MPID_THREAD_CS_ENTER(VCI, win_ptr->mutex);

    /* We don't bother with the case where the errhandler is NULL;
     * in this case, the error handler was the original, MPI_ERRORS_ARE_FATAL,
     * which is builtin and can never be freed. */
    if (win_ptr->errhandler != NULL) {
        MPIR_Errhandler_free_impl(win_ptr->errhandler);
    }

    MPIR_Errhandler_add_ref(errhandler_ptr);
    win_ptr->errhandler = errhandler_ptr;

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

static int call_errhandler(MPIR_Comm * comm_ptr, MPIR_Errhandler * errhandler, int errorcode,
                           int handle)
{
    int mpi_errno = MPI_SUCCESS;

    int kind = HANDLE_GET_MPI_KIND(handle);

    /* Check for predefined error handlers */
    if (!errhandler || errhandler->handle == MPI_ERRORS_ARE_FATAL ||
        errhandler->handle == MPI_ERRORS_ABORT) {
        if (!errhandler || errhandler->handle == MPI_ERRORS_ARE_FATAL) {
            comm_ptr = NULL;
        }
        const char *fcname = NULL;
        if (kind == MPIR_COMM) {
            fcname = "MPI_Comm_call_errhandler";
        } else if (kind == MPIR_WIN) {
            fcname = "MPI_Win_call_errhandler";
        } else if (kind == MPIR_SESSION) {
            fcname = "MPI_Session_call_errhandler";
        }
        MPIR_Handle_fatal_error(comm_ptr, fcname, errorcode);
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

    MPIR_handle h;
    h.kind = kind;
    h.u.handle = handle;
    mpi_errno = MPIR_call_errhandler(errhandler, errorcode, h);

  fn_exit:
    return mpi_errno;
}

int MPIR_Comm_call_errhandler_impl(MPIR_Comm * comm_ptr, int errorcode)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(VCI, comm_ptr->mutex);

    mpi_errno = call_errhandler(comm_ptr, comm_ptr->errhandler, errorcode, comm_ptr->handle);

    MPID_THREAD_CS_EXIT(VCI, comm_ptr->mutex);
    return mpi_errno;
}

int MPIR_Win_call_errhandler_impl(MPIR_Win * win_ptr, int errorcode)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(VCI, win_ptr->mutex);

    mpi_errno = call_errhandler(win_ptr->comm_ptr, win_ptr->errhandler, errorcode, win_ptr->handle);

    MPID_THREAD_CS_EXIT(VCI, win_ptr->mutex);
    return mpi_errno;
}

int MPIR_Session_call_errhandler_impl(MPIR_Session * session_ptr, int errorcode)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = call_errhandler(NULL, session_ptr->errhandler, errorcode, session_ptr->handle);

    return mpi_errno;
}

int MPIR_Errhandler_free_impl(MPIR_Errhandler * errhan_ptr)
{
    int in_use;
    MPIR_Errhandler_release_ref(errhan_ptr, &in_use);
    if (!in_use) {
        if (errhan_ptr->language == MPIR_LANG__X) {
            errhan_ptr->destructor_fn(errhan_ptr->extra_state);
        }
        MPIR_Handle_obj_free(&MPIR_Errhandler_mem, errhan_ptr);
    }
    return MPI_SUCCESS;
}

int MPIR_Error_class_impl(int errorcode, int *errorclass)
{
    /* NOTE: MPIR_ERR_GET_CLASS is for internal use and only applies
     *       to non-dynamic error code */
    *errorclass = errorcode & (ERROR_CLASS_MASK);
    if ((errorcode & ERROR_DYN_MASK) && (errorcode & ERROR_DYN_CLASS)) {
        /* dynamic error code with dynamic error class */
        *errorclass = *errorclass | (ERROR_DYN_MASK | ERROR_DYN_CLASS);
    }

    return MPI_SUCCESS;
}

int MPIR_Error_string_impl(int errorcode, char *string, int *resultlen)
{
    MPIR_Err_get_string(errorcode, string, MPI_MAX_ERROR_STRING, NULL);
    *resultlen = (int) strlen(string);

    return MPI_SUCCESS;
}
