/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4_INIT_H_INCLUDED
#define CH4_INIT_H_INCLUDED

#include "ch4_impl.h"
#include "ch4r_proc.h"
#include "ch4i_comm.h"
#include "strings.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : CH4
      description : cvars that control behavior of the CH4 device

cvars:
    - name        : MPIR_CVAR_CH4_NETMOD
      category    : CH4
      type        : string
      default     : ""
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If non-empty, this cvar specifies which network module to use

    - name        : MPIR_CVAR_CH4_SHM
      category    : CH4
      type        : string
      default     : ""
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If non-empty, this cvar specifies which shm module to use

    - name        : MPIR_CVAR_CH4_SHMAM_EAGER
      category    : CH4
      type        : string
      default     : ""
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If non-empty, this cvar specifies which shmam eager module to use

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#undef FUNCNAME
#define FUNCNAME MPIDI_choose_netmod
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_choose_netmod(void)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_CHOOSE_NETMOD);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_CHOOSE_NETMOD);

    MPIR_Assert(MPIR_CVAR_CH4_NETMOD != NULL);

    if (strcmp(MPIR_CVAR_CH4_NETMOD, "") == 0) {
        /* netmod not specified, using the default */
        MPIDI_NM_func = MPIDI_NM_funcs[0];
        MPIDI_NM_native_func = MPIDI_NM_native_funcs[0];
        goto fn_exit;
    }

    for (i = 0; i < MPIDI_num_netmods; ++i) {
        /* use MPL variant of strncasecmp if we get one */
        if (!strncasecmp(MPIR_CVAR_CH4_NETMOD, MPIDI_NM_strings[i], MPIDI_MAX_NETMOD_STRING_LEN)) {
            MPIDI_NM_func = MPIDI_NM_funcs[i];
            MPIDI_NM_native_func = MPIDI_NM_native_funcs[i];
            goto fn_exit;
        }
    }

    MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**invalid_netmod", "**invalid_netmod %s",
                         MPIR_CVAR_CH4_NETMOD);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_CHOOSE_NETMOD);
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_choose_shm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_choose_shm(void)
{

    int mpi_errno = MPI_SUCCESS;
#if defined(MPIDI_BUILD_CH4_SHM)
    int i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_CHOOSE_SHM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_CHOOSE_SHM);


    MPIR_Assert(MPIR_CVAR_CH4_SHM != NULL);

    if (strcmp(MPIR_CVAR_CH4_SHM, "") == 0) {
        /* shm not specified, using the default */
        MPIDI_SHM_func = MPIDI_SHM_funcs[0];
        MPIDI_SHM_native_func = MPIDI_SHM_native_funcs[0];
        goto fn_exit;
    }

    for (i = 0; i < MPIDI_num_shms; ++i) {
        /* use MPL variant of strncasecmp if we get one */
        if (!strncasecmp(MPIR_CVAR_CH4_SHM, MPIDI_SHM_strings[i], MPIDI_MAX_SHM_STRING_LEN)) {
            MPIDI_SHM_func = MPIDI_SHM_funcs[i];
            MPIDI_SHM_native_func = MPIDI_SHM_native_funcs[i];
            goto fn_exit;
        }
    }

    MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**ch4|invalid_shm", "**ch4|invalid_shm %s",
                         MPIR_CVAR_CH4_SHM);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_CHOOSE_SHM);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#else
    return mpi_errno;
#endif
}


#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ)
#define MAX_THREAD_MODE MPI_THREAD_MULTIPLE
#elif  (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL)
#define MAX_THREAD_MODE MPI_THREAD_MULTIPLE
#elif  (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__SINGLE)
#define MAX_THREAD_MODE MPI_THREAD_SERIALIZED
#elif  (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__LOCKFREE)
#define MAX_THREAD_MODE MPI_THREAD_SERIALIZED
#else
#error "Thread Granularity:  Invalid"
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_Init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Init(int *argc,
                                        char ***argv,
                                        int requested, int *provided, int *has_args, int *has_env)
{
    int pmi_errno, mpi_errno = MPI_SUCCESS, rank, has_parent, size, appnum, thr_err;
    void *netmod_contexts;
    int avtid, max_n_avts;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_INIT);

