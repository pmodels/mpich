/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_SUPPRESS_ABORT_MESSAGE
      category    : ERROR_HANDLING
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : Disable printing of abort error message.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* Global variables */
#ifndef MPIR_SESSION_PREALLOC
#define MPIR_SESSION_PREALLOC 2
#endif

MPIR_Session MPIR_Session_direct[MPIR_SESSION_PREALLOC];

MPIR_Object_alloc_t MPIR_Session_mem = { 0, 0, 0, 0,
    MPIR_SESSION, sizeof(MPIR_Session),
    MPIR_Session_direct, MPIR_SESSION_PREALLOC,
    NULL
};

/* Psets */
static const char *default_pset_list[] = {
    "mpi://WORLD",
    "mpi://SELF",
    NULL
};

/* TODO: move into MPIR_Session struct */
const char **MPIR_pset_list = default_pset_list;

/* Implementations */

int MPIR_Session_init_impl(MPIR_Info * info_ptr, MPIR_Errhandler * errhandler_ptr,
                           MPIR_Session ** p_session_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Session *session_ptr = NULL;

    /* Until we identify all the functions that may share states across sessions and
     * protect them approprately, we require THREAD_MULTIPLE to support sessions */

    int provided;
    mpi_errno = MPII_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided, &session_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Assert(provided == MPI_THREAD_MULTIPLE);

    session_ptr->thread_level = provided;

    *p_session_ptr = session_ptr;

  fn_exit:
    return mpi_errno;

  fn_fail:
    if (session_ptr) {
        MPIR_Handle_obj_free(&MPIR_Session_mem, session_ptr);
    }
    goto fn_exit;
}

int MPIR_Session_finalize_impl(MPIR_Session * session_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    /* MPII_Finalize will free the session_ptr */
    mpi_errno = MPII_Finalize(session_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Session_get_num_psets_impl(MPIR_Session * session_ptr, MPIR_Info * info_ptr,
                                    int *npset_names)
{
    int i = 0;
    while (MPIR_pset_list[i]) {
        i++;
    }
    *npset_names = i;
    return MPI_SUCCESS;
}

int MPIR_Session_get_nth_pset_impl(MPIR_Session * session_ptr, MPIR_Info * info_ptr,
                                   int n, int *pset_len, char *pset_name)
{
    int mpi_errno = MPI_SUCCESS;

    int i = 0;
    while (MPIR_pset_list[i] && i < n) {
        i++;
    }

    if (i != n) {
        MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_ARG, goto fn_fail, "**psetinvalidn");
    }

    int len = strlen(MPIR_pset_list[i]);

    if (*pset_len == 0) {
        *pset_len = len + 1;
        goto fn_exit;
    }

    if (len > *pset_len - 1) {
        len = *pset_len - 1;
    }
    strncpy(pset_name, MPIR_pset_list[i], len);
    pset_name[len] = '\0';

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Session_get_info_impl(MPIR_Session * session_ptr, MPIR_Info ** info_p_p)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Info_alloc(info_p_p);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Info_set_impl(*info_p_p, "mpi_thread_support_level", "MPI_THREAD_MULTIPLE");
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    *info_p_p = NULL;
    goto fn_exit;
}

int MPIR_Session_get_pset_info_impl(MPIR_Session * session_ptr, const char *pset_name,
                                    MPIR_Info ** info_p_p)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Info_alloc(info_p_p);
    MPIR_ERR_CHECK(mpi_errno);

    int mpi_size;
    if (strcmp(pset_name, "mpi://WORLD") == 0) {
        mpi_size = MPIR_Process.size;
    } else if (strcmp(pset_name, "mpi://SELF") == 0) {
        mpi_size = 1;
    } else {
        /* TODO: Implement pset struct, locate pset struct ptr */
        MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_ARG, goto fn_fail, "**psetinvalidname");
    }

    char buf[20];
    sprintf(buf, "%d", mpi_size);
    mpi_errno = MPIR_Info_set_impl(*info_p_p, "mpi_size", buf);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    *info_p_p = NULL;
    goto fn_exit;
}

int MPIR_Abort_impl(MPIR_Comm * comm_ptr, int errorcode)
{
    int mpi_errno = MPI_SUCCESS;

    if (!comm_ptr) {
        /* Use comm world if the communicator is not valid */
        comm_ptr = MPIR_Process.comm_self;
    }

    char abort_str[MPI_MAX_OBJECT_NAME + 100] = "";
    char comm_name[MPI_MAX_OBJECT_NAME];
    int len = MPI_MAX_OBJECT_NAME;
    MPIR_Comm_get_name_impl(comm_ptr, comm_name, &len);
    if (len == 0) {
        MPL_snprintf(comm_name, MPI_MAX_OBJECT_NAME, "comm=0x%X", comm_ptr->handle);
    }
    if (!MPIR_CVAR_SUPPRESS_ABORT_MESSAGE)
        /* FIXME: This is not internationalized */
        MPL_snprintf(abort_str, sizeof(abort_str),
                     "application called MPI_Abort(%s, %d) - process %d", comm_name, errorcode,
                     comm_ptr->rank);
    mpi_errno = MPID_Abort(comm_ptr, mpi_errno, errorcode, abort_str);

    return mpi_errno;
}
