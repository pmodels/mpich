/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpir_session.h"

/* Global variables */
#ifndef MPIR_SESSION_PREALLOC
#define MPIR_SESSION_PREALLOC 2
#endif

/* Preallocated session object */
MPIR_Session MPIR_Session_direct[MPIR_SESSION_PREALLOC];

MPIR_Object_alloc_t MPIR_Session_mem = { 0, 0, 0, 0, 0, 0, 0,
    MPIR_SESSION, sizeof(MPIR_Session),
    MPIR_Session_direct, MPIR_SESSION_PREALLOC,
    {0}
};

int MPIR_Session_create(MPIR_Session ** p_session_ptr, int thread_level)
{
    int mpi_errno = MPI_SUCCESS;

    *p_session_ptr = (MPIR_Session *) MPIR_Handle_obj_alloc(&MPIR_Session_mem);
    /* --BEGIN ERROR HANDLING-- */
    MPIR_ERR_CHKHANDLEMEM(*p_session_ptr);
    /* --END ERROR HANDLING-- */
    MPIR_Object_set_ref(*p_session_ptr, 1);

    (*p_session_ptr)->errhandler = NULL;
    (*p_session_ptr)->bsendbuffer = NULL;
    /* FIXME: actually do something with session thread_level */
    (*p_session_ptr)->thread_level = thread_level;
    /* disable strict finalize feature by default */
    (*p_session_ptr)->strict_finalize = false;
    (*p_session_ptr)->memory_alloc_kinds = NULL;

    {
        int thr_err;
        MPID_Thread_mutex_create(&(*p_session_ptr)->mutex, &thr_err);
        MPIR_Assert(thr_err == 0);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Session_release(MPIR_Session * session_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int inuse;

    MPIR_Session_release_ref(session_ptr, &inuse);
    if (!inuse) {
        /* Only if refcount is 0 do we actually free. */

        /* Handle any clean up on session */
        mpi_errno = MPIR_Session_bsend_finalize(session_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        int thr_err;
        MPID_Thread_mutex_destroy(&session_ptr->mutex, &thr_err);
        MPIR_Assert(thr_err == 0);

        /* Free the error handler */
        if (session_ptr->errhandler) {
            MPIR_Errhandler_free_impl(session_ptr->errhandler);
        }

        MPL_free(session_ptr->memory_alloc_kinds);

        MPIR_Handle_obj_free(&MPIR_Session_mem, session_ptr);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static
int thread_level_to_int(char *value_str, int *value_i)
{
    int mpi_errno = MPI_SUCCESS;

    if (value_str == NULL || value_i == NULL) {
        mpi_errno = MPI_ERR_OTHER;
        goto fn_fail;
    }

    if (strcmp(value_str, "MPI_THREAD_MULTIPLE") == 0) {
        *value_i = MPI_THREAD_MULTIPLE;
    } else if (strcmp(value_str, "MPI_THREAD_SINGLE") == 0) {
        *value_i = MPI_THREAD_SINGLE;
    } else if (strcmp(value_str, "MPI_THREAD_FUNNELED") == 0) {
        *value_i = MPI_THREAD_FUNNELED;
    } else if (strcmp(value_str, "MPI_THREAD_SERIALIZED") == 0) {
        *value_i = MPI_THREAD_SERIALIZED;
    } else {
        mpi_errno = MPI_ERR_OTHER;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Session_get_thread_level_from_info(MPIR_Info * info_ptr, int *threadlevel)
{
    int mpi_errno = MPI_SUCCESS;
    int buflen = 0;
    char *thread_level_s = NULL;
    int flag = 0;
    const char key[] = "thread_level";

    /* No info pointer, nothing todo here */
    if (info_ptr == NULL) {
        goto fn_exit;
    }

    /* Get the length of the thread level value */
    mpi_errno = MPIR_Info_get_valuelen_impl(info_ptr, key, &buflen, &flag);
    MPIR_ERR_CHECK(mpi_errno);

    if (!flag) {
        /* Key thread_level not found in info object */
        goto fn_exit;
    }

    /* Get thread level value. No need to check flag afterwards
     * because we would not be here if thread_level key was not in info object.
     */
    thread_level_s = MPL_malloc(buflen + 1, MPL_MEM_BUFFER);
    mpi_errno = MPIR_Info_get_impl(info_ptr, key, buflen, thread_level_s, &flag);
    MPIR_ERR_CHECK(mpi_errno);

    /* Set requested thread level value as output */
    mpi_errno = thread_level_to_int(thread_level_s, threadlevel);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (thread_level_s) {
        MPL_free(thread_level_s);
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Session_get_strict_finalize_from_info(MPIR_Info * info_ptr, bool * strict_finalize)
{
    int mpi_errno = MPI_SUCCESS;
    int buflen = 0;
    char *strict_finalize_s = NULL;
    int flag = 0;
    const char key[] = "strict_finalize";

    /* default is no strict finalize */
    *strict_finalize = false;

    /* No info pointer, nothing todo here */
    if (info_ptr == NULL) {
        goto fn_exit;
    }

    /* Get the length of the strict finalize value */
    mpi_errno = MPIR_Info_get_valuelen_impl(info_ptr, key, &buflen, &flag);
    MPIR_ERR_CHECK(mpi_errno);

    if (!flag) {
        /* Key strict_finalize not found in info object */
        goto fn_exit;
    }

    /* Get strict finalize value. No need to check flag afterwards
     * because we would not be here if strict_finalize key was not in info object.
     */
    strict_finalize_s = MPL_malloc(buflen + 1, MPL_MEM_BUFFER);
    mpi_errno = MPIR_Info_get_impl(info_ptr, key, buflen, strict_finalize_s, &flag);
    MPIR_ERR_CHECK(mpi_errno);

    /* Set strict finalize value as output. */
    if ((strcmp(strict_finalize_s, "1") == 0) || (strcmp(strict_finalize_s, "true") == 0)) {
        *strict_finalize = true;
    }

  fn_exit:
    if (strict_finalize_s) {
        MPL_free(strict_finalize_s);
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Session_get_memory_kinds_from_info(MPIR_Info * info_ptr, char **out_kinds)
{
    int mpi_errno = MPI_SUCCESS;
    const char key[] = "mpi_memory_alloc_kinds";
    char *user_kinds = NULL;

    *out_kinds = NULL;

    if (info_ptr) {
        int buflen = 0;
        int flag;

        mpi_errno = MPIR_Info_get_string_impl(info_ptr, key, &buflen, NULL, &flag);
        MPIR_ERR_CHECK(mpi_errno);

        if (flag) {
            user_kinds = MPL_malloc(buflen, MPL_MEM_OTHER);
            mpi_errno = MPIR_Info_get_string_impl(info_ptr, key, &buflen, user_kinds, &flag);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    if (user_kinds) {
        mpi_errno = MPIR_get_supported_memory_kinds(user_kinds, out_kinds);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_fail:
    MPL_free(user_kinds);
    return mpi_errno;
}