#ifdef MPL_USE_DBG_LOGGING
    MPIDI_CH4_DBG_GENERAL = MPL_dbg_class_alloc("CH4", "ch4");
    MPIDI_CH4_DBG_MAP = MPL_dbg_class_alloc("CH4_MAP", "ch4_map");
    MPIDI_CH4_DBG_MEMORY = MPL_dbg_class_alloc("CH4_MEMORY", "ch4_memory");
#endif
    MPIDI_choose_netmod();
    pmi_errno = PMI_Init(&has_parent);

    if (pmi_errno != PMI_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(pmi_errno, MPI_ERR_OTHER, "**pmi_init", "**pmi_init %d", pmi_errno);
    }

    pmi_errno = PMI_Get_rank(&rank);

    if (pmi_errno != PMI_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(pmi_errno, MPI_ERR_OTHER, "**pmi_get_rank",
                             "**pmi_get_rank %d", pmi_errno);
    }

    pmi_errno = PMI_Get_size(&size);

    if (pmi_errno != 0) {
        MPIR_ERR_SETANDJUMP1(pmi_errno, MPI_ERR_OTHER, "**pmi_get_size",
                             "**pmi_get_size %d", pmi_errno);
    }

    pmi_errno = PMI_Get_appnum(&appnum);

    if (pmi_errno != PMI_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(pmi_errno, MPI_ERR_OTHER, "**pmi_get_appnum",
                             "**pmi_get_appnum %d", pmi_errno);
    }

    MPID_Thread_mutex_create(&MPIDI_CH4I_THREAD_PROGRESS_MUTEX, &thr_err);
    MPID_Thread_mutex_create(&MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX, &thr_err);

    /* ---------------------------------- */
    /* Initialize MPI_COMM_SELF           */
    /* ---------------------------------- */
    MPIR_Process.comm_self->rank = 0;
    MPIR_Process.comm_self->remote_size = 1;
    MPIR_Process.comm_self->local_size = 1;

    /* ---------------------------------- */
    /* Initialize MPI_COMM_WORLD          */
    /* ---------------------------------- */
    MPIR_Process.comm_world->rank = rank;
    MPIR_Process.comm_world->remote_size = size;
    MPIR_Process.comm_world->local_size = size;

    mpi_errno = MPIDI_choose_shm();
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POPFATAL(mpi_errno);
    }

    MPIDI_CH4_Global.allocated_max_n_avts = 0;
    MPIDIU_avt_init();
    MPIDIU_get_next_avtid(&avtid);
    MPIR_Assert(avtid == 0);
    max_n_avts = MPIDIU_get_max_n_avts();

    MPIDII_av_table = (MPIDII_av_table_t **)
        MPL_malloc(max_n_avts * sizeof(MPIDII_av_table_t *));

    MPIDII_av_table[0] = (MPIDII_av_table_t *)
        MPL_malloc(size * sizeof(MPIDII_av_entry_t)
                   + sizeof(MPIDII_av_table_t));

    MPIDII_av_table[0]->size = size;
    MPIR_Object_set_ref(MPIDII_av_table[0], 1);

    MPIDIU_alloc_globals_for_avtid(avtid);

    MPIDII_av_table0 = MPIDII_av_table[0];

    /* initialize rank_map */
    MPIDII_COMM(MPIR_Process.comm_world, map).mode = MPIDII_RANK_MAP_DIRECT_INTRA;
    MPIDII_COMM(MPIR_Process.comm_world, map).avtid = 0;
    MPIDII_COMM(MPIR_Process.comm_world, map).size = size;
    MPIDII_COMM(MPIR_Process.comm_world, local_map).mode = MPIDII_RANK_MAP_NONE;
    MPIDIU_avt_add_ref(0);

    MPIDII_COMM(MPIR_Process.comm_self, map).mode = MPIDII_RANK_MAP_OFFSET_INTRA;
    MPIDII_COMM(MPIR_Process.comm_self, map).avtid = 0;
    MPIDII_COMM(MPIR_Process.comm_self, map).size = 1;
    MPIDII_COMM(MPIR_Process.comm_self, map).reg.offset = rank;
    MPIDII_COMM(MPIR_Process.comm_self, local_map).mode = MPIDII_RANK_MAP_NONE;
    MPIDIU_avt_add_ref(0);

    MPIR_Process.attrs.tag_ub = (1ULL << MPIDI_CH4U_TAG_SHIFT) - 1;
    /* discuss */

    mpi_errno = MPIDI_NM_mpi_init_hook(rank, size, appnum, &MPIR_Process.attrs.tag_ub,
                                       MPIR_Process.comm_world,
                                       MPIR_Process.comm_self, has_parent, 1, &netmod_contexts);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POPFATAL(mpi_errno);
    }

