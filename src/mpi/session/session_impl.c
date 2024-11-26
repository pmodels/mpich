/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpir_session.h"
#include "mpi_init.h"
#include "mpir_pset.h"

#define MPIR_SESSION_INFO_ITEM_LEN 20
#define MPIR_SESSION_NUM_DEFAULT_PSETS 2
#define MPIR_SESSION_WORLD_PSET_IDX 0
#define MPIR_SESSION_SELF_PSET_IDX 1

int MPIR_Session_init_impl(MPIR_Info * info_ptr, MPIR_Errhandler * errhandler_ptr,
                           MPIR_Session ** p_session_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Session *session_ptr = NULL;

    int thread_level;
    bool strict_finalize;
    char *memory_alloc_kinds;
    /* Get the thread level requested by the user via info object (if any) */
    mpi_errno = MPIR_Session_get_thread_level_from_info(info_ptr, &thread_level);
    MPIR_ERR_CHECK(mpi_errno);

    /* Get the strict finalize parameter via info object (if any) */
    mpi_errno = MPIR_Session_get_strict_finalize_from_info(info_ptr, &strict_finalize);
    MPIR_ERR_CHECK(mpi_errno);

    /* Remark on MPI_THREAD_SINGLE: Multiple sessions may run in threads
     * concurrently, so significant work is needed to support per-session MPI_THREAD_SINGLE.
     * For now, it probably still works with MPI_THREAD_SINGLE since we use MPL_Initlock_lock
     * for cross-session locks in MPII_Init_thread.
     *
     * The MPI4 standard recommends users to _not_ request MPI_THREAD_SINGLE thread
     * support level for sessions "because this will conflict with other components of an
     * application requesting higher levels of thread support" (Sec. 11.3.1).
     *
     * TODO: support per-session MPI_THREAD_SINGLE, use user-requested thread level here
     * instead of MPI_THREAD_MULTIPLE, and optimize
     */
    int provided;

    mpi_errno = MPII_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided, &session_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    session_ptr->thread_level = provided;

    session_ptr->requested_thread_level = thread_level;
    session_ptr->strict_finalize = strict_finalize;

    /* Get memory allocation kinds requested by the user (if any). This depends on CVAR
     * infrastructure, so it must run after MPII_Init_thread. */
    mpi_errno = MPIR_Session_get_memory_kinds_from_info(info_ptr, &memory_alloc_kinds);
    MPIR_ERR_CHECK(mpi_errno);
    if (memory_alloc_kinds) {
        session_ptr->memory_alloc_kinds = memory_alloc_kinds;
    } else {
        MPIR_Assert(MPIR_Process.memory_alloc_kinds);
        session_ptr->memory_alloc_kinds = MPL_strdup(MPIR_Process.memory_alloc_kinds);
    }

    *p_session_ptr = session_ptr;

  fn_exit:
    return mpi_errno;

  fn_fail:
    if (session_ptr) {
        MPIR_Session_release(session_ptr);
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
    *npset_names = MPIR_SESSION_NUM_DEFAULT_PSETS + MPIR_Pset_count();
    return MPI_SUCCESS;
}

int MPIR_Session_get_nth_pset_impl(MPIR_Session * session_ptr, MPIR_Info * info_ptr,
                                   int n, int *pset_len, char *pset_name)
{
    int mpi_errno = MPI_SUCCESS;
    char *uri = NULL;
    bool uri_needs_free = false;
    int num_psets = 0;
    MPIR_Session_get_num_psets_impl(session_ptr, info_ptr, &num_psets);

    if (n < 0 || (n >= num_psets)) {
        MPIR_ERR_SETANDSTMT1(mpi_errno, MPI_ERR_ARG, goto fn_fail, "**psetinvalidn",
                             "**psetinvalidn %d", n);
    } else if (n == MPIR_SESSION_WORLD_PSET_IDX) {
        uri =
            MPL_malloc((strlen(MPIR_SESSION_WORLD_PSET_NAME) + 1) * sizeof(char), MPL_MEM_SESSION);
        MPL_strncpy(uri, MPIR_SESSION_WORLD_PSET_NAME, strlen(MPIR_SESSION_WORLD_PSET_NAME) + 1);
        uri_needs_free = true;
    } else if (n == MPIR_SESSION_SELF_PSET_IDX) {
        uri = MPL_malloc((strlen(MPIR_SESSION_SELF_PSET_NAME) + 1) * sizeof(char), MPL_MEM_SESSION);
        MPL_strncpy(uri, MPIR_SESSION_SELF_PSET_NAME, strlen(MPIR_SESSION_SELF_PSET_NAME) + 1);
        uri_needs_free = true;
    } else {
        MPIR_Pset *pset;
        mpi_errno = MPIR_Pset_by_idx(n - MPIR_SESSION_NUM_DEFAULT_PSETS, &pset);
        MPIR_ERR_CHECK(mpi_errno);
        uri = pset->uri;
    }

    MPIR_Assert(uri != NULL);
    int len = strlen(uri);

    /* If the name buffer length is 0, just return needed length */
    if (*pset_len == 0) {
        *pset_len = len + 1;
        goto fn_exit;
    }

    /* Copy the name, truncate if necessary */
    if (len > *pset_len - 1) {
        len = *pset_len - 1;
    }
    strncpy(pset_name, uri, len);
    pset_name[len] = '\0';

  fn_exit:
    if (uri_needs_free) {
        MPL_free(uri);
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Session_get_info_impl(MPIR_Session * session_ptr, MPIR_Info ** info_p_p)
{
    int mpi_errno = MPI_SUCCESS;
    const char *buf_thread_level = MPII_threadlevel_name(session_ptr->thread_level);

    int len_strict_finalize = snprintf(NULL, 0, "%d", session_ptr->strict_finalize) + 1;
    char *buf_strict_finalize = MPL_malloc(len_strict_finalize, MPL_MEM_BUFFER);
    snprintf(buf_strict_finalize, len_strict_finalize, "%d", session_ptr->strict_finalize);

    mpi_errno = MPIR_Info_alloc(info_p_p);
    MPIR_ERR_CHECK(mpi_errno);

    /* Set thread level */
    mpi_errno = MPIR_Info_set_impl(*info_p_p, "thread_level", buf_thread_level);
    MPIR_ERR_CHECK(mpi_errno);

    /* Set strict finalize */
    mpi_errno = MPIR_Info_set_impl(*info_p_p, "strict_finalize", buf_strict_finalize);
    MPIR_ERR_CHECK(mpi_errno);

    /* Set memory allocation kinds */
    mpi_errno =
        MPIR_Info_set_impl(*info_p_p, "mpi_memory_alloc_kinds", session_ptr->memory_alloc_kinds);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (buf_strict_finalize) {
        MPL_free(buf_strict_finalize);
    }
    return mpi_errno;
  fn_fail:
    *info_p_p = NULL;
    goto fn_exit;
}

int MPIR_Session_get_pset_info_impl(MPIR_Session * session_ptr, const char *pset_name,
                                    MPIR_Info ** info_p_p)
{
    int mpi_errno = MPI_SUCCESS;
    char buf_size[MPIR_SESSION_INFO_ITEM_LEN];
    char buf_valid[MPIR_SESSION_INFO_ITEM_LEN];
    int size = -1;
    bool is_valid = true;

    mpi_errno = MPIR_Info_alloc(info_p_p);
    MPIR_ERR_CHECK(mpi_errno);

    if (MPL_stricmp(pset_name, MPIR_SESSION_WORLD_PSET_NAME) == 0) {
        size = MPIR_Process.size;
    } else if (MPL_stricmp(pset_name, MPIR_SESSION_SELF_PSET_NAME) == 0) {
        size = 1;
    } else {
        MPIR_Pset *pset = NULL;
        mpi_errno = MPIR_Pset_by_name(pset_name, &pset);
        if (mpi_errno == MPI_ERR_OTHER || pset == NULL) {
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_ARG, goto fn_fail, "**psetinvalidname");
        }

        size = pset->size;
        is_valid = pset->is_valid;
    }

    snprintf(buf_size, MPIR_SESSION_INFO_ITEM_LEN, "%d", size);
    mpi_errno = MPIR_Info_set_impl(*info_p_p, "mpi_size", buf_size);
    MPIR_ERR_CHECK(mpi_errno);

    snprintf(buf_valid, MPIR_SESSION_INFO_ITEM_LEN, "%d", is_valid);
    mpi_errno = MPIR_Info_set_impl(*info_p_p, "pset_valid", buf_valid);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    *info_p_p = NULL;
    goto fn_exit;
}
