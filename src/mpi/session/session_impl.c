/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpir_session.h"
#include "mpi_init.h"

/* Psets */
static const char *default_pset_list[] = {
    "mpi://WORLD",
    "mpi://SELF",
    NULL
};

/* TODO: move into MPIR_Session struct */
const char **MPIR_pset_list = default_pset_list;

int MPIR_Session_init_impl(MPIR_Info * info_ptr, MPIR_Errhandler * errhandler_ptr,
                           MPIR_Session ** p_session_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Session *session_ptr = NULL;

    int thread_level;
    bool strict_finalize;
    char *memory_alloc_kinds;
    /* Get the thread level requested by the user via info object or set the default */
    mpi_errno = MPIR_Session_get_thread_level_from_info(info_ptr, &thread_level);
    MPIR_ERR_CHECK(mpi_errno);

    /* Get the strict finalize parameter via info object (if any) */
    mpi_errno = MPIR_Session_get_strict_finalize_from_info(info_ptr, &strict_finalize);
    MPIR_ERR_CHECK(mpi_errno);

    int provided;
    mpi_errno = MPII_Init_thread(NULL, NULL, thread_level, &provided, &session_ptr);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(provided == MPIR_ThreadInfo.thread_provided);

    /* Enter global CS after system initialized */
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

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

    if (errhandler_ptr) {
        session_ptr->errhandler = errhandler_ptr;
        MPIR_Errhandler_add_ref(errhandler_ptr);
    }

    /* populate psets */
    session_ptr->num_psets = 2;
    session_ptr->psets = MPL_malloc(session_ptr->num_psets * sizeof(struct MPIR_Pset),
                                    MPL_MEM_GROUP);
    MPIR_ERR_CHKANDJUMP(!session_ptr->psets, mpi_errno, MPI_ERR_OTHER, "**nomem");

    session_ptr->psets[0].name = MPL_strdup("mpi://WORLD");
    mpi_errno = MPIR_Group_dup(MPIR_GROUP_WORLD_PTR, session_ptr, &session_ptr->psets[0].group);
    MPIR_ERR_CHECK(mpi_errno);

    session_ptr->psets[1].name = MPL_strdup("mpi://SELF");
    mpi_errno = MPIR_Group_dup(MPIR_GROUP_SELF_PTR, session_ptr, &session_ptr->psets[1].group);
    MPIR_ERR_CHECK(mpi_errno);

    /* TODO: append a list of dynamically updated global psets */

    *p_session_ptr = session_ptr;

  fn_exit:
    if (session_ptr) {
        MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    }
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

    /* account the reference taken by psets and self */
    int session_refs = MPIR_Object_get_ref(session_ptr) - session_ptr->num_psets - 1;

    if ((session_refs > 0) && session_ptr->strict_finalize) {
        /* For strict_finalize, we return an error if there still exist
         * other refs to the session (other than the self-ref).
         * In addition, we call MPID_Progress_poke() to allow users to
         * poll for success of the session finalize.
         */
        MPID_Progress_poke();
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                         MPI_ERR_PENDING, "**sessioninuse", "**sessioninuse %d",
                                         session_refs - 1);
        goto fn_fail;
    }

    for (int i = 0; i < session_ptr->num_psets; i++) {
        mpi_errno = MPIR_Group_free_impl(session_ptr->psets[i].group);
        MPIR_ERR_CHECK(mpi_errno);

        MPL_free(session_ptr->psets[i].name);
    }
    MPL_free(session_ptr->psets);

    mpi_errno = MPIR_Session_release(session_ptr);
    MPIR_ERR_CHECK(mpi_errno);

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

    if (!MPIR_pset_list[i]) {
        MPIR_ERR_SETANDSTMT1(mpi_errno, MPI_ERR_ARG, goto fn_fail, "**psetinvalidn",
                             "**psetinvalidn %d", n);
    }

    int len = strlen(MPIR_pset_list[i]);

    /* if the name buffer length is 0, just return needed length */
    if (*pset_len == 0) {
        *pset_len = len + 1;
        goto fn_exit;
    }

    /* copy the name, truncate if necessary */
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
    const char *buf_thread_level = MPII_threadlevel_name(MPIR_ThreadInfo.thread_provided);

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
    snprintf(buf, sizeof(buf), "%d", mpi_size);
    mpi_errno = MPIR_Info_set_impl(*info_p_p, "mpi_size", buf);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    *info_p_p = NULL;
    goto fn_exit;
}