#ifdef MPIDI_BUILD_CH4_LOCALITY_INFO
    int i;
    for (i = 0; i < MPIR_Process.comm_world->local_size; i++) {
        MPIDII_av_table0->table[i].is_local = 0;
    }
    MPIDI_CH4U_build_nodemap(MPIR_Process.comm_world->rank,
                             MPIR_Process.comm_world,
                             MPIR_Process.comm_world->local_size,
                             MPIDI_CH4_Global.node_map[0], &MPIDI_CH4_Global.max_node_id);

    for (i = 0; i < MPIR_Process.comm_world->local_size; i++) {
        MPIDII_av_table0->table[i].is_local =
            (MPIDI_CH4_Global.node_map[0][i] ==
             MPIDI_CH4_Global.node_map[0][MPIR_Process.comm_world->rank]) ? 1 : 0;
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, "WORLD RANK %d %s local", i,
                         MPIDII_av_table0->table[i].is_local ? "is" : "is not"));
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, "Node id (i) (me) %d %d", MPIDI_CH4_Global.node_map[0][i],
                         MPIDI_CH4_Global.node_map[0][MPIR_Process.comm_world->rank]));
    }
#endif

#ifdef MPIDI_BUILD_CH4_SHM
    mpi_errno = MPIDI_SHM_mpi_init_hook(rank, size);

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POPFATAL(mpi_errno);
    }
#endif

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    MPIR_Process.attrs.appnum = appnum;
    MPIR_Process.attrs.wtime_is_global = 1;
    MPIR_Process.attrs.io = MPI_ANY_SOURCE;

    MPIR_Comm_commit(MPIR_Process.comm_self);
    MPIR_Comm_commit(MPIR_Process.comm_world);

    /* -------------------------------- */
    /* Return MPICH Parameters          */
    /* -------------------------------- */
    switch (requested) {
    case MPI_THREAD_SINGLE:
    case MPI_THREAD_SERIALIZED:
    case MPI_THREAD_FUNNELED:
        *provided = requested;
        break;

    case MPI_THREAD_MULTIPLE:
        *provided = MAX_THREAD_MODE;
        break;
    }

    *has_args = TRUE;
    *has_env = TRUE;
    MPIDI_CH4_Global.is_initialized = 0;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_InitCompleted
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_InitCompleted(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_INITCOMPLETED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_INITCOMPLETED);
    MPIDI_CH4_Global.is_initialized = 1;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_INITCOMPLETED);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Finalize(void)
{
    int mpi_errno, thr_err;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_FINALIZE);

    mpi_errno = MPIDI_NM_mpi_finalize_hook();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
#ifdef MPIDI_BUILD_CH4_SHM
    mpi_errno = MPIDI_SHM_mpi_finalize_hook();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
#endif

    int i;
    int max_n_avts;
    max_n_avts = MPIDIU_get_max_n_avts();
    for (i = 0; i < max_n_avts; i++) {
        if (MPIDII_av_table[i] != NULL) {
            MPIDIU_avt_release_ref(i);
        }
    }
    MPL_free(MPIDII_av_table);
    MPL_free(MPIDI_CH4_Global.node_map);

    MPIDIU_avt_destroy();

    MPID_Thread_mutex_destroy(&MPIDI_CH4I_THREAD_PROGRESS_MUTEX, &thr_err);
    MPID_Thread_mutex_destroy(&MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX, &thr_err);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Get_universe_size
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Get_universe_size(int *universe_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno = PMI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_GET_UNIVERSE_SIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_GET_UNIVERSE_SIZE);


    pmi_errno = PMI_Get_universe_size(universe_size);

    if (pmi_errno != PMI_SUCCESS)
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER,
                             "**pmi_get_universe_size", "**pmi_get_universe_size %d", pmi_errno);

    if (*universe_size < 0)
        *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_GET_UNIVERSE_SIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Get_processor_name
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Get_processor_name(char *name, int namelen, int *resultlen)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_GET_PROCESSOR_NAME);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_GET_PROCESSOR_NAME);

    if (!MPIDI_CH4_Global.pname_set) {
#if defined(HAVE_GETHOSTNAME)

        if (gethostname(MPIDI_CH4_Global.pname, MPI_MAX_PROCESSOR_NAME) == 0)
            MPIDI_CH4_Global.pname_len = (int) strlen(MPIDI_CH4_Global.pname);

#elif defined(HAVE_SYSINFO)

        if (sysinfo(SI_HOSTNAME, MPIDI_CH4_Global.pname, MPI_MAX_PROCESSOR_NAME) == 0)
            MPIDI_CH4_Global.pname_len = (int) strlen(MPIDI_CH4_Global.pname);

#else
        MPL_snprintf(MPIDI_CH4_Global.pname, MPI_MAX_PROCESSOR_NAME, "%d",
                     MPIR_Process.comm_world->rank);
        MPIDI_CH4_Global.pname_len = (int) strlen(MPIDI_CH4_Global.pname);
#endif
        MPIDI_CH4_Global.pname_set = 1;
    }

    MPIR_ERR_CHKANDJUMP(MPIDI_CH4_Global.pname_len <= 0,
                        mpi_errno, MPI_ERR_OTHER, "**procnamefailed");
    MPL_strncpy(name, MPIDI_CH4_Global.pname, namelen);

    if (resultlen)
        *resultlen = MPIDI_CH4_Global.pname_len;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_GET_PROCESSOR_NAME);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Abort
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Abort(MPIR_Comm * comm,
                                         int mpi_errno, int exit_code, const char *error_msg)
{
    char sys_str[MPI_MAX_ERROR_STRING + 5] = "";
    char comm_str[MPI_MAX_ERROR_STRING] = "";
    char world_str[MPI_MAX_ERROR_STRING] = "";
    char error_str[2 * MPI_MAX_ERROR_STRING + 128];
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_ABORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_ABORT);

    if (MPIR_Process.comm_world) {
        int rank = MPIR_Process.comm_world->rank;
        snprintf(world_str, sizeof(world_str), " on node %d", rank);
    }

    if (comm) {
        int rank = comm->rank;
        int context_id = comm->context_id;
        snprintf(comm_str, sizeof(comm_str), " (rank %d in comm %d)", rank, context_id);
    }

    if (!error_msg)
        error_msg = "Internal error";

    if (mpi_errno != MPI_SUCCESS) {
        char msg[MPI_MAX_ERROR_STRING] = "";
        MPIR_Err_get_string(mpi_errno, msg, MPI_MAX_ERROR_STRING, NULL);
        snprintf(sys_str, sizeof(msg), " (%s)", msg);
    }
    MPL_snprintf(error_str, sizeof(error_str), "Abort(%d)%s%s: %s%s\n",
                 exit_code, world_str, comm_str, error_msg, sys_str);
    MPL_error_printf("%s", error_str);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_ABORT);
    fflush(stderr);
    fflush(stdout);
    PMI_Abort(exit_code, error_msg);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Alloc_mem
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void *MPIDI_Alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    void *p;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_ALLOC_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_ALLOC_MEM);

    p = MPIDI_NM_mpi_alloc_mem(size, info_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_ALLOC_MEM);
    return p;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Free_mem
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Free_mem(void *ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_FREE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_FREE_MEM);
    mpi_errno = MPIDI_NM_mpi_free_mem(ptr);

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_FREE_MEM);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Comm_get_lpid
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_get_lpid(MPIR_Comm * comm_ptr,
                                                 int idx, int *lpid_ptr, MPL_bool is_remote)
{
    int mpi_errno = MPI_SUCCESS;
    int avtid = 0, lpid = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_COMM_GET_LPID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_COMM_GET_LPID);

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    else if (is_remote)
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    else {
        MPIDIU_comm_rank_to_pid_local(comm_ptr, idx, &lpid, &avtid);
    }

    *lpid_ptr = MPIDIU_LPID_CREATE(avtid, lpid);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_COMM_GET_LPID);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_GPID_Get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_GPID_Get(MPIR_Comm * comm_ptr, int rank, MPIR_Gpid * gpid)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_GPID_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_GPID_GET);

    mpi_errno = MPIDI_NM_gpid_get(comm_ptr, rank, gpid);
    MPIDI_CH4U_get_node_id(comm_ptr, rank, &MPIDII_GPID(gpid).node);

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_GPID_GET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Get_node_id
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Get_node_id(MPIR_Comm * comm, int rank, MPID_Node_id_t * id_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_GET_NODE_ID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_GET_NODE_ID);

    MPIDI_CH4U_get_node_id(comm, rank, id_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_GET_NODE_ID);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Get_max_node_id
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Get_max_node_id(MPIR_Comm * comm, MPID_Node_id_t * max_id_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_GET_MAX_NODE_ID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_GET_MAX_NODE_ID);

    MPIDI_CH4U_get_max_node_id(comm, max_id_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_GET_MAX_NODE_ID);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_GetAllInComm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_GPID_GetAllInComm(MPIR_Comm * comm_ptr,
                                                     int local_size, MPIR_Gpid local_gpids[],
                                                     int *singleAVT)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_GETALLINCOMM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_GETALLINCOMM);

    mpi_errno = MPIDI_NM_getallincomm(comm_ptr, local_size, local_gpids, singleAVT);

    if (MPIDII_COMM(comm_ptr, map).mode == MPIDII_RANK_MAP_MLUT) {
        *singleAVT = FALSE;
    }
    else {
        *singleAVT = TRUE;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_GETALLINCOMM);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_GPID_ToLpidArray
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_GPID_ToLpidArray(int size, MPIR_Gpid gpid[], int lpid[])
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_GPID_TOLPIDARRAY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_GPID_TOLPIDARRAY);

    mpi_errno = MPIDI_NM_gpid_tolpidarray(size, gpid, lpid);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    /* update node_map */
    for (i = 0; i < size; i++) {
        int _avtid = 0, _lpid = 0;
        /* if this is a new process, update node_map and locality */
        if (MPIDIU_LPID_IS_NEW_AVT(lpid[i])) {
            MPIDIU_LPID_CLEAR_NEW_AVT_MARK(lpid[i]);
            _avtid = MPIDIU_LPID_GET_AVTID(lpid[i]);
            _lpid = MPIDIU_LPID_GET_LPID(lpid[i]);
            MPIDI_CH4_Global.node_map[_avtid][_lpid] = MPIDII_GPID(&gpid[i]).node;
            /* new process groups are always assumed to be remote */
#ifdef MPIDI_BUILD_CH4_LOCALITY_INFO
            if (_avtid != 0) {
                MPIDII_av_table[_avtid]->table[_lpid].is_local = 0;
            }
#endif
        }
    }

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_GPID_TOLPIDARRAY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Create_intercomm_from_lpids
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                               int size, const int lpids[])
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_CREATE_INTERCOMM_FROM_LPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_CREATE_INTERCOMM_FROM_LPIDS);

    MPIDII_rank_map_mlut_t *mlut = NULL;
    MPIDII_COMM(newcomm_ptr, map).mode = MPIDII_RANK_MAP_MLUT;
    MPIDII_COMM(newcomm_ptr, map).avtid = -1;
    mpi_errno = MPIDII_alloc_mlut(&mlut, size);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
    MPIDII_COMM(newcomm_ptr, map).size = size;
    MPIDII_COMM(newcomm_ptr, map).irreg.mlut.t = mlut;
    MPIDII_COMM(newcomm_ptr, map).irreg.mlut.gpid = mlut->gpid;

    for (i = 0; i < size; i++) {
        MPIDII_COMM(newcomm_ptr, map).irreg.mlut.gpid[i].avtid = MPIDIU_LPID_GET_AVTID(lpids[i]);
        MPIDII_COMM(newcomm_ptr, map).irreg.mlut.gpid[i].lpid = MPIDIU_LPID_GET_LPID(lpids[i]);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " remote rank=%d, avtid=%d, lpid=%d", i,
                         MPIDII_COMM(newcomm_ptr, map).irreg.mlut.gpid[i].avtid,
                         MPIDII_COMM(newcomm_ptr, map).irreg.mlut.gpid[i].lpid));
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_CREATE_INTERCOMM_FROM_LPIDS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Aint_add
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_Aint_add(MPI_Aint base, MPI_Aint disp)
{
    MPI_Aint result;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_AINT_ADD);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_AINT_ADD);
    result = MPIR_VOID_PTR_CAST_TO_MPI_AINT((char *) MPIR_AINT_CAST_TO_VOID_PTR(base) + disp);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_AINT_ADD);
    return result;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Aint_diff
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_Aint_diff(MPI_Aint addr1, MPI_Aint addr2)
{
    MPI_Aint result;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_AINT_DIFF);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_AINT_DIFF);

    result = MPIR_PTR_DISP_CAST_TO_MPI_AINT((char *) MPIR_AINT_CAST_TO_VOID_PTR(addr1)
                                            - (char *) MPIR_AINT_CAST_TO_VOID_PTR(addr2));
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_AINT_DIFF);
    return result;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Type_create_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Type_create_hook(MPIR_Datatype * type)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_TYPE_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_TYPE_CREATE_HOOK);

    mpi_errno = MPIDI_NM_mpi_type_create_hook(type);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

#if defined(MPIDI_BUILD_CH4_SHM)
    mpi_errno = MPIDI_SHM_mpi_type_create_hook(type);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_TYPE_CREATE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Type_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Type_free_hook(MPIR_Datatype * type)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_TYPE_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_TYPE_FREE_HOOK);

    mpi_errno = MPIDI_NM_mpi_type_free_hook(type);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

#if defined(MPIDI_BUILD_CH4_SHM)
    mpi_errno = MPIDI_SHM_mpi_type_free_hook(type);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_TYPE_FREE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Op_create_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Op_create_hook(MPIR_Op * op)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OP_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OP_CREATE_HOOK);

    mpi_errno = MPIDI_NM_mpi_op_create_hook(op);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

#if defined(MPIDI_BUILD_CH4_SHM)
    mpi_errno = MPIDI_SHM_mpi_op_create_hook(op);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OP_CREATE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Op_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Op_free_hook(MPIR_Op * op)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OP_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OP_FREE_HOOK);

    mpi_errno = MPIDI_NM_mpi_op_free_hook(op);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

#if defined(MPIDI_BUILD_CH4_SHM)
    mpi_errno = MPIDI_SHM_mpi_op_free_hook(op);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OP_FREE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_INIT_H_INCLUDED */
